#include "counting_semaphore_scheduler.h"
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QScrollBar>
#include <QRandomGenerator>
#include <QColor>
#include <QFont>
#include <QBrush>
#include <QPen>
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
    ganttView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    ganttScene->clear();
    
    // Calcular dimensiones mejoradas
    int headerHeight = 140;  // Más espacio para contador de semáforo
    int processRowHeight = 50;
    int totalHeight = headerHeight + (processes.size() * processRowHeight) + 100;
    
    ganttScene->setSceneRect(0, 0, 6000, totalHeight);
    
    // === PANEL DE INFORMACIÓN SUPERIOR ===
    setupInformationPanel();
    
    // === LEYENDA DE ESTADOS ===
    setupLegend();
    
    // === FILAS DE PROCESOS ===
    setupProcessRows(headerHeight);
    
    // === LÍNEAS DE TIEMPO ===
    setupTimeGrid(headerHeight, totalHeight - 50);
}

void CountingSemaphoreScheduler::setupInformationPanel() {
    // Fondo del panel de información
    QGraphicsRectItem *infoPanelBg = ganttScene->addRect(0, 0, 6000, 100, 
        QPen(Qt::darkGray, 2), QBrush(QColor(255, 248, 220))); // Color crema
    
    // Título principal
    QGraphicsTextItem *title = ganttScene->addText("🔢 SIMULACIÓN SEMÁFORO DE CONTEO", 
        QFont("Arial", 14, QFont::Bold));
    title->setPos(20, 10);
    title->setDefaultTextColor(QColor(139, 69, 19)); // Marrón
    
    // Tiempo actual
    currentTimeLabel = ganttScene->addText("⏱️ Tiempo: 0", QFont("Arial", 12, QFont::Bold));
    currentTimeLabel->setPos(20, 40);
    currentTimeLabel->setDefaultTextColor(QColor(0, 100, 0));
    
    // Estado del semáforo con contador visual
    semaphoreStatusLabel = ganttScene->addText("🎯 Recursos disponibles: 0/0", QFont("Arial", 12, QFont::Bold));
    semaphoreStatusLabel->setPos(200, 40);
    semaphoreStatusLabel->setDefaultTextColor(QColor(0, 100, 150));
    
    // Cola de bloqueados
    blockedQueueLabel = ganttScene->addText("🚧 Procesos bloqueados: 0", QFont("Arial", 12));
    blockedQueueLabel->setPos(450, 40);
    blockedQueueLabel->setDefaultTextColor(QColor(150, 0, 0));
    
    // Proceso actual
    currentProcessLabel = ganttScene->addText("▶️ Ejecutando: NINGUNO", QFont("Arial", 12));
    currentProcessLabel->setPos(20, 70);
    currentProcessLabel->setDefaultTextColor(QColor(0, 0, 150));
    
    // Indicador visual de recursos (será creado dinámicamente)
    resourceIndicatorLabel = ganttScene->addText("📊 Estado recursos: ", QFont("Arial", 10));
    resourceIndicatorLabel->setPos(250, 70);
    resourceIndicatorLabel->setDefaultTextColor(QColor(100, 100, 100));
}

void CountingSemaphoreScheduler::setupLegend() {
    int legendY = 105;
    
    // Título de leyenda
    QGraphicsTextItem *legendTitle = ganttScene->addText("📋 LEYENDA DE ESTADOS:", 
        QFont("Arial", 10, QFont::Bold));
    legendTitle->setPos(20, legendY);
    legendTitle->setDefaultTextColor(Qt::black);
    
    // Estados con colores y símbolos específicos para semáforo de conteo
    struct LegendItem {
        QColor color;
        QString symbol;
        QString description;
        int x;
    };
    
    QList<LegendItem> legendItems = {
        {QColor(100, 200, 100), "▶️", "EJECUTANDO", 200},
        {QColor(255, 165, 0), "🎯", "USANDO RECURSO", 320},
        {QColor(255, 100, 100), "⏸️", "BLOQUEADO", 470},
        {QColor(200, 200, 200), "💤", "INACTIVO", 580},
        {QColor(150, 150, 255), "⏳", "ESPERANDO", 680}
    };
    
    for (const auto& item : legendItems) {
        // Rectángulo de color
        ganttScene->addRect(item.x, legendY + 3, 20, 15, 
            QPen(Qt::black), QBrush(item.color));
        
        // Texto explicativo
        QGraphicsTextItem *text = ganttScene->addText(
            item.symbol + " " + item.description, QFont("Arial", 8));
        text->setPos(item.x + 25, legendY);
        text->setDefaultTextColor(Qt::black);
    }
}

void CountingSemaphoreScheduler::setupProcessRows(int startY) {
    for (int i = 0; i < processes.size(); i++) {
        int y = startY + (i * 50);
        
        // Línea separadora
        ganttScene->addLine(0, y, 6000, y, QPen(QColor(200, 200, 200)));
        
        // Etiqueta del proceso con fondo
        QGraphicsRectItem *labelBg = ganttScene->addRect(5, y + 5, 80, 30, 
            QPen(Qt::black), QBrush(processColors[processes[i].pid]));
        
        QGraphicsTextItem *processLabel = ganttScene->addText(
            processes[i].pid, QFont("Arial", 10, QFont::Bold));
        processLabel->setPos(10, y + 10);
        processLabel->setDefaultTextColor(Qt::white);
        
        // Información del proceso
        QString processInfo = QString("BT:%1 AT:%2 P:%3")
            .arg(processes[i].burstTime)
            .arg(processes[i].arrivalTime)
            .arg(processes[i].priority);
        
        QGraphicsTextItem *infoLabel = ganttScene->addText(
            processInfo, QFont("Arial", 8));
        infoLabel->setPos(90, y + 15);
        infoLabel->setDefaultTextColor(QColor(80, 80, 80));
    }
}

void CountingSemaphoreScheduler::setupTimeGrid(int startY, int endY) {
    // Líneas verticales y etiquetas de tiempo
    for (int i = 0; i <= 120; i++) {
        int x = 200 + (i * 40);  // Desplazado para dar espacio a etiquetas
        
        // Línea vertical
        QPen timePen = (i % 5 == 0) ? QPen(Qt::darkGray, 2) : QPen(Qt::lightGray, 1);
        ganttScene->addLine(x, startY, x, endY, timePen);
        
        // Etiqueta de tiempo cada 5 unidades
        if (i % 5 == 0) {
            QGraphicsTextItem *timeLabel = ganttScene->addText(
                QString::number(i), QFont("Arial", 10, QFont::Bold));
            timeLabel->setPos(x - 8, endY + 5);
            timeLabel->setDefaultTextColor(Qt::black);
        }
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

void CountingSemaphoreScheduler::assignProcessColors() {
    processColors.clear();
    QList<QColor> predefinedColors = {
        QColor(70, 130, 180),   // Steel Blue
        QColor(220, 20, 60),    // Crimson
        QColor(255, 140, 0),    // Dark Orange
        QColor(50, 205, 50),    // Lime Green
        QColor(138, 43, 226),   // Blue Violet
        QColor(255, 20, 147),   // Deep Pink
        QColor(0, 191, 255),    // Deep Sky Blue
        QColor(255, 165, 0)     // Orange
    };
    
    for (int i = 0; i < processes.size(); i++) {
        processColors[processes[i].pid] = predefinedColors[i % predefinedColors.size()];
    }
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
    
    // Dibujar estados de todos los procesos
    drawAllProcessStates();
    
    // Ejecutar proceso actual
    if (hasCurrentProcess) {
        executeCurrentProcess();
    }
    
    // Actualizar labels informativos
    updateInformationLabels();
    
    // Actualizar indicador visual de recursos
    updateResourceIndicator();
    
    // Verificar si todos los procesos terminaron
    if (checkSimulationComplete()) {
        stopSimulation();
        return;
    }
    
    currentTime++;
    ganttView->ensureVisible((200 + currentTime * 40), 0, 100, 0);
}

void CountingSemaphoreScheduler::drawAllProcessStates() {
    for (int i = 0; i < processes.size(); i++) {
        const Process& p = processes[i];
        int x = 200 + (currentTime * 40);
        int y = 140 + (i * 50) + 10;
        
        ProcessState state = getProcessState(p.pid);
        QColor stateColor = getStateColor(state);
        QString stateSymbol = getStateSymbol(state);
        
        // Rectángulo del estado con borde más grueso
        QGraphicsRectItem *stateRect = ganttScene->addRect(x, y, 35, 25, 
            QPen(Qt::black, 2), QBrush(stateColor));
        
        // Símbolo del estado
        QGraphicsTextItem *symbolText = ganttScene->addText(
            stateSymbol, QFont("Arial", 10, QFont::Bold));
        symbolText->setPos(x + 8, y + 5);
        symbolText->setDefaultTextColor(Qt::black);
        
        // Información adicional para recursos
        if (state == ACCESSING_RESOURCE) {
            // Agregar indicador de número de recurso usado
            QGraphicsTextItem *resourceNum = ganttScene->addText(
                QString::number(getResourcesUsedByProcess(p.pid)), 
                QFont("Arial", 8, QFont::Bold));
            resourceNum->setPos(x + 25, y + 15);
            resourceNum->setDefaultTextColor(Qt::darkRed);
        }
        
        // Tooltip con información detallada
        QString tooltip = QString("Proceso: %1\nEstado: %2\nTiempo: %3\nRecursos en uso: %4")
            .arg(p.pid)
            .arg(getStateName(state))
            .arg(currentTime)
            .arg(getResourcesUsedByProcess(p.pid));
        stateRect->setToolTip(tooltip);
    }
}

CountingSemaphoreScheduler::ProcessState CountingSemaphoreScheduler::getProcessState(const QString& pid) {
    // Si aún no ha llegado
    for (const Process& p : processes) {
        if (p.pid == pid && p.arrivalTime > currentTime) {
            return INACTIVE;
        }
    }
    
    // Si está ejecutándose
    if (hasCurrentProcess && currentProcess.pid == pid) {
        return EXECUTING;
    }
    
    // Si está bloqueado
    QQueue<Process> tempQueue = blockedQueue;
    while (!tempQueue.isEmpty()) {
        if (tempQueue.dequeue().pid == pid) {
            return BLOCKED;
        }
    }
    
    // Si está en cola de listos
    QQueue<Process> readyTemp = readyQueue;
    while (!readyTemp.isEmpty()) {
        if (readyTemp.dequeue().pid == pid) {
            return READY;
        }
    }
    
    // Si está accediendo a recurso
    if (processResourceUsage.contains(pid) && processResourceUsage[pid] > 0) {
        return ACCESSING_RESOURCE;
    }
    
    return INACTIVE;
}

QColor CountingSemaphoreScheduler::getStateColor(ProcessState state) {
    switch (state) {
        case EXECUTING: return QColor(100, 200, 100);         // Verde
        case ACCESSING_RESOURCE: return QColor(255, 165, 0);  // Naranja
        case BLOCKED: return QColor(255, 100, 100);           // Rojo
        case READY: return QColor(150, 150, 255);             // Azul claro
        case INACTIVE: return QColor(200, 200, 200);          // Gris
        default: return Qt::white;
    }
}

QString CountingSemaphoreScheduler::getStateSymbol(ProcessState state) {
    switch (state) {
        case EXECUTING: return "▶️";
        case ACCESSING_RESOURCE: return "🎯";
        case BLOCKED: return "⏸️";
        case READY: return "⏳";
        case INACTIVE: return "💤";
        default: return "?";
    }
}

QString CountingSemaphoreScheduler::getStateName(ProcessState state) {
    switch (state) {
        case EXECUTING: return "Ejecutando";
        case ACCESSING_RESOURCE: return "Usando Recurso";
        case BLOCKED: return "Bloqueado";
        case READY: return "Esperando";
        case INACTIVE: return "Inactivo";
        default: return "Desconocido";
    }
}

void CountingSemaphoreScheduler::updateInformationLabels() {
    // Actualizar tiempo
    currentTimeLabel->setPlainText(QString("⏱️ Tiempo: %1").arg(currentTime));
    
    // Actualizar estado del semáforo con información detallada
    if (!resources.isEmpty()) {
        const Resource& resource = resources.first();
        int currentValue = resourceSemaphores[resource.name].value;
        int totalResources = resource.counter;
        int usedResources = totalResources - std::max(0, currentValue);
        
        QString semStatus = QString("🎯 Recursos: %1/%2 disponibles")
            .arg(std::max(0, currentValue))
            .arg(totalResources);
        
        QColor semColor;
        if (currentValue > 0) {
            semColor = QColor(0, 150, 0);  // Verde si hay recursos
        } else if (currentValue == 0) {
            semColor = QColor(255, 165, 0); // Naranja si todos en uso
        } else {
            semColor = QColor(150, 0, 0);   // Rojo si hay cola de espera
        }
        
        semaphoreStatusLabel->setPlainText(semStatus);
        semaphoreStatusLabel->setDefaultTextColor(semColor);
    }
    
    // Actualizar cola de bloqueados
    blockedQueueLabel->setPlainText(QString("🚧 Procesos bloqueados: %1").arg(blockedQueue.size()));
    
    // Actualizar proceso actual
    QString currentProc = "▶️ Ejecutando: ";
    if (hasCurrentProcess) {
        currentProc += currentProcess.pid;
    } else {
        currentProc += "NINGUNO";
    }
    currentProcessLabel->setPlainText(currentProc);
}

void CountingSemaphoreScheduler::updateResourceIndicator() {
    if (resources.isEmpty()) return;
    
    const Resource& resource = resources.first();
    int currentValue = resourceSemaphores[resource.name].value;
    int totalResources = resource.counter;
    
    // Crear indicador visual con rectángulos
    QString indicator = "📊 ";
    for (int i = 0; i < totalResources; i++) {
        if (i < std::max(0, currentValue)) {
            indicator += "🟢"; // Recurso disponible
        } else {
            indicator += "🔴"; // Recurso en uso
        }
    }
    
    // Agregar información de procesos en cola si hay
    if (currentValue < 0) {
        indicator += QString(" (Cola: %1)").arg(-currentValue);
    }
    
    resourceIndicatorLabel->setPlainText(indicator);
}

int CountingSemaphoreScheduler::getResourcesUsedByProcess(const QString& pid) {
    return processResourceUsage.value(pid, 0);
}

bool CountingSemaphoreScheduler::checkSimulationComplete() {
    bool allDone = true;
    for (const Process& p : processes) {
        if (processExecutionTimes[p.pid] < p.burstTime) {
            allDone = false;
            break;
        }
    }
    
    return allDone && readyQueue.isEmpty() && blockedQueue.isEmpty();
}

void CountingSemaphoreScheduler::processActions() {
    for (const Action& action : actions) {
        if (action.cycle == currentTime) {
            handleSemaphoreOperation(action);
        }
    }
}

void CountingSemaphoreScheduler::handleSemaphoreOperation(const Action& action) {
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
        bool blocked = false;
        P(resourceSemaphores[action.resource], process, blocked);
        
        if (blocked) {
            if (hasCurrentProcess && currentProcess.pid == action.pid) {
                blockedQueue.enqueue(currentProcess);
                hasCurrentProcess = false;
            }
        } else {
            // Proceso obtuvo el recurso
            processResourceUsage[action.pid]++;
        }
        
        // Liberar después de tiempo fijo (mejoraremos esto más adelante)
        QTimer::singleShot(3000, [this, action, process]() {
            bool processReleased = false;
            Process releasedProcess = V(resourceSemaphores[action.resource], processReleased);
            
            // Decrementar uso de recursos
            if (processResourceUsage.contains(action.pid)) {
                processResourceUsage[action.pid]--;
                if (processResourceUsage[action.pid] <= 0) {
                    processResourceUsage.remove(action.pid);
                }
            }
            
            if (processReleased) {
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
    
    if (currentProcessRemainingTime <= 0) {
        hasCurrentProcess = false;
    }
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

void CountingSemaphoreScheduler::initializeResources() {
    resourceSemaphores.clear();
    processResourceUsage.clear();
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
    processResourceUsage.clear();
    
    // Inicializar tiempos de ejecución
    for (const Process& p : processes) {
        processExecutionTimes[p.pid] = 0;
        processStartTimes[p.pid] = -1;
    }
    
    simulationTimer->start(800); // 800ms por ciclo
}

void CountingSemaphoreScheduler::stopSimulation() {
    simulationTimer->stop();
    calculateMetrics();
}

void CountingSemaphoreScheduler::calculateMetrics() {
    double totalExecutionTime = 0;
    for (QString pid : processExecutionTimes.keys()) {
        totalExecutionTime += processExecutionTimes[pid];
    }
    
    double avgExecutionTime = totalExecutionTime / processes.size();
    emit simulationFinished(avgExecutionTime);
}