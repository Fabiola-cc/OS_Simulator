#include "priority_scheduler.h"
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QScrollBar>
#include <QRandomGenerator>
#include <QColor>
#include <algorithm>
#include <climits> 

//==============================================================================
// CONSTRUCTOR Y CONFIGURACIÓN INICIAL
//==============================================================================

/**
 * Constructor del programador de prioridades
 * Inicializa todas las variables necesarias para la simulación
 */
PriorityScheduler::PriorityScheduler(QObject *parent) : QObject(parent) {
    // Inicialización de componentes gráficos
    ganttScene = new QGraphicsScene(this);
    simulationTimer = new QTimer(this);
    
    // Inicialización de variables de estado de simulación
    currentTime = 0;
    currentProcessIndex = 0;
    simulationRunning = false;
    currentProcess = nullptr;
    drawPosition = 0; 
    
    // Conectar el timer a la función de actualización de simulación
    connect(simulationTimer, &QTimer::timeout, this, &PriorityScheduler::updateSimulation);
}


//==============================================================================
// CONFIGURACIÓN DE PROCESOS Y COLORES
//==============================================================================

/**
 * Establece la lista de procesos a programar
 * newProcesses Lista de procesos con sus características
 */
void PriorityScheduler::setProcesses(const QList<Process>& newProcesses) {
    processes = newProcesses;
    
    // Crear copia ordenada por tiempo de llegada para referencia
    arrivalSortedProcesses = processes;
    std::sort(arrivalSortedProcesses.begin(), arrivalSortedProcesses.end(), 
              [](const Process& a, const Process& b) {
                  return a.arrivalTime < b.arrivalTime;
              });
    
    // Asignar colores únicos a cada proceso para visualización
    assignProcessColors();
}

/**
 * Asigna colores aleatorios únicos a cada proceso
 * Los colores generados no son demasiado claros para mejor visibilidad
 */
void PriorityScheduler::assignProcessColors() {
    processColors.clear();
    
    for (const Process& p : processes) {
        if (!processColors.contains(p.pid)) {
            QColor color;
            // Generar colores con suficiente contraste (evitar colores muy claros)
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

/**
 * Configura el diagrama de Gantt para la visualización
 * view Vista gráfica donde se mostrará el diagrama
 */
void PriorityScheduler::setupGanttChart(QGraphicsView *view) {
    ganttView = view;
    ganttView->setScene(ganttScene);
    ganttView->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    ganttView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    
    // Limpiar la escena antes de configurar
    ganttScene->clear();
    
    // Configurar dimensiones de la escena (una sola fila horizontal compacta)
    ganttScene->setSceneRect(0, 0, 5000, 100);
    
    // Dibujar línea base horizontal para la visualización
    ganttScene->addLine(0, 30, 5000, 30, QPen(Qt::white));

    // Dibujar líneas verticales de tiempo y etiquetas numéricas
    for (int i = 0; i <= 100; i++) {
        int x = i * 30;
        // Línea vertical para cada unidad de tiempo
        ganttScene->addLine(x, 0, x, 60, QPen(Qt::white));
        // Etiqueta numérica del tiempo
        QGraphicsTextItem *timeLabel = ganttScene->addText(QString::number(i));
        timeLabel->setPos(x - 5, 65);
    }
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

        // Buscar el proceso original en arrivalSortedProcesses para obtener el burst original
        auto it = std::find_if(arrivalSortedProcesses.begin(), arrivalSortedProcesses.end(),
                               [&](const Process& p) { return p.pid == currentProcess->pid; });

        if (it != arrivalSortedProcesses.end()) {
            int waitTime = completionTimes[currentProcess->pid] -
                           it->arrivalTime - it->burstTime;
            waitingTimes[currentProcess->pid] = waitTime;
        }
    }

    // Avanzar el tiempo
    currentTime++;

    // Desplazar la vista para mostrar el ciclo actual
    ganttView->ensureVisible(currentTime * 30, 0, 100, 0);
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