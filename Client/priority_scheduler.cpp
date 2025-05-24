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


//==============================================================================
// CONTROL DE SIMULACIÓN
//==============================================================================

/**
 * Inicia la simulación del algoritmo de prioridades
 * Resetea todas las variables y comienza la ejecución animada
 */
void PriorityScheduler::startSimulation() {
    if (!simulationRunning && !processes.isEmpty()) {
        // Reinicializar todas las variables de simulación
        currentTime = 0;
        currentProcessIndex = 0;
        currentProcess = nullptr;
        simulationRunning = true;
        drawPosition = 0; // Posición para dibujo sin espacios idle
        
        // Limpiar y reconfigurar la visualización
        ganttScene->clear();
        setupGanttChart(ganttView);
        
        // Limpiar métricas de ejecuciones anteriores
        waitingTimes.clear();
        completionTimes.clear();
        
        // Iniciar timer de animación (500ms por ciclo de CPU)
        simulationTimer->start(500);
    }
}

/**
 * Detiene la simulación y calcula métricas finales
 */
void PriorityScheduler::stopSimulation() {
    simulationTimer->stop();
    simulationRunning = false;
    
    // Calcular y emitir métricas finales
    calculateMetrics();
}

//==============================================================================
// ALGORITMO DE SELECCIÓN DE PROCESOS
//==============================================================================

/**
 * Selecciona el siguiente proceso a ejecutar basado en prioridad
 * Implementa algoritmo de prioridades con anticipación (preemptive)
 * Puntero al proceso con mayor prioridad disponible, o nullptr si no hay
 */
Process* PriorityScheduler::getNextProcessByPriority() {
    Process* next = nullptr;
    int highestPriority = INT_MAX; // Menor número = Mayor prioridad
    int earliestArrival = INT_MAX;
    
    // Buscar entre todos los procesos que han llegado y no han terminado
    for (int i = 0; i < processes.size(); i++) {
        if (processes[i].arrivalTime <= currentTime && processes[i].burstTime > 0) {
            // Verificar si tiene mayor prioridad (valor numérico menor)
            if (processes[i].priority < highestPriority) {
                highestPriority = processes[i].priority;
                earliestArrival = processes[i].arrivalTime;
                next = &processes[i];
            } 
            // En caso de empate en prioridad, aplicar FCFS (First Come First Served)
            else if (processes[i].priority == highestPriority && 
                     processes[i].arrivalTime < earliestArrival) {
                earliestArrival = processes[i].arrivalTime;
                next = &processes[i];
            }
        }
    }
    
    // OPTIMIZACIÓN: Verificar si vale la pena esperar por un proceso de mayor prioridad
    if (next != nullptr) {
        // Buscar procesos que lleguen en el siguiente ciclo con mayor prioridad
        for (int i = 0; i < processes.size(); i++) {
            if (processes[i].arrivalTime == currentTime + 1 && 
                processes[i].burstTime > 0 && 
                processes[i].priority < next->priority) {
                // Es mejor esperar al proceso de mayor prioridad
                return nullptr; // Causa CPU idle por un ciclo
            }
        }
    }
    
    return next;
}


//==============================================================================
// MOTOR DE SIMULACIÓN
//==============================================================================

/**
 * Función principal de actualización de la simulación
 * Se ejecuta cada ciclo del timer para avanzar la simulación
 */
void PriorityScheduler::updateSimulation() {
    // ALGORITMO PREEMPTIVO: Verificar en cada ciclo si hay proceso con mayor prioridad
    Process* highestPriorityProcess = getNextProcessByPriority();
    
    // Determinar si cambiar de proceso (preemption)
    if (highestPriorityProcess != nullptr) {
        // Cambiar proceso si:
        // 1. No hay proceso actual ejecutándose
        // 2. El nuevo proceso tiene mayor prioridad
        // 3. Mismo nivel de prioridad pero llegó antes (FCFS tie-breaking)
        if (currentProcess == nullptr || 
            highestPriorityProcess->priority < currentProcess->priority ||
            (highestPriorityProcess->priority == currentProcess->priority && 
             highestPriorityProcess->arrivalTime < currentProcess->arrivalTime)) {
            currentProcess = highestPriorityProcess;
        }
    }

    // Verificar si no hay procesos listos para ejecutar
    if (currentProcess == nullptr || currentProcess->burstTime <= 0) {
        // Verificar si quedan procesos pendientes en el sistema
        bool pendingProcesses = false;
        for (const Process& p : processes) {
            if (p.burstTime > 0) {
                pendingProcesses = true;
                break;
            }
        }

        if (!pendingProcesses) {
            // Simulación completa: todos los procesos han terminado
            stopSimulation();
            return;
        } else {
            // CPU idle: avanzar tiempo sin dibujar (eliminar idle time visual)
            currentTime++;
            return;
        }
    }

    // EJECUCIÓN: Procesar un ciclo del proceso actual
    currentProcess->burstTime--;

    // VISUALIZACIÓN: Dibujar rectángulo del proceso en el diagrama de Gantt
    int y = 30; // Posición vertical fija
    ganttScene->addRect(
        drawPosition * 30, y, 30, 30,
        QPen(Qt::black), QBrush(processColors[currentProcess->pid])
    );

    // Añadir identificador del proceso en el rectángulo
    QGraphicsTextItem *textItem = ganttScene->addText(currentProcess->pid);
    textItem->setPos(drawPosition * 30 + 5, y + 5);

    // FINALIZACIÓN: Verificar si el proceso ha completado su ejecución
    if (currentProcess->burstTime <= 0) {
        // Registrar tiempo de finalización
        completionTimes[currentProcess->pid] = currentTime + 1;

        // Calcular tiempo de espera usando datos originales del proceso
        auto it = std::find_if(arrivalSortedProcesses.begin(), arrivalSortedProcesses.end(),
                               [&](const Process& p) { return p.pid == currentProcess->pid; });

        if (it != arrivalSortedProcesses.end()) {
            int waitTime = completionTimes[currentProcess->pid] -
                           it->arrivalTime - it->burstTime;
            waitingTimes[currentProcess->pid] = waitTime;
        }
        
        // Liberar proceso actual para seleccionar el siguiente
        currentProcess = nullptr;
    }

    // AVANCE: Incrementar contadores de tiempo
    currentTime++;        // Tiempo lógico de simulación
    drawPosition++;       // Posición de dibujo (sin espacios idle)

    // Actualizar vista para seguir la ejecución actual
    ganttView->ensureVisible(drawPosition * 30, 0, 100, 0);
}

//==============================================================================
// CÁLCULO DE MÉTRICAS
//==============================================================================

/**
 * Calcula las métricas finales de la simulación
 * Calcula tiempo de espera promedio y emite señal de finalización
 */
void PriorityScheduler::calculateMetrics() {
    double totalWaitingTime = 0;
    
    // Sumar todos los tiempos de espera individuales
    for (const QString& pid : waitingTimes.keys()) {
        totalWaitingTime += waitingTimes[pid];
    }
    
    // Calcular promedio
    double avgWaitingTime = totalWaitingTime / processes.size();
    
    // Notificar finalización con resultado
    emit simulationFinished(avgWaitingTime);
}

