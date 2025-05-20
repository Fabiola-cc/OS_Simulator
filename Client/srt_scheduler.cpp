#include "srt_scheduler.h"
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QScrollBar>
#include <QRandomGenerator>
#include <QColor>
#include <algorithm>
#include <climits>
#include <QDebug>

ShortestRemainingTimeScheduler::ShortestRemainingTimeScheduler(QObject *parent) : QObject(parent) {
    ganttScene = new QGraphicsScene(this);
    simulationTimer = new QTimer(this);
    currentTime = 0;
    currentProcessIndex = 0;
    simulationRunning = false;
    currentProcess = nullptr;
    
    // Conectar el timer a la función de actualización
    connect(simulationTimer, &QTimer::timeout, this, &ShortestRemainingTimeScheduler::updateSimulation);
}

void ShortestRemainingTimeScheduler::setupGanttChart(QGraphicsView *view) {
    ganttView = view;
    ganttView->setScene(ganttScene);
    ganttView->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    ganttView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    
    // Limpiar la escena
    ganttScene->clear();
    
    // Configurar una escena más compacta (una sola fila horizontal)
    ganttScene->setSceneRect(0, 0, 5000, 100); // Altura reducida
    
    // Línea base para la fila única
    ganttScene->addLine(0, 30, 5000, 30, QPen(Qt::white)); // línea horizontal única

    // Dibujar líneas verticales y etiquetas de tiempo
    for (int i = 0; i <= 100; i++) {
        int x = i * 30;
        ganttScene->addLine(x, 0, x, 60, QPen(Qt::white)); // líneas verticales
        QGraphicsTextItem *timeLabel = ganttScene->addText(QString::number(i));
        timeLabel->setPos(x - 5, 65); // etiqueta del tiempo
    }
}


void ShortestRemainingTimeScheduler::setProcesses(const QList<Process>& newProcesses) {
    processes = newProcesses;
    
    // Ordena los procesos por tiempo de ejecución para procesarlos en orden
    burstSortedProcesses = processes;
    std::sort(burstSortedProcesses.begin(), burstSortedProcesses.end(), 
              [](const Process& a, const Process& b) {
                  return a.burstTime < b.burstTime;
              });
    // Asignar colores aleatorios a cada proceso
    assignProcessColors();
}

void ShortestRemainingTimeScheduler::assignProcessColors() {
    processColors.clear();
    for (const Process& p : processes) {
        if (!processColors.contains(p.pid)) {
            // Generar color aleatorio que no sea demasiado claro
            QColor color;
            do {
                color = QColor(
                    QRandomGenerator::global()->bounded(50, 200),
                    QRandomGenerator::global()->bounded(50, 200),
                    QRandomGenerator::global()->bounded(50, 200)
                );
            } while (color.lightness() > 200);
            
            processColors[p.pid] = color;
        }
    }
}

void ShortestRemainingTimeScheduler::startSimulation() {
    if (!simulationRunning && !processes.isEmpty()) {
        // Inicializar variables de simulación
        currentTime = 0;
        currentProcessIndex = 0;
        currentProcess = nullptr;
        simulationRunning = true;
        
        // Limpiar métricas
        waitingTimes.clear();
        completionTimes.clear();
        
        // Iniciar el timer (velocidad de la animación: 500ms por ciclo)
        simulationTimer->start(500);
    }
}

void ShortestRemainingTimeScheduler::stopSimulation() {
    simulationTimer->stop();
    simulationRunning = false;
    
    // Calcular y mostrar métricas finales
    calculateMetrics();
}

Process* ShortestRemainingTimeScheduler::getNextProcessByBurst() {
    Process* next = nullptr;
    int minRemainingTime = INT_MAX;

    for (Process& p : processes) {
        // El proceso debe haber llegado y no debe estar terminado
        if (p.arrivalTime <= currentTime && p.burstTime > 0) {
            // Seleccionar el que tenga el menor tiempo restante
            if (p.burstTime < minRemainingTime) {
                minRemainingTime = p.burstTime;
                next = &p;
            }
        }
    }

    return next; // nullptr si no hay procesos listos para ejecutar
}

void ShortestRemainingTimeScheduler::updateSimulation() {
    // Verificar si todos los procesos ya terminaron
    bool allDone = true;
    for (const Process& p : processes) {
        if (p.burstTime > 0) {
            allDone = false;
            break;
        }
    }

    if (allDone) {
        stopSimulation();
        return;
    }

    // Obtener el proceso con menor burst restante que haya llegado
    Process* nextProcess = getNextProcessByBurst();

    // Si no hay proceso disponible en este ciclo (CPU idle)
    if (nextProcess == nullptr) {
        int y = 30;
        ganttScene->addRect(
            currentTime * 30, y, 30, 30,
            QPen(Qt::black), QBrush(Qt::lightGray)
        );

        QGraphicsTextItem *idleText = ganttScene->addText("idle");
        idleText->setPos(currentTime * 30 + 5, y + 5);

        currentTime++;
        ganttView->ensureVisible(currentTime * 30, 0, 100, 0);
        return;
    }

    // Interrupción si el siguiente proceso es distinto al actual
    if (currentProcess != nextProcess) {
        currentProcess = nextProcess;
    }

    // Ejecutar el proceso actual por un ciclo
    currentProcess->burstTime--;

    int y = 30;
    ganttScene->addRect(
        currentTime * 30, y, 30, 30,
        QPen(Qt::black), QBrush(processColors[currentProcess->pid])
    );

    QGraphicsTextItem *textItem = ganttScene->addText(currentProcess->pid);
    textItem->setPos(currentTime * 30 + 5, y + 5);

    // Si termina, registrar su tiempo de finalización y tiempo de espera
    if (currentProcess->burstTime == 0) {
        completionTimes[currentProcess->pid] = currentTime + 1;

        // Buscar burst original
        auto it = std::find_if(burstSortedProcesses.begin(), burstSortedProcesses.end(),
                               [&](const Process& p) { return p.pid == currentProcess->pid; });

        if (it != burstSortedProcesses.end()) {
            int waitTime = completionTimes[currentProcess->pid] - it->arrivalTime - it->burstTime;
            waitingTimes[currentProcess->pid] = waitTime;
        }

        currentProcess = nullptr; // Liberar para el siguiente ciclo
    }

    currentTime++;
    ganttView->ensureVisible(currentTime * 30, 0, 100, 0);
}

void ShortestRemainingTimeScheduler::calculateMetrics() {
    double totalWaitingTime = 0;
    
    // Calcular tiempo de espera para cada proceso
    for (const QString& pid : waitingTimes.keys()) {
        totalWaitingTime += waitingTimes[pid];
    }
    
    // Calcular tiempo de espera promedio
    double avgWaitingTime = totalWaitingTime / processes.size();
    
    // Emitir señal con resultados
    emit simulationFinished(avgWaitingTime);
}