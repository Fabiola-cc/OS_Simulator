#include "priority_scheduler.h"
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QScrollBar>
#include <QRandomGenerator>
#include <QColor>
#include <algorithm>
#include <climits>  // Para INT_MAX

PriorityScheduler::PriorityScheduler(QObject *parent) : QObject(parent) {
    ganttScene = new QGraphicsScene(this);
    simulationTimer = new QTimer(this);
    currentTime = 0;
    currentProcessIndex = 0;
    simulationRunning = false;
    currentProcess = nullptr;
    
    // Conectar el timer a la función de actualización
    connect(simulationTimer, &QTimer::timeout, this, &PriorityScheduler::updateSimulation);
}

void PriorityScheduler::setupGanttChart(QGraphicsView *view) {
    ganttView = view;
    ganttView->setScene(ganttScene);
    ganttView->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    ganttView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    
    // Limpiar la escena
    ganttScene->clear();
    
    // Configurar la escena para el diagrama de Gantt
    ganttScene->setSceneRect(0, 0, 5000, 300); // Amplio para permitir scroll
    
    // Dibujar líneas horizontales de base y etiquetas de procesos
    for (int i = 0; i < processes.size(); i++) {
        // Línea base
        ganttScene->addLine(0, (i+1)*30, 5000, (i+1)*30, QPen(Qt::lightGray));
        
        // Etiqueta del proceso
        QGraphicsTextItem *label = ganttScene->addText(processes[i].pid);
        label->setPos(-50, i*30 + 10);
    }
    
    // Dibujar líneas verticales para los ciclos (se actualizarán durante la simulación)
    for (int i = 0; i <= 100; i++) { // Inicialmente dibujamos 100 ciclos
        ganttScene->addLine(i*30, 0, i*30, (processes.size()+1)*30, QPen(Qt::lightGray));
        QGraphicsTextItem *timeLabel = ganttScene->addText(QString::number(i));
        timeLabel->setPos(i*30 - 5, (processes.size()+1)*30);
    }
}

void PriorityScheduler::setProcesses(const QList<Process>& newProcesses) {
    processes = newProcesses;
    
    // Ordena los procesos por tiempo de llegada para procesarlos en orden
    arrivalSortedProcesses = processes;
    std::sort(arrivalSortedProcesses.begin(), arrivalSortedProcesses.end(), 
              [](const Process& a, const Process& b) {
                  return a.arrivalTime < b.arrivalTime;
              });
    
    // Asignar colores aleatorios a cada proceso
    assignProcessColors();
}

void PriorityScheduler::assignProcessColors() {
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

void PriorityScheduler::startSimulation() {
    if (!simulationRunning && !processes.isEmpty()) {
        // Inicializar variables de simulación
        currentTime = 0;
        currentProcessIndex = 0;
        currentProcess = nullptr;
        simulationRunning = true;
        
        // Limpiar métricas
        waitingTimes.clear();
        completionTimes.clear();
        
        // Inicializar diagramas
        drawGanttChart();
        
        // Iniciar el timer (velocidad de la animación: 500ms por ciclo)
        simulationTimer->start(500);
    }
}

void PriorityScheduler::stopSimulation() {
    simulationTimer->stop();
    simulationRunning = false;
    
    // Calcular y mostrar métricas finales
    calculateMetrics();
}

Process* PriorityScheduler::getNextProcessByPriority() {
    Process* next = nullptr;
    int highestPriority = INT_MAX; // Menor número = Mayor prioridad
    
    // Busca entre todos los procesos que han llegado pero no han terminado
    for (int i = 0; i < processes.size(); i++) {
        // Si el proceso ya ha llegado y no ha completado su ejecución
        if (processes[i].arrivalTime <= currentTime && processes[i].burstTime > 0) {
            // Si tiene mayor prioridad (valor más bajo) que el actual mejor candidato
            if (processes[i].priority < highestPriority) {
                highestPriority = processes[i].priority;
                next = &processes[i];
            } 
            // En caso de empate, el que llegó primero (FCFS)
            else if (processes[i].priority == highestPriority && next != nullptr && 
                     processes[i].arrivalTime < next->arrivalTime) {
                next = &processes[i];
            }
        }
    }
    
    return next;
}

void PriorityScheduler::updateSimulation() {
    // Si no hay un proceso en ejecución, buscar el siguiente por prioridad
    if (currentProcess == nullptr || currentProcess->burstTime <= 0) {
        currentProcess = getNextProcessByPriority();
        
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
                // Dibujar un espacio vacío (CPU idle)
                ganttScene->addRect(
                    currentTime * 30 - 30, 0, 30, processes.size() * 30, 
                    QPen(Qt::black), QBrush(Qt::lightGray)
                );
                QGraphicsTextItem *idleText = ganttScene->addText("idle");
                idleText->setPos(currentTime * 30 - 25, processes.size() * 30 / 2);
                
                // Asegurar que el diagrama sea visible desplazándose
                ganttView->ensureVisible(currentTime * 30, 0, 100, 0);
                return;
            }
        }
    }
    
    // Ejecutar el proceso actual por un ciclo
    currentProcess->burstTime--;
    
    // Dibujar el ciclo actual en el diagrama de Gantt
    int processIndex = processes.indexOf(*currentProcess);
    ganttScene->addRect(
        currentTime * 30, processIndex * 30, 30, 30, 
        QPen(Qt::black), QBrush(processColors[currentProcess->pid])
    );
    
    // Añadir texto del proceso en el diagrama
    QGraphicsTextItem *textItem = ganttScene->addText(currentProcess->pid);
    textItem->setPos(currentTime * 30 + 5, processIndex * 30 + 5);
    
    // Si el proceso ha terminado, registrar su tiempo de finalización
    if (currentProcess->burstTime <= 0) {
        completionTimes[currentProcess->pid] = currentTime + 1;
        int waitTime = completionTimes[currentProcess->pid] - 
                       arrivalSortedProcesses[processIndex].arrivalTime - 
                       arrivalSortedProcesses[processIndex].burstTime;
        waitingTimes[currentProcess->pid] = waitTime;
    }
    
    // Avanzar el tiempo
    currentTime++;
    
    // Desplazar la vista para mostrar el ciclo actual
    ganttView->ensureVisible(currentTime * 30, 0, 100, 0);
}

void PriorityScheduler::drawGanttChart() {
    // Ya se configura en setupGanttChart y se va dibujando dinámicamente
    // en updateSimulation
}

void PriorityScheduler::calculateMetrics() {
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