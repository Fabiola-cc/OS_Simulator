#include "counting_semaphore_scheduler.h"
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QScrollBar>
#include <QRandomGenerator>
#include <QColor>
#include <algorithm>

CountingSemaphoreScheduler::CountingSemaphoreScheduler(QObject *parent) : QObject(parent) {
    ganttScene = new QGraphicsScene(this);
    simulationTimer = new QTimer(this);
    currentTime = 0;
    hasCurrentProcess = false;
    currentProcessRemainingTime = 0;
    
    connect(simulationTimer, &QTimer::timeout, this, &CountingSemaphoreScheduler::updateSimulation);
}

void CountingSemaphoreScheduler::setupGanttChart(QGraphicsView *view) {
    ganttView = view;
    ganttView->setScene(ganttScene);
    ganttView->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    ganttView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    
    ganttScene->clear();
    ganttScene->setSceneRect(0, 0, 5000, 200);
    
    // Líneas base para cada proceso
    for (int i = 0; i < processes.size(); i++) {
        int y = 30 + i * 40;
        ganttScene->addLine(0, y, 5000, y, QPen(Qt::white));
        
        // Etiqueta del proceso
        QGraphicsTextItem *processLabel = ganttScene->addText(processes[i].pid);
        processLabel->setPos(-25, y - 15);
    }
    
    // Líneas verticales de tiempo
    for (int i = 0; i <= 100; i++) {
        int x = i * 30;
        ganttScene->addLine(x, 0, x, 200, QPen(Qt::white));
        QGraphicsTextItem *timeLabel = ganttScene->addText(QString::number(i));
        timeLabel->setPos(x - 5, 210);
    }
}

void CountingSemaphoreScheduler::setProcesses(const QList<Process>& newProcesses) {
    processes = newProcesses;
    
    // Ordenar por tiempo de llegada
    std::sort(processes.begin(), processes.end(),
              [](const Process& a, const Process& b) {
                  return a.arrivalTime < b.arrivalTime;
              });
    
    assignProcessColors();
}

void CountingSemaphoreScheduler::setResources(const QList<Resource>& newResources) {
    resources = newResources;
    initializeResources();
}

void CountingSemaphoreScheduler::setActions(const QList<Action>& newActions) {
    actions = newActions;
    
    // Ordenar acciones por ciclo
    std::sort(actions.begin(), actions.end(),
              [](const Action& a, const Action& b) {
                  return a.cycle < b.cycle;
              });
}

