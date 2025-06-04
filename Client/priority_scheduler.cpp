#include "priority_scheduler.h"
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QScrollBar>
#include <QRandomGenerator>
#include <QColor>
#include <algorithm>
#include <climits>  // Para INT_MAX

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
    processStartTimes.clear();
    totalIdleTime = 0;
    
    // Conectar el timer a la función de actualización de simulación
    connect(simulationTimer, &QTimer::timeout, this, &PriorityScheduler::updateSimulation);
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
        processStartTimes.clear();
        totalIdleTime = 0;
        
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
// ALGORITMO DE ORGANIZACIÓN Y SELECCIÓN DE PROCESOS
//==============================================================================

/**
 * Organiza los procesos disponibles por prioridad
 * Retorna una lista ordenada de procesos listos para ejecutar
 */
QList<Process*> PriorityScheduler::organizeProcessesByPriority() {
    QList<Process*> availableProcesses;
    
    // Recopilar todos los procesos que han llegado y no han terminado
    for (int i = 0; i < processes.size(); i++) {
        if (processes[i].arrivalTime <= currentTime && processes[i].burstTime > 0) {
            availableProcesses.append(&processes[i]);
        }
    }
    
    // Ordenar por prioridad (menor número = mayor prioridad)
    // En caso de empate, usar FCFS (First Come First Served)
    std::sort(availableProcesses.begin(), availableProcesses.end(),
              [](Process* a, Process* b) {
                  if (a->priority != b->priority) {
                      return a->priority < b->priority; // Menor número = mayor prioridad
                  }
                  return a->arrivalTime < b->arrivalTime; // FCFS para empates
              });
    
    return availableProcesses;
}

/**
 * Verifica si vale la pena esperar por procesos de mayor prioridad
 * Retorna true si hay procesos de mayor prioridad llegando pronto
 */
bool PriorityScheduler::shouldWaitForHigherPriority() {
    // Obtener la mejor prioridad actual
    auto currentAvailable = organizeProcessesByPriority();
    if (currentAvailable.isEmpty()) {
        return true; // Esperar si no hay nada disponible ahora
    }
    
    int currentBestPriority = currentAvailable.first()->priority;
    
    // Verificar si hay procesos de mayor prioridad llegando en los próximos ciclos
    for (int futureTime = currentTime + 1; futureTime <= currentTime + 3; futureTime++) {
        for (const Process& p : processes) {
            if (p.arrivalTime == futureTime && 
                p.burstTime > 0 && 
                p.priority < currentBestPriority) {
                return true; // Vale la pena esperar
            }
        }
    }
    
    return false; // No vale la pena esperar
}

/**
 * Selecciona el siguiente proceso a ejecutar
 * Primero organiza los procesos y luego selecciona el de mayor prioridad
 */
Process* PriorityScheduler::getNextProcessByPriority() {
    // Verificar si vale la pena esperar por procesos de mayor prioridad
    if (shouldWaitForHigherPriority()) {
        return nullptr; // Hacer CPU idle para reorganizar
    }
    
    // Organizar procesos disponibles por prioridad
    auto organizedProcesses = organizeProcessesByPriority();
    
    // Retornar el proceso de mayor prioridad (primero en la lista ordenada)
    return organizedProcesses.isEmpty() ? nullptr : organizedProcesses.first();
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
        if (currentProcess == nullptr || 
            highestPriorityProcess->priority < currentProcess->priority ||
            (highestPriorityProcess->priority == currentProcess->priority && 
             highestPriorityProcess->arrivalTime < currentProcess->arrivalTime)) {
            
            if (currentProcess != highestPriorityProcess && 
                !processStartTimes.contains(highestPriorityProcess->pid)) {
                
                int adjustedStartTime = currentTime - totalIdleTime;
                processStartTimes[highestPriorityProcess->pid] = adjustedStartTime;
                adjustedStartTimes[highestPriorityProcess->pid] = adjustedStartTime;  
                
            }
            
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
            // ★ CPU IDLE: Contar tiempo de organización
            totalIdleTime++;
            currentTime++;
            return;
        }
    }

    // EJECUCIÓN: Procesar un ciclo del proceso actual
    currentProcess->burstTime--;

    // VISUALIZACIÓN: Dibujar rectángulo del proceso en el diagrama de Gantt
    int y = 30;
    ganttScene->addRect(
        drawPosition * 30, y, 30, 30,
        QPen(Qt::black), QBrush(processColors[currentProcess->pid])
    );

    // Añadir identificador del proceso en el rectángulo
    QGraphicsTextItem *textItem = ganttScene->addText(currentProcess->pid);
    textItem->setPos(drawPosition * 30 + 5, y + 5);

    // ★ FINALIZACIÓN MEJORADA CON TIEMPO AJUSTADO
    if (currentProcess->burstTime <= 0) {
        // Registrar tiempo de finalización (sin idle time)
        int adjustedCompletionTime = (currentTime + 1) - totalIdleTime;
        completionTimes[currentProcess->pid] = adjustedCompletionTime;

        // Calcular waiting time con tiempos ajustados
        if (processStartTimes.contains(currentProcess->pid)) {
            auto it = std::find_if(arrivalSortedProcesses.begin(), arrivalSortedProcesses.end(),
                                   [&](const Process& p) { return p.pid == currentProcess->pid; });

            if (it != arrivalSortedProcesses.end()) {
                int startTime = processStartTimes[currentProcess->pid];
                int completionTime = adjustedCompletionTime;
                int originalBurstTime = it->burstTime;
                
                int totalTime = completionTime - startTime;
                int waitTime = totalTime - originalBurstTime;
                
                waitingTimes[currentProcess->pid] = waitTime;
  
            }
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
       double totalAdjustedStartTime = 0;
    
    for (const QString& pid : adjustedStartTimes.keys()) {
        int adjustedStartTime = adjustedStartTimes[pid];
        
        
        totalAdjustedStartTime += adjustedStartTime;  // ← SUMAR START TIMES
        
    }
    
    double avgStartTime = totalAdjustedStartTime / processes.size();
    emit simulationFinished(avgStartTime);
}

//==============================================================================
// SIMULACIÓN SIN INTERFAZ GRÁFICA
//==============================================================================

/**
 * Ejecuta simulación completa sin visualización para obtener métricas
 * Implementa la misma lógica de organización que la simulación visual
 */
double PriorityScheduler::simulateWithoutGUI() {
    // Crear copia independiente de los procesos para simulación
    QList<Process> simProcesses = processes;
    
    // Preservar burst times originales para cálculos correctos
    QMap<QString, int> originalBurstTimes;
    for (const Process& p : simProcesses) {
        originalBurstTimes[p.pid] = p.burstTime;
    }
    
    // Variables locales para métricas de simulación
    QMap<QString, int> simWaitingTimes;
    QMap<QString, int> simCompletionTimes;
    QMap<QString, int> simAdjustedStartTimes; 
    
    int simTime = 0;
    int simTotalIdleTime = 0;
    Process* currentSimProcess = nullptr;
    
    // BUCLE PRINCIPAL DE SIMULACIÓN
    while (true) {
        // Organizar procesos disponibles por prioridad
        QList<Process*> availableProcesses;
        for (int i = 0; i < simProcesses.size(); i++) {
            if (simProcesses[i].arrivalTime <= simTime && simProcesses[i].burstTime > 0) {
                availableProcesses.append(&simProcesses[i]);
            }
        }
        
        // Ordenar por prioridad
        std::sort(availableProcesses.begin(), availableProcesses.end(),
                  [](Process* a, Process* b) {
                      if (a->priority != b->priority) {
                          return a->priority < b->priority;
                      }
                      return a->arrivalTime < b->arrivalTime;
                  });
        
        // Verificar si vale la pena esperar
        bool shouldWait = false;
        if (!availableProcesses.isEmpty()) {
            int currentBestPriority = availableProcesses.first()->priority;
            for (int futureTime = simTime + 1; futureTime <= simTime + 3; futureTime++) {
                for (const Process& p : simProcesses) {
                    if (p.arrivalTime == futureTime && 
                        p.burstTime > 0 && 
                        p.priority < currentBestPriority) {
                        shouldWait = true;
                        break;
                    }
                }
                if (shouldWait) break;
            }
        }
        
        // Seleccionar proceso
        Process* highestPriorityProcess = nullptr;
        if (!shouldWait && !availableProcesses.isEmpty()) {
            highestPriorityProcess = availableProcesses.first();
        }
        
        // ★ REGISTRAR ADJUSTED START TIME
        if (highestPriorityProcess != nullptr && 
            !simAdjustedStartTimes.contains(highestPriorityProcess->pid)) {
            
            int adjustedStartTime = simTime - simTotalIdleTime;  // ← CALCULAR ADJUSTED
            simProcessStartTimes[highestPriorityProcess->pid] = simTime;
            simAdjustedStartTimes[highestPriorityProcess->pid] = adjustedStartTime;  // ← GUARDAR
        }
        
        // Asignar proceso seleccionado (algoritmo preemptivo)
        currentSimProcess = highestPriorityProcess;

        // Manejar caso sin procesos disponibles
        if (currentSimProcess == nullptr) {
            // Verificar si aún hay procesos pendientes en el sistema
            bool pendingProcesses = false;
            for (const Process& p : simProcesses) {
                if (p.burstTime > 0) {
                    pendingProcesses = true;
                    break;
                }
            }

            if (!pendingProcesses) {
                // Simulación terminada: todos los procesos completados
                break;
            } else {
                // CPU idle: avanzar tiempo y continuar
                simTotalIdleTime++; 
                simTime++;
                continue;
            }
        }

        // Ejecutar un ciclo del proceso seleccionado
        currentSimProcess->burstTime--;

        // Verificar finalización del proceso
       if (currentSimProcess->burstTime <= 0) {
            int adjustedCompletionTime = (simTime + 1) - simTotalIdleTime;  // ← ADJUSTED COMPLETION
            simCompletionTimes[currentSimProcess->pid] = adjustedCompletionTime;

            if (simAdjustedStartTimes.contains(currentSimProcess->pid)) {
                auto it = std::find_if(arrivalSortedProcesses.begin(), arrivalSortedProcesses.end(),
                                       [&](const Process& p) { return p.pid == currentSimProcess->pid; });

                if (it != arrivalSortedProcesses.end()) {
                    int adjustedStartTime = simAdjustedStartTimes[currentSimProcess->pid];  // ← USAR ADJUSTED
                    int adjustedCompletionTime = simCompletionTimes[currentSimProcess->pid];
                    int originalBurstTime = originalBurstTimes[currentSimProcess->pid];
                    
                    int totalTime = adjustedCompletionTime - adjustedStartTime;
                    int waitTime = totalTime - originalBurstTime;
                    
                    simWaitingTimes[currentSimProcess->pid] = waitTime;
                }
            }
        }

        // Avanzar tiempo de simulación
        simTime++;
    }
    
    // Calcular tiempo de espera promedio final
    double totalWaitingTime = 0;
    for (const QString& pid : simWaitingTimes.keys()) {
        int adjustedStartTime = simAdjustedStartTimes[pid];  
        int waitTime = simWaitingTimes[pid];
    
        totalWaitingTime += adjustedStartTime;  // ← Opción 2: Sumar start times
    
    }
    
    return totalWaitingTime / processes.size();
}