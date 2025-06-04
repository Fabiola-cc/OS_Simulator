#include "mutex_synchronizer.h"
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QScrollBar>
#include <QRandomGenerator>
#include <QColor>
#include <algorithm>
#include <pthread.h>
#include <unistd.h>  
#include <iostream>
#include <QSet>

pthread_mutex_t globalMutex = PTHREAD_MUTEX_INITIALIZER;

struct ThreadArgs {
    Process* process;
    MutexSynchronizer* synchronizer;
};



MutexSynchronizer::MutexSynchronizer(QObject *parent) : QObject(parent) {
    ganttScene = new QGraphicsScene(this);
    simulationTimer = new QTimer(this);
    currentTime = 0;
    hasCurrentProcess = false;
    currentProcessRemainingTime = 0;

    connect(simulationTimer, &QTimer::timeout, this, &MutexSynchronizer::updateSimulation);
}

void MutexSynchronizer::setupGanttChart(QGraphicsView *view) {
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

void MutexSynchronizer::setupInformationPanel() {
    // Fondo del panel de información
    QGraphicsRectItem *infoPanelBg = ganttScene->addRect(0, 0, 6000, 100, 
        QPen(Qt::darkGray, 2), QBrush(QColor(255, 248, 220))); // Color crema
    
    // Título principal
    QGraphicsTextItem *title = ganttScene->addText("SIMULACIÓN MUTEX", 
        QFont("Arial", 14, QFont::Bold));
    title->setPos(20, 10);
    title->setDefaultTextColor(QColor(139, 69, 19)); // Marrón
    
    // Tiempo actual
    currentTimeLabel = ganttScene->addText("Tiempo: 0", QFont("Arial", 12, QFont::Bold));
    currentTimeLabel->setPos(20, 40);
    currentTimeLabel->setDefaultTextColor(QColor(0, 100, 0));
    
    // Estado del semáforo con contador visual
    semaphoreStatusLabel = ganttScene->addText("Recursos disponibles: 0/0", QFont("Arial", 12, QFont::Bold));
    semaphoreStatusLabel->setPos(200, 40);
    semaphoreStatusLabel->setDefaultTextColor(QColor(0, 100, 150));
    
    // Cola de bloqueados
    blockedQueueLabel = ganttScene->addText("Procesos bloqueados: 0", QFont("Arial", 12));
    blockedQueueLabel->setPos(450, 40);
    blockedQueueLabel->setDefaultTextColor(QColor(150, 0, 0));
    
    // Indicador visual de recursos (será creado dinámicamente)
    resourceIndicatorLabel = ganttScene->addText("Estado recursos: ", QFont("Arial", 10));
    resourceIndicatorLabel->setPos(250, 70);
    resourceIndicatorLabel->setDefaultTextColor(QColor(100, 100, 100));

    resourceUsageLabel = ganttScene->addText("", QFont("Courier New", 10));
    resourceUsageLabel->setPos(370, 70);
    resourceUsageLabel->setDefaultTextColor(Qt::darkBlue);


    processLogBackground = ganttScene->addRect(
        700, 10,    
        420, 80,          
        QPen(Qt::darkGray, 1),
        QBrush(QColor(255, 255, 240))
    );

    // Título del log
    QGraphicsTextItem *logTitle = ganttScene->addText(
        "Historial de Procesos", QFont("Arial", 10, QFont::Bold));
    logTitle->setPos(710, 15);
    logTitle->setDefaultTextColor(Qt::black);

    // Área de texto tipo consola (Courier)
    logWidget = new QTextEdit;
    logWidget->setReadOnly(true);
    logWidget->setFont(QFont("Courier New", 8));
    logWidget->setStyleSheet("background-color: #FFFFF0;");

    QGraphicsProxyWidget *proxy = ganttScene->addWidget(logWidget);
    proxy->setPos(710, 35);   
    proxy->resize(400, 25);   

}

void MutexSynchronizer::setupLegend() {
    int legendY = 105;
    
    // Título de leyenda
    QGraphicsTextItem *legendTitle = ganttScene->addText("LEYENDA DE ESTADOS:", 
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
        {QColor(100, 200, 100), "▶️", "ACCESED", 200},
        {QColor(255, 100, 100), "▶️", "WAITING", 320},
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

void MutexSynchronizer::setupProcessRows(int startY) {
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

void MutexSynchronizer::setupTimeGrid(int startY, int endY) {
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


void MutexSynchronizer::setProcesses(const QList<Process>& newProcesses) {
    processes = newProcesses;
    std::sort(processes.begin(), processes.end(), [](const Process& a, const Process& b) {
        return a.arrivalTime < b.arrivalTime;
    });
    assignProcessColors();
}

void MutexSynchronizer::setResources(const QList<Resource>& newResources) {
    resources = newResources;
    initializeResources();
}

void MutexSynchronizer::setActions(const QList<Action>& newActions) {
    actions = newActions;
    std::sort(actions.begin(), actions.end(), [](const Action& a, const Action& b) {
        return a.cycle < b.cycle;
    });
}

void MutexSynchronizer::assignProcessColors() {
    processColors.clear();
    for (const Process& p : processes) {
        QColor color;
        do {
            color = QColor(QRandomGenerator::global()->bounded(50, 200),
                           QRandomGenerator::global()->bounded(50, 200),
                           QRandomGenerator::global()->bounded(50, 200));
        } while (color.lightness() > 200);
        processColors[p.pid] = color;
    }
}

int MutexSynchronizer::processIndexByPid(const QString& pid) const {
    for (int i = 0; i < processes.size(); ++i) {
        if (processes[i].pid == pid) {
            return i;
        }
    }
    return -1; // si no lo encuentra
}


void MutexSynchronizer::initializeResources() {
    resourceMutexes.clear();
    for (const Resource& r : resources) {
        resourceMutexes[r.name] = true; // true = disponible
    }
}


void MutexSynchronizer::startSimulation() {
    readyQueue.clear();
    blockedQueue.clear();
    processExecutionTimes.clear();
    processStartTimes.clear();

    currentTime = 0;

    for (const Process& p : processes) {
        processExecutionTimes[p.pid] = 0;
        processStartTimes[p.pid] = -1;
    }

    simulationTimer->start(800);  // cada 800 ms avanza un ciclo
}

void MutexSynchronizer::drawAccessBar(const QString& pid, int index, int time, QColor color, const QString& operation) {
    int y = 140 + index * 50;
    int x = 200 + time * 40;

    // Dibuja el rectángulo
    ganttScene->addRect(x, y + 5, 40, 30, QPen(Qt::black), QBrush(color));

    // Dibuja el número de recursos utilizados (para ahora, siempre 1)
    QGraphicsTextItem *resourceNum = ganttScene->addText(operation, QFont("Arial", 8, QFont::Bold));
    resourceNum->setPos(x + 25, y + 15);
    resourceNum->setDefaultTextColor(Qt::black);
}


void MutexSynchronizer::updateSimulation() {
    
    // Obtener acciones del ciclo actual
    QList<Action> currentActions;
    for (const Action& action : actions) {
        if (action.cycle == currentTime) {
            currentActions.append(action);
        }
    }

    // 1. Priorizar acciones bloqueadas: tomar solo la primera acción bloqueada por proceso
    QMap<QString, Action> firstBlockedPerProcess;  // pid -> primera acción bloqueada
    QQueue<Action> newBlockedQueue; // nueva cola para bloquear las que no se puedan ejecutar

    while (!blockedQueue.isEmpty()) {
        Action action = blockedQueue.dequeue();
        QString pid = action.pid;

        if (!firstBlockedPerProcess.contains(pid)) {
            firstBlockedPerProcess[pid] = action;
        } else {
            // Más de una acción bloqueada para el mismo proceso, la volvemos a bloquear
            newBlockedQueue.enqueue(action);
        }
    }

    // 2. Ejecutar las acciones bloqueadas priorizadas
    QSet<QString> processesExecutedFromBlocked; // procesos que lograron ejecutar alguna acción bloqueada
    for (auto it = firstBlockedPerProcess.begin(); it != firstBlockedPerProcess.end(); ++it) {
        const Action& action = it.value();
        const QString& pid = action.pid;
        const QString& resource = action.resource;
        const QString& operation = action.operation;
        int index = processIndexByPid(pid);
        if (index == -1) continue;

        if (resourceMutexes.value(resource, true)) {
            resourceMutexes[resource] = false; // recurso tomado
            resourceOwners[resource] = pid;
            drawAccessBar(pid, index, currentTime, QColor(100, 200, 100), operation); // verde éxito

           appendLog(QString("T%1 | %2 ACCEDE %3 -> %4").arg(currentTime).arg(pid).arg(resource).arg(operation));
            processesExecutedFromBlocked.insert(pid);
        } else {
            newBlockedQueue.enqueue(action);
            drawAccessBar(pid, index, currentTime, QColor(255, 100, 100), "WAITING");
            
            appendLog(QString("T%1 | %2 BLOQUEADO esperando %3").arg(currentTime).arg(pid).arg(resource));
        }

    }

    // 3. Procesar las acciones actuales del ciclo
    for (const Action& action : currentActions) {
        const QString& pid = action.pid;
        const QString& resource = action.resource;
        const QString& operation = action.operation;
        int index = processIndexByPid(pid);
        if (index == -1) continue;

        if (processesExecutedFromBlocked.contains(pid)) {
            newBlockedQueue.enqueue(action);
            drawAccessBar(pid, index, currentTime, QColor(255, 100, 100), "WAITING");
            
            appendLog(QString("T%1 | %2 BLOQUEADO por acción anterior").arg(currentTime).arg(pid));
        } else {
            if (resourceMutexes.value(resource, true)) {
                resourceMutexes[resource] = false;
                resourceOwners[resource] = pid;
                drawAccessBar(pid, index, currentTime, QColor(100, 200, 100), operation);
                
                appendLog(QString("T%1 | %2 ACCEDE %3 -> %4").arg(currentTime).arg(pid).arg(resource).arg(operation));
            } else {
                newBlockedQueue.enqueue(action);
                drawAccessBar(pid, index, currentTime, QColor(255, 100, 100), "WAITING");
                
                appendLog(QString("T%1 | %2 BLOQUEADO esperando %3").arg(currentTime).arg(pid).arg(resource));
            }
        }

    }

    // 4. Actualizar la cola de bloqueados con la nueva
    blockedQueue = newBlockedQueue;

    QStringList resourceStatusList;

    for (auto it = resourceMutexes.begin(); it != resourceMutexes.end(); ++it) {
        const QString& resource = it.key();
        bool available = it.value();

        QString status;
        if (available) {
            status = "LIBRE";
        } else {
            QString user = resourceOwners.value(resource, "?");
            status = QString("→%1").arg(user);
        }

        resourceStatusList << QString("%1:%2").arg(resource).arg(status);
    }

    int totalResources = resourceMutexes.size();
    int availableResources = 0;
    for (auto available : resourceMutexes) {
        if (available) availableResources++;
    }
    semaphoreStatusLabel->setPlainText(
        QString("Recursos disponibles: %1/%2").arg(availableResources).arg(totalResources)
    );

    // 5. Liberar todos los recursos para el siguiente ciclo
    for (auto& keyValue : resourceMutexes) {
        keyValue = true;
    }

    

    // 6. Actualizar etiqueta de procesos bloqueados (únicos)
    QSet<QString> blockedPids;
    for (const Action& a : blockedQueue) {
        blockedPids.insert(a.pid);
    }
    blockedQueueLabel->setPlainText(
        QString("Procesos bloqueados: %1").arg(blockedPids.size())
    );

    
    resourceUsageLabel->setPlainText(resourceStatusList.join(" | "));

    // 7. Incrementar tiempo y actualizar etiqueta
    currentTime++;
    appendLog("-----------------------");
    currentTimeLabel->setPlainText(QString("Tiempo: %1").arg(currentTime));

    bool noMoreActions = true;
    for (const Action& action : actions) {
        if (action.cycle >= currentTime) {
            noMoreActions = false;
            break;
        }
    }

    bool noBlockedProcesses = blockedQueue.isEmpty();

    if (noMoreActions && noBlockedProcesses) {
        stopSimulation();
        currentTimeLabel->setPlainText(currentTimeLabel->toPlainText() + " END");
        semaphoreStatusLabel->setPlainText(
        QString("Recursos disponibles: %1/%2").arg(totalResources).arg(totalResources));
        appendLog("---- SIMULACIÓN FINALIZADA ----");
    }
   
}

void MutexSynchronizer::appendLog(const QString& line) {
    logWidget->append(line);
    logWidget->verticalScrollBar()->setValue(logWidget->verticalScrollBar()->maximum());
}



void MutexSynchronizer::stopSimulation() {
    simulationTimer->stop();
}