void CountingSemaphoreScheduler::assignProcessColors() {
    processColors.clear();
    for (const Process& p : processes) {
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

void CountingSemaphoreScheduler::initializeResources() {
    resourceSemaphores.clear();
    for (const Resource& r : resources) {
        Semaphore sem(r.counter); // Usar el contador del recurso
        resourceSemaphores[r.name] = sem;
    }
}

void CountingSemaphoreScheduler::startSimulation() {
    currentTime = 0;
    hasCurrentProcess = false;
    currentProcessRemainingTime = 0;
    readyQueue.clear();
    blockedQueue.clear();
    processExecutionTimes.clear();
    processStartTimes.clear();
    
    // Inicializar tiempos de ejecución
    for (const Process& p : processes) {
        processExecutionTimes[p.pid] = 0;
        processStartTimes[p.pid] = -1;
    }
    
    simulationTimer->start(800); // Más lento para ver la sincronización
}

void CountingSemaphoreScheduler::stopSimulation() {
    simulationTimer->stop();
    calculateMetrics();
}

void CountingSemaphoreScheduler::updateSimulation() {
    // Agregar procesos que llegan en este tiempo
    for (const Process& p : processes) {
        if (p.arrivalTime == currentTime) {
            readyQueue.enqueue(p);
            if (processStartTimes[p.pid] == -1) {
                processStartTimes[p.pid] = currentTime;
            }
        }
    }
    
    // Procesar acciones de semáforos para este ciclo
    processActions();
    
    // Si no hay proceso ejecutándose, tomar uno de la cola
    if (!hasCurrentProcess && !readyQueue.isEmpty()) {
        currentProcess = readyQueue.dequeue();
        currentProcessRemainingTime = currentProcess.burstTime;
        hasCurrentProcess = true;
    }
    
    // Ejecutar proceso actual
    if (hasCurrentProcess) {
        executeCurrentProcess();
    }
    
    // Verificar si todos los procesos terminaron
    bool allDone = true;
    for (const Process& p : processes) {
        if (processExecutionTimes[p.pid] < p.burstTime) {
            allDone = false;
            break;
        }
    }
    
    if (allDone && readyQueue.isEmpty() && blockedQueue.isEmpty()) {
        stopSimulation();
        return;
    }
    
    currentTime++;
    ganttView->ensureVisible(currentTime * 30, 0, 100, 0);
}

void CountingSemaphoreScheduler::processActions() {
    for (const Action& action : actions) {
        if (action.cycle == currentTime) {
            handleSemaphoreOperation(action);
        }
    }
}

void CountingSemaphoreScheduler::handleSemaphoreOperation(const Action& action) {
    // Encontrar el proceso
    Process process;
    bool found = false;
    for (const Process& p : processes) {
        if (p.pid == action.pid) {
            process = p;
            found = true;
            break;
        }
    }
    
    if (!found) return;
    
    if (action.operation == "READ" || action.operation == "WRITE") {
        // Operación P (solicitar recurso)
        bool blocked = false;
        P(resourceSemaphores[action.resource], process, blocked);
        
        if (blocked) {
            // Mover proceso a cola de bloqueados
            if (hasCurrentProcess && currentProcess.pid == action.pid) {
                blockedQueue.enqueue(currentProcess);
                hasCurrentProcess = false;
            }
            
            // Dibujar estado bloqueado
            int processIndex = -1;
            for (int i = 0; i < processes.size(); i++) {
                if (processes[i].pid == action.pid) {
                    processIndex = i;
                    break;
                }
            }
            
            if (processIndex != -1) {
                int y = 30 + processIndex * 40;
                ganttScene->addRect(currentTime * 30, y, 30, 30,
                                    QPen(Qt::black), QBrush(Qt::red));
                QGraphicsTextItem *textItem = ganttScene->addText("B");
                textItem->setPos(currentTime * 30 + 10, y + 5);
            }
        } else {
            // Proceso puede acceder al recurso
            int processIndex = -1;
            for (int i = 0; i < processes.size(); i++) {
                if (processes[i].pid == action.pid) {
                    processIndex = i;
                    break;
                }
            }
            
            if (processIndex != -1) {
                int y = 30 + processIndex * 40;
                ganttScene->addRect(currentTime * 30, y, 30, 30,
                                    QPen(Qt::black), QBrush(Qt::yellow));
                QGraphicsTextItem *textItem = ganttScene->addText("R");
                textItem->setPos(currentTime * 30 + 10, y + 5);
            }
        }
        
        // Después de cierto tiempo, liberar recurso (operación V)
        QTimer::singleShot(3000, [this, action, process]() {
            bool processReleased = false;
            Process releasedProcess = V(resourceSemaphores[action.resource], processReleased);
            
            if (processReleased) {
                // Mover proceso de bloqueados a ready
                QQueue<Process> tempQueue;
                while (!blockedQueue.isEmpty()) {
                    Process p = blockedQueue.dequeue();
                    if (p.pid == releasedProcess.pid) {
                        readyQueue.enqueue(p);
                    } else {
                        tempQueue.enqueue(p);
                    }
                }
                blockedQueue = tempQueue;
            }
        });
    }
}

void CountingSemaphoreScheduler::executeCurrentProcess() {
    if (!hasCurrentProcess) return;
    
    currentProcessRemainingTime--;
    processExecutionTimes[currentProcess.pid]++;
    
    // Encontrar índice del proceso para dibujar
    int processIndex = -1;
    for (int i = 0; i < processes.size(); i++) {
        if (processes[i].pid == currentProcess.pid) {
            processIndex = i;
            break;
        }
    }
    
    if (processIndex != -1) {
        int y = 30 + processIndex * 40;
        ganttScene->addRect(currentTime * 30, y, 30, 30,
                            QPen(Qt::black), QBrush(processColors[currentProcess.pid]));
        QGraphicsTextItem *textItem = ganttScene->addText(currentProcess.pid);
        textItem->setPos(currentTime * 30 + 5, y + 5);
    }
    
    // Si el proceso terminó su burst time
    if (currentProcessRemainingTime <= 0) {
        hasCurrentProcess = false;
    }
}

void CountingSemaphoreScheduler::calculateMetrics() {
    double totalExecutionTime = 0;
    for (QString pid : processExecutionTimes.keys()) {
        totalExecutionTime += processExecutionTimes[pid];
    }
    
    double avgExecutionTime = totalExecutionTime / processes.size();
    emit simulationFinished(avgExecutionTime);
}