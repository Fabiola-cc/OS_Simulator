#include "sjf_scheduler.h"
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QScrollBar>
#include <QRandomGenerator>
#include <QColor>
#include <algorithm>
#include <climits>

ShortestJobFirstScheduler::ShortestJobFirstScheduler(QObject *parent) : QObject(parent) {
    ganttScene = new QGraphicsScene(this);
    simulationTimer = new QTimer(this);
    currentTime = 0;
    currentProcessIndex = 0;
    simulationRunning = false;
    currentProcess = nullptr;
    nextIndex = 0;
    
    // Conectar el timer a la función de actualización
    connect(simulationTimer, &QTimer::timeout, this, &ShortestJobFirstScheduler::updateSimulation);
}

void ShortestJobFirstScheduler::setupGanttChart(QGraphicsView *view) {
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


void ShortestJobFirstScheduler::setProcesses(const QList<Process>& newProcesses) {
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

void ShortestJobFirstScheduler::assignProcessColors() {
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

void ShortestJobFirstScheduler::startSimulation() {
    if (!simulationRunning && !processes.isEmpty()) {
        // Inicializar variables de simulación
        currentTime = 0;
        currentProcessIndex = 0;
        currentProcess = nullptr;
        simulationRunning = true;
        nextIndex = 0;
        
        // Limpiar métricas
        waitingTimes.clear();
        completionTimes.clear();
        
        // Iniciar el timer (velocidad de la animación: 500ms por ciclo)
        simulationTimer->start(500);
    }
}

void ShortestJobFirstScheduler::stopSimulation() {
    simulationTimer->stop();
    simulationRunning = false;
    
    // Calcular y mostrar métricas finales
    calculateMetrics();
}

void ShortestJobFirstScheduler::updateSimulation() {
    // Si no hay un proceso en ejecución, buscar el siguiente según su llegada
    if (currentProcess == nullptr || currentProcess->burstTime <= 0) {
        // Obtener el siguiente proceso desde burstSortedProcesses
        if (nextIndex >= burstSortedProcesses.size()) {
            stopSimulation(); // Todos terminaron
            return;
        }

        // Buscar el proceso original para modificar su burstTime real
        QString pid = burstSortedProcesses[nextIndex].pid;
        for (Process& p : processes) {
            if (p.pid == pid) {
                currentProcess = &p;
                break;
            }
        }

        // Si no hay procesos disponibles en este momento
        if (currentProcess == nullptr) {
            // Verificar si quedan procesos por llegar
            bool pendingProcesses = false;
            for (const Process& p : processes) {
                if (p.burstTime > 0) {
                    pendingProcesses = true;
                    break;
                }
            }

            if (!pendingProcesses) {
                // Todos los procesos han terminado
                stopSimulation();
                return;
            } else {
                // Avanzar el tiempo hasta el siguiente proceso
                currentTime++;

                // Dibujar un espacio vacío (CPU idle) en la fila única
                int y = 30;
                ganttScene->addRect(
                    (currentTime - 1) * 30, y, 30, 30,
                    QPen(Qt::black), QBrush(Qt::lightGray)
                );

                QGraphicsTextItem *idleText = ganttScene->addText("idle");
                idleText->setPos((currentTime - 1) * 30 + 5, y + 5);

                // Desplazar la vista para mostrar el ciclo actual
                ganttView->ensureVisible(currentTime * 30, 0, 100, 0);
                return;
            }
        }
        nextIndex++;
    }

    // Ejecutar el proceso actual por un ciclo
    currentProcess->burstTime--;

    // Dibujar el ciclo actual en la misma fila (y = 30)
    int y = 30;
    ganttScene->addRect(
        currentTime * 30, y, 30, 30,
        QPen(Qt::black), QBrush(processColors[currentProcess->pid])
    );

    // Añadir texto del proceso en el diagrama
    QGraphicsTextItem *textItem = ganttScene->addText(currentProcess->pid);
    textItem->setPos(currentTime * 30 + 5, y + 5);

    // Si el proceso ha terminado, registrar su tiempo de finalización
    if (currentProcess->burstTime <= 0) {
        completionTimes[currentProcess->pid] = currentTime + 1;

        // Buscar el proceso original en burstSortedProcesses para obtener el burst original
        auto it = std::find_if(burstSortedProcesses.begin(), burstSortedProcesses.end(),
                               [&](const Process& p) { return p.pid == currentProcess->pid; });

        if (it != burstSortedProcesses.end()) {
            int waitTime = completionTimes[currentProcess->pid] - it->burstTime;
            waitingTimes[currentProcess->pid] = waitTime;
        }
    }

    // Avanzar el tiempo
    currentTime++;

    // Desplazar la vista para mostrar el ciclo actual
    ganttView->ensureVisible(currentTime * 30, 0, 100, 0);
}


void ShortestJobFirstScheduler::calculateMetrics() {
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

double ShortestJobFirstScheduler::simulateWithoutGUI() {
    completionTimes.clear();
    waitingTimes.clear();
    currentTime = 0;

    QList<Process> readyQueue = processes;
    QList<QString> executedPIDs;

    int completed = 0;

    while (completed < processes.size()) {
        Process selected = burstSortedProcesses.first();
        int startTime = currentTime;
        int finishTime = startTime + selected.burstTime;
        int waitingTime = startTime;

        currentTime = finishTime;
        executedPIDs.append(selected.pid);
        completionTimes[selected.pid] = finishTime;
        waitingTimes[selected.pid] = waitingTime;
        
        burstSortedProcesses.removeFirst();
        completed++;
    }

    // Calcular promedio
    double totalWaitingTime = 0;
    for (const QString& pid : waitingTimes.keys()) {
        totalWaitingTime += waitingTimes[pid];
    }

    return totalWaitingTime / processes.size();
}