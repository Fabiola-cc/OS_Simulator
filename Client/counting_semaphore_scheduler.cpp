#include "counting_semaphore_scheduler.h"
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QScrollBar>
#include <QRandomGenerator>
#include <QColor>
#include <QFont>
#include <QBrush>
#include <QPen>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <algorithm>

CountingSemaphoreScheduler::CountingSemaphoreScheduler(QObject *parent) : QObject(parent) {
    ganttScene = new QGraphicsScene(this);
    simulationTimer = new QTimer(this);
    currentTime = 0;
    sidePanel = nullptr;
    
    connect(simulationTimer, &QTimer::timeout, this, &CountingSemaphoreScheduler::updateSimulation);
}

void CountingSemaphoreScheduler::setupGanttChart(QGraphicsView *view) {
    ganttView = view;
    ganttView->setScene(ganttScene);
    ganttView->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    ganttView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    ganttView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    ganttScene->clear();
    
    // Calcular dimensiones
    int headerHeight = 140;
    int processRowHeight = 50;
    int totalHeight = headerHeight + (processes.size() * processRowHeight) + 100;
    
    ganttScene->setSceneRect(0, 0, 6000, totalHeight);
    
    setupInformationPanel();
    setupLegend();
    setupProcessRows(headerHeight);
    setupTimeGrid(headerHeight, totalHeight - 50);
}

void CountingSemaphoreScheduler::setupSidePanel(QWidget *parent) {
    sidePanel = new QWidget(parent);
    sidePanel->setFixedWidth(350);
    sidePanel->setStyleSheet("QWidget { background-color: #f5f5f5; border: 1px solid #ccc; }");
    
    QVBoxLayout *sidePanelLayout = new QVBoxLayout(sidePanel);
    sidePanelLayout->setSpacing(10);
    sidePanelLayout->setContentsMargins(10, 10, 10, 10);
    
    // Título del panel
    QLabel *panelTitle = new QLabel("📊 INFORMACIÓN EN TIEMPO REAL");
    panelTitle->setStyleSheet("QLabel { font-size: 14px; font-weight: bold; color: #2c3e50; margin-bottom: 10px; }");
    panelTitle->setAlignment(Qt::AlignCenter);
    sidePanelLayout->addWidget(panelTitle);
    
    // Tiempo actual
    currentTimeDisplayLabel = new QLabel("⏱️ Tiempo Actual: 0");
    currentTimeDisplayLabel->setStyleSheet("QLabel { font-size: 12px; font-weight: bold; color: #27ae60; background-color: #ecf0f1; padding: 5px; border-radius: 3px; }");
    sidePanelLayout->addWidget(currentTimeDisplayLabel);
    
    // Estado de semáforos
    QGroupBox *semaphoreGroup = new QGroupBox("🎯 Estado de Semáforos");
    semaphoreGroup->setStyleSheet(
        "QGroupBox { "
        "   font-weight: bold; "
        "   color: #34495e; "
        "   font-size: 12px; "
        "   margin-top: 10px; "
        "} "
        "QGroupBox::title { "
        "   subcontrol-origin: margin; "
        "   subcontrol-position: top left; "
        "   left: 10px; "
        "   padding: 0 8px 0 8px; "
        "   background-color: #f5f5f5; "
        "}"
    );
    QVBoxLayout *semaphoreLayout = new QVBoxLayout(semaphoreGroup);
    semaphoreLayout->setContentsMargins(10, 15, 10, 10);
    
    semaphoreStatusLabel = new QLabel("Iniciando...");
    semaphoreStatusLabel->setStyleSheet(
        "QLabel { "
        "   font-size: 11px; "
        "   color: #2c3e50; "
        "   background-color: white; "
        "   padding: 8px; "
        "   border-radius: 3px; "
        "   border: 1px solid #dee2e6; "
        "}"
    );
    semaphoreStatusLabel->setWordWrap(true);
    semaphoreLayout->addWidget(semaphoreStatusLabel);
    
    sidePanelLayout->addWidget(semaphoreGroup);
    
    // Lista de procesos con widgets expandibles
    QGroupBox *processGroup = new QGroupBox("👥 Procesos y Acciones Pendientes");
    processGroup->setStyleSheet(
        "QGroupBox { "
        "   font-weight: bold; "
        "   color: #34495e; "
        "   font-size: 12px; "
        "   margin-top: 10px; "
        "} "
        "QGroupBox::title { "
        "   subcontrol-origin: margin; "
        "   subcontrol-position: top left; "
        "   left: 10px; "
        "   padding: 0 8px 0 8px; "
        "   background-color: #f5f5f5; "
        "}"
    );
    QVBoxLayout *processGroupLayout = new QVBoxLayout(processGroup);
    processGroupLayout->setContentsMargins(10, 15, 10, 10);
    
    // Área de scroll para los procesos
    processScrollArea = new QScrollArea();
    processScrollArea->setMinimumHeight(180);
    processScrollArea->setMaximumHeight(220);
    processScrollArea->setWidgetResizable(true);
    processScrollArea->setStyleSheet(
        "QScrollArea { "
        "   background-color: white; "
        "   border: 1px solid #bdc3c7; "
        "   border-radius: 3px; "
        "}"
        "QScrollBar:vertical { "
        "   width: 12px; "
        "   background-color: #ecf0f1; "
        "}"
        "QScrollBar::handle:vertical { "
        "   background-color: #bdc3c7; "
        "   border-radius: 6px; "
        "   min-height: 20px; "
        "}"
    );
    
    // Container para todos los procesos
    processContainer = new QWidget();
    processContainerLayout = new QVBoxLayout(processContainer);
    processContainerLayout->setSpacing(3);
    processContainerLayout->setContentsMargins(8, 8, 8, 8);
    
    processScrollArea->setWidget(processContainer);
    processGroupLayout->addWidget(processScrollArea);
    sidePanelLayout->addWidget(processGroup);
    
    // Lista de recursos
    QGroupBox *resourceGroup = new QGroupBox("📦 Estado de Recursos");
    resourceGroup->setStyleSheet(
        "QGroupBox { "
        "   font-weight: bold; "
        "   color: #34495e; "
        "   font-size: 12px; "
        "   margin-top: 10px; "
        "} "
        "QGroupBox::title { "
        "   subcontrol-origin: margin; "
        "   subcontrol-position: top left; "
        "   left: 10px; "
        "   padding: 0 8px 0 8px; "
        "   background-color: #f5f5f5; "
        "}"
    );
    QVBoxLayout *resourceLayout = new QVBoxLayout(resourceGroup);
    resourceLayout->setContentsMargins(10, 15, 10, 10);
    
    resourceListWidget = new QListWidget();
    resourceListWidget->setMinimumHeight(60);
    resourceListWidget->setMaximumHeight(80);
    resourceListWidget->setStyleSheet(
        "QListWidget { "
        "   background-color: white; "
        "   border: 1px solid #bdc3c7; "
        "   border-radius: 3px; "
        "   font-size: 11px; "
        "}"
        "QListWidgetItem { "
        "   padding: 4px; "
        "   margin: 1px; "
        "   border-radius: 2px; "
        "}"
    );
    resourceLayout->addWidget(resourceListWidget);
    sidePanelLayout->addWidget(resourceGroup);
    
    // Espacio flexible al final
    sidePanelLayout->addStretch();
}

void CountingSemaphoreScheduler::createProcessWidgets() {
    // Limpiar widgets existentes
    processWidgets.clear();
    processLabels.clear();
    pendingActionContainers.clear();
    pendingActionLayouts.clear();
    
    // Limpiar el container layout
    while (processContainerLayout->count() > 0) {
        QLayoutItem *item = processContainerLayout->takeAt(0);
        if (item && item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
    
    for (const Process& p : processes) {
        // Widget principal del proceso
        QWidget *processWidget = new QWidget();
        processWidget->setStyleSheet(
            "QWidget { "
            "   background-color: white; "
            "   border: 1px solid #dee2e6; "
            "   border-radius: 4px; "
            "   margin: 2px; "
            "}"
        );
        
        QVBoxLayout *processLayout = new QVBoxLayout(processWidget);
        processLayout->setSpacing(4);
        processLayout->setContentsMargins(8, 6, 8, 6);
        
        // Label principal del proceso con información completa
        QLabel *processLabel = new QLabel();
        QString processInfo = QString("%1 (AT:%2, BT:%3, P:%4)")
            .arg(p.pid)
            .arg(p.arrivalTime)
            .arg(p.burstTime)
            .arg(p.priority);
        processLabel->setText(processInfo);
        processLabel->setStyleSheet(
            "QLabel { "
            "   font-weight: bold; "
            "   padding: 4px; "
            "   border-radius: 3px; "
            "   background-color: #f8f9fa; "
            "   color: #495057; "
            "   font-size: 11px; "
            "}"
        );
        processLayout->addWidget(processLabel);
        
        // Container para acciones pendientes (inicialmente oculto)
        QWidget *pendingContainer = new QWidget();
        pendingContainer->setStyleSheet(
            "QWidget { "
            "   background-color: #f8f9fa; "
            "   border: 1px solid #e9ecef; "
            "   border-radius: 3px; "
            "   margin-left: 5px; "
            "   margin-top: 2px; "
            "}"
        );
        pendingContainer->hide();
        
        QVBoxLayout *pendingLayout = new QVBoxLayout(pendingContainer);
        pendingLayout->setSpacing(2);
        pendingLayout->setContentsMargins(6, 4, 6, 4);
        
        processLayout->addWidget(pendingContainer);
        
        // Agregar al container principal
        processContainerLayout->addWidget(processWidget);
        
        // Guardar referencias
        processWidgets[p.pid] = processWidget;
        processLabels[p.pid] = processLabel;
        pendingActionContainers[p.pid] = pendingContainer;
        pendingActionLayouts[p.pid] = pendingLayout;
    }
    
    // Agregar stretch al final
    processContainerLayout->addStretch();
}

void CountingSemaphoreScheduler::setupInformationPanel() {
    // Fondo del panel de información
    QGraphicsRectItem *infoPanelBg = ganttScene->addRect(0, 0, 6000, 100, 
        QPen(Qt::darkGray, 2), QBrush(QColor(255, 248, 220)));
    
    // Título principal
    QGraphicsTextItem *title = ganttScene->addText("🔢 SIMULACIÓN SEMÁFORO DE CONTEO", 
        QFont("Arial", 14, QFont::Bold));
    title->setPos(20, 10);
    title->setDefaultTextColor(QColor(139, 69, 19));
    
    // Tiempo actual
    currentTimeLabel = ganttScene->addText("⏱️ Tiempo: 0", QFont("Arial", 12, QFont::Bold));
    currentTimeLabel->setPos(20, 40);
    currentTimeLabel->setDefaultTextColor(QColor(0, 100, 0));
    
    // Estado de recursos
    resourceStatusLabel = ganttScene->addText("📊 Estado de recursos", QFont("Arial", 12, QFont::Bold));
    resourceStatusLabel->setPos(200, 40);
    resourceStatusLabel->setDefaultTextColor(QColor(0, 100, 150));
    
    // Acciones pendientes
    pendingActionsLabel = ganttScene->addText("⏳ Acciones pendientes: 0", QFont("Arial", 12));
    pendingActionsLabel->setPos(20, 70);
    pendingActionsLabel->setDefaultTextColor(QColor(150, 0, 150));
}

void CountingSemaphoreScheduler::setupLegend() {
    int legendY = 105;
    
    // Título de leyenda
    QGraphicsTextItem *legendTitle = ganttScene->addText("📋 LEYENDA:", 
        QFont("Arial", 10, QFont::Bold));
    legendTitle->setPos(20, legendY);
    legendTitle->setDefaultTextColor(Qt::black);
    
    // Verde - Acceso
    ganttScene->addRect(150, legendY + 3, 20, 15, QPen(Qt::black), QBrush(Qt::green));
    QGraphicsTextItem *accessText = ganttScene->addText("✅ ACCESO CONCEDIDO", QFont("Arial", 8));
    accessText->setPos(175, legendY);
    accessText->setDefaultTextColor(Qt::black);
    
    // Rojo - Esperando
    ganttScene->addRect(320, legendY + 3, 20, 15, QPen(Qt::black), QBrush(Qt::red));
    QGraphicsTextItem *waitingText = ganttScene->addText("❌ ESPERANDO", QFont("Arial", 8));
    waitingText->setPos(345, legendY);
    waitingText->setDefaultTextColor(Qt::black);
    
    // Gris - Sin acción
    ganttScene->addRect(450, legendY + 3, 20, 15, QPen(Qt::black), QBrush(Qt::lightGray));
    QGraphicsTextItem *noActionText = ganttScene->addText("⚪ SIN ACCIÓN", QFont("Arial", 8));
    noActionText->setPos(475, legendY);
    noActionText->setDefaultTextColor(Qt::black);
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
    }
}

void CountingSemaphoreScheduler::setupTimeGrid(int startY, int endY) {
    // Líneas verticales y etiquetas de tiempo
    for (int i = 0; i <= 120; i++) {
        int x = 200 + (i * 40);
        
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
    assignProcessColors();
    // Los widgets se crearán cuando se configure el panel lateral
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

void CountingSemaphoreScheduler::initializeResources() {
    resourceCounters.clear();
    for (const Resource& r : resources) {
        resourceCounters[r.name] = r.counter; // Contador inicial disponible
    }
}

void CountingSemaphoreScheduler::updateSimulation() {
    // Procesar acciones pendientes y nuevas para este ciclo
    processCurrentCycleActions();
    
    // Dibujar estados de todos los procesos para este ciclo
    drawAllProcessStates();
    
    // Actualizar información
    updateInformationLabels();
    
    // Actualizar panel lateral
    updateSidePanel();
    
    // Verificar si la simulación ha terminado
    if (checkSimulationComplete()) {
        stopSimulation();
        return;
    }
    
    currentTime++;
    ganttView->ensureVisible((200 + currentTime * 40), 0, 100, 0);
}

void CountingSemaphoreScheduler::updateSidePanel() {
    if (!sidePanel) return;
    
    // Actualizar tiempo actual
    currentTimeDisplayLabel->setText(QString("⏱️ Tiempo Actual: %1").arg(currentTime));
    
    // Actualizar estado de semáforos
    QString semaphoreInfo;
    for (const Resource& r : resources) {
        int available = resourceCounters[r.name];
        int total = r.counter;
        int inUse = total - available;
        
        semaphoreInfo += QString("🎯 %1:\n").arg(r.name);
        semaphoreInfo += QString("   • Total: %1\n").arg(total);
        semaphoreInfo += QString("   • Disponibles: %2\n").arg(available);
        semaphoreInfo += QString("   • En uso: %3\n").arg(inUse);
        
        // Indicador visual
        QString indicator = "   • Estado: ";
        for (int i = 0; i < total; i++) {
            if (i < available) {
                indicator += "🟢"; // Disponible
            } else {
                indicator += "🔴"; // En uso
            }
        }
        semaphoreInfo += indicator + "\n\n";
    }
    semaphoreStatusLabel->setText(semaphoreInfo.trimmed());
    
    // Actualizar procesos y sus acciones pendientes
    for (const Process& p : processes) {
        updateProcessPendingActions(p.pid);
    }
    
    // DEBUGGING: Mostrar información de estado actual
    qDebug() << "=== ESTADO EN TIEMPO" << currentTime << "===";
    qDebug() << "Acciones pendientes total:" << pendingActions.size();
    qDebug() << "Acciones en ciclo actual - acceso:" << currentCycleAccess.size();
    qDebug() << "Acciones en ciclo actual - esperando:" << currentCycleWaiting.size();
    
    for (auto it = pendingActions.begin(); it != pendingActions.end(); ++it) {
        qDebug() << "Pendiente:" << it.key() << "->" << it.value().operation << "en" << it.value().resource;
    }
    
    // Actualizar lista de recursos
    resourceListWidget->clear();
    for (const Resource& r : resources) {
        int available = resourceCounters[r.name];
        int total = r.counter;
        QString resourceInfo = QString("%1: %2/%3 disponibles")
            .arg(r.name)
            .arg(available)
            .arg(total);
        
        QListWidgetItem *item = new QListWidgetItem(resourceInfo);
        if (available > 0) {
            item->setBackground(QColor("#d4edda")); // Verde
            item->setForeground(QColor("#155724"));
        } else {
            item->setBackground(QColor("#fff3cd")); // Amarillo
            item->setForeground(QColor("#856404"));
        }
        
        resourceListWidget->addItem(item);
    }
}

void CountingSemaphoreScheduler::updateProcessPendingActions(const QString& pid) {
    if (!processLabels.contains(pid) || !pendingActionContainers.contains(pid)) {
        qDebug() << "ERROR: No se encontraron widgets para" << pid;
        return;
    }
    
    // Actualizar label principal del proceso con estado y colores
    QString state = getProcessCurrentState(pid);
    QString stateSymbol = getProcessStateSymbol(state);
    
    // Obtener información básica del proceso
    QString processInfo = QString("%1 %2").arg(stateSymbol).arg(pid);
    for (const Process& p : processes) {
        if (p.pid == pid) {
            processInfo += QString(" (AT:%1, BT:%2, P:%3) - %4")
                .arg(p.arrivalTime)
                .arg(p.burstTime)
                .arg(p.priority)
                .arg(state);
            break;
        }
    }
    
    QLabel *processLabel = processLabels[pid];
    processLabel->setText(processInfo);
    
    // Aplicar colores según el estado
    QColor stateColor = getProcessStateColor(state);
    QString textColor = stateColor.lightness() > 150 ? "#2c3e50" : "white";
    QString styleSheet = QString(
        "QLabel { "
        "   font-weight: bold; "
        "   padding: 5px; "
        "   border-radius: 3px; "
        "   background-color: %1; "
        "   color: %2; "
        "   font-size: 11px; "
        "   border: 1px solid %3; "
        "}"
    ).arg(stateColor.name()).arg(textColor).arg(stateColor.darker(120).name());
    
    processLabel->setStyleSheet(styleSheet);
    
    // Obtener referencias a los containers
    QWidget *pendingContainer = pendingActionContainers[pid];
    QVBoxLayout *pendingLayout = pendingActionLayouts[pid];
    
    // Limpiar acciones pendientes previas
    while (pendingLayout->count() > 0) {
        QLayoutItem *item = pendingLayout->takeAt(0);
        if (item && item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
    
    // Obtener TODAS las acciones pendientes para este proceso
    QList<Action> processPendingActions;
    
    // Buscar en pendingActions (acciones que están esperando recursos)
    for (auto it = pendingActions.begin(); it != pendingActions.end(); ++it) {
        if (it.key() == pid) {
            processPendingActions.append(it.value());
        }
    }
    
    // También buscar acciones futuras que aún no han llegado su tiempo
    for (const Action& action : actions) {
        if (action.pid == pid && action.cycle > currentTime) {
            processPendingActions.append(action);
        }
    }
    
    qDebug() << "Proceso" << pid << "tiene" << processPendingActions.size() << "acciones pendientes en tiempo" << currentTime;
    
    // FORZAR SIEMPRE MOSTRAR AL MENOS UNA SUBLISTA PARA P2 (TEMPORAL PARA PRUEBAS)
    if (pid == "P2") {
        // Crear una acción ficticia para P2 si no tiene ninguna
        if (processPendingActions.isEmpty()) {
            Action fakeAction;
            fakeAction.pid = "P2";
            fakeAction.operation = "read";
            fakeAction.resource = "R1";
            fakeAction.cycle = currentTime + 1;
            processPendingActions.append(fakeAction);
            qDebug() << "FORZANDO sublista para P2 con acción ficticia";
        }
    }
    
    // Si hay acciones pendientes, mostrar el container y agregar las acciones
    if (!processPendingActions.isEmpty()) {
        qDebug() << "Mostrando container para" << pid << "con" << processPendingActions.size() << "acciones";
        
        for (int i = 0; i < processPendingActions.size(); i++) {
            const Action& action = processPendingActions[i];
            
            // Crear widget para cada acción pendiente
            QWidget *actionWidget = new QWidget();
            actionWidget->setFixedHeight(30);
            actionWidget->setStyleSheet(
                "QWidget { "
                "   background-color: #fff3cd; "
                "   border: 2px solid #ffeaa7; "
                "   border-left: 4px solid #fdcb6e; "
                "   border-radius: 4px; "
                "   margin: 2px; "
                "}"
            );
            
            QHBoxLayout *actionLayout = new QHBoxLayout(actionWidget);
            actionLayout->setContentsMargins(8, 5, 8, 5);
            actionLayout->setSpacing(8);
            
            // Símbolo del estado
            QLabel *symbolLabel = new QLabel("❌");
            symbolLabel->setStyleSheet(
                "QLabel { "
                "   font-weight: bold; "
                "   color: #e17055; "
                "   background: transparent; "
                "   border: none; "
                "   font-size: 14px; "
                "}"
            );
            symbolLabel->setFixedSize(25, 25);
            actionLayout->addWidget(symbolLabel);
            
            // Información de la acción
            QString actionText;
            if (action.cycle <= currentTime) {
                actionText = QString("Esperando %1 (%2)").arg(action.resource).arg(action.operation);
            } else {
                actionText = QString("Programada: %1 en %2 (t=%3)").arg(action.operation).arg(action.resource).arg(action.cycle);
            }
            
            QLabel *actionInfo = new QLabel(actionText);
            actionInfo->setStyleSheet(
                "QLabel { "
                "   color: #8d6e63; "
                "   font-size: 11px; "
                "   background: transparent; "
                "   border: none; "
                "   font-weight: bold; "
                "}"
            );
            actionLayout->addWidget(actionInfo);
            
            actionLayout->addStretch();
            
            pendingLayout->addWidget(actionWidget);
            
            qDebug() << "Agregada sublista para" << pid << ":" << actionText;
        }
        
        // MOSTRAR el container
        pendingContainer->show();
        pendingContainer->setVisible(true);
        pendingContainer->update();
        
        qDebug() << "Container MOSTRADO para" << pid;
    } else {
        // No hay acciones pendientes, ocultar el container
        pendingContainer->hide();
        qDebug() << "Container OCULTO para" << pid << "- sin acciones pendientes";
    }
    
    // Forzar actualización del widget padre
    if (processWidgets.contains(pid)) {
        processWidgets[pid]->update();
        processWidgets[pid]->repaint();
    }
}

QString CountingSemaphoreScheduler::getProcessCurrentState(const QString& pid) {
    if (currentCycleAccess.contains(pid)) {
        Action action = currentCycleAccess[pid];
        return QString("ACCESO a %1").arg(action.resource);
    } else if (currentCycleWaiting.contains(pid)) {
        return "ESPERANDO";
    } else if (pendingActions.contains(pid)) {
        return "EN COLA";
    } else {
        return "INACTIVO";
    }
}

QColor CountingSemaphoreScheduler::getProcessStateColor(const QString& state) {
    if (state.contains("ACCESO")) {
        return QColor("#d4edda"); // Verde claro
    } else if (state.contains("ESPERANDO")) {
        return QColor("#f8d7da"); // Rojo claro
    } else if (state.contains("EN COLA")) {
        return QColor("#fff3cd"); // Amarillo claro
    } else {
        return QColor("#f8f9fa"); // Gris claro
    }
}

QString CountingSemaphoreScheduler::getProcessStateSymbol(const QString& state) {
    if (state.contains("ACCESO")) {
        return "✅";
    } else if (state.contains("ESPERANDO")) {
        return "❌";
    } else if (state.contains("EN COLA")) {
        return "⏳";
    } else {
        return "⚪";
    }
}

void CountingSemaphoreScheduler::processCurrentCycleActions() {
    // Primero, procesar acciones pendientes
    QMap<QString, Action> processedPendingActions;
    
    for (auto it = pendingActions.begin(); it != pendingActions.end(); ) {
        Action pendingAction = it.value();
        QString pid = it.key();
        
        // Verificar si el recurso está disponible ahora
        if (resourceCounters[pendingAction.resource] > 0) {
            // Conceder acceso
            resourceCounters[pendingAction.resource]--;
            currentCycleAccess[pid] = pendingAction;
            processedPendingActions[pid] = pendingAction;
            it = pendingActions.erase(it);
        } else {
            // Sigue esperando
            currentCycleWaiting[pid] = pendingAction;
            ++it;
        }
    }
    
    // Ahora procesar nuevas acciones para este ciclo
    for (const Action& action : actions) {
        if (action.cycle == currentTime) {
            QString pid = action.pid;
            
            // Si ya procesamos una acción pendiente para este proceso
            if (processedPendingActions.contains(pid)) {
                Action pendingProcessed = processedPendingActions[pid];
                
                // Si es el mismo recurso, la nueva acción se vuelve pendiente
                if (pendingProcessed.resource == action.resource) {
                    pendingActions[pid] = action;
                } else {
                    // Diferente recurso, intentar ejecutar la nueva también
                    if (resourceCounters[action.resource] > 0) {
                        resourceCounters[action.resource]--;
                        // Ya no podemos mostrar dos accesos simultáneos en la visualización
                        // Priorizamos la acción pendiente que ya se ejecutó
                    } else {
                        pendingActions[pid] = action;
                    }
                }
            } else {
                // No hay acción pendiente, procesar esta acción
                if (resourceCounters[action.resource] > 0) {
                    // Conceder acceso
                    resourceCounters[action.resource]--;
                    currentCycleAccess[pid] = action;
                } else {
                    // Recurso no disponible, hacer pendiente
                    pendingActions[pid] = action;
                    currentCycleWaiting[pid] = action;
                }
            }
        }
    }
}

void CountingSemaphoreScheduler::drawAllProcessStates() {
    // Limpiar estados del ciclo anterior
    currentCycleAccess.clear();
    currentCycleWaiting.clear();
    
    // Liberar todos los recursos al inicio del ciclo (recursos se liberan automáticamente)
    for (const Resource& r : resources) {
        resourceCounters[r.name] = r.counter;
    }
    
    // Procesar acciones para este ciclo
    processCurrentCycleActions();
    
    // Dibujar estado de cada proceso EN EL GANTT SOLAMENTE
    for (int i = 0; i < processes.size(); i++) {
        const Process& p = processes[i];
        int x = 200 + (currentTime * 40);
        int y = 140 + (i * 50) + 10;
        
        QColor stateColor;
        QString stateText;
        QString tooltip;
        
        if (currentCycleAccess.contains(p.pid)) {
            // Proceso tiene acceso
            Action accessAction = currentCycleAccess[p.pid];
            stateColor = Qt::green;
            stateText = "✅";
            tooltip = QString("Proceso: %1\nAcceso a: %2\nOperación: %3\nTiempo: %4")
                .arg(p.pid)
                .arg(accessAction.resource)
                .arg(accessAction.operation)
                .arg(currentTime);
        } else if (currentCycleWaiting.contains(p.pid)) {
            // Proceso está esperando
            Action waitingAction = currentCycleWaiting[p.pid];
            stateColor = Qt::red;
            stateText = "❌";
            tooltip = QString("Proceso: %1\nEsperando: %2\nOperación: %3\nTiempo: %4")
                .arg(p.pid)
                .arg(waitingAction.resource)
                .arg(waitingAction.operation)
                .arg(currentTime);
        } else {
            // Sin acción
            stateColor = Qt::lightGray;
            stateText = "⚪";
            tooltip = QString("Proceso: %1\nSin acción\nTiempo: %2")
                .arg(p.pid)
                .arg(currentTime);
        }
        
        // Dibujar rectángulo del estado EN EL GANTT
        QGraphicsRectItem *stateRect = ganttScene->addRect(x, y, 35, 25, 
            QPen(Qt::black, 2), QBrush(stateColor));
        stateRect->setToolTip(tooltip);
        
        // Dibujar símbolo del estado EN EL GANTT
        QGraphicsTextItem *symbolText = ganttScene->addText(
            stateText, QFont("Arial", 10, QFont::Bold));
        symbolText->setPos(x + 8, y + 5);
        symbolText->setDefaultTextColor(Qt::black);
    }
    
    // NOTA: Las sublistas se manejan SOLO en el panel lateral, no aquí
}

void CountingSemaphoreScheduler::updateInformationLabels() {
    // Actualizar tiempo
    currentTimeLabel->setPlainText(QString("⏱️ Tiempo: %1").arg(currentTime));
    
    // Actualizar estado de recursos
    QString resourceStatus = "📊 Recursos: ";
    for (const Resource& r : resources) {
        int available = resourceCounters[r.name];
        int total = r.counter;
        resourceStatus += QString("%1(%2/%3) ").arg(r.name).arg(available).arg(total);
    }
    resourceStatusLabel->setPlainText(resourceStatus);
    
    // Actualizar acciones pendientes
    pendingActionsLabel->setPlainText(QString("⏳ Acciones pendientes: %1").arg(pendingActions.size()));
}

bool CountingSemaphoreScheduler::checkSimulationComplete() {
    // Verificar si hay más acciones por procesar
    for (const Action& action : actions) {
        if (action.cycle > currentTime) {
            return false;
        }
    }
    
    // Verificar si hay acciones pendientes
    if (!pendingActions.isEmpty()) {
        return false;
    }
    
    return true;
}

void CountingSemaphoreScheduler::startSimulation() {
    currentTime = 0;
    pendingActions.clear();
    currentCycleAccess.clear();
    currentCycleWaiting.clear();
    
    initializeResources();
    
    // Crear los widgets de procesos si el panel lateral existe
    if (sidePanel && processContainer) {
        createProcessWidgets();
        
        // Simular algunas acciones pendientes para mostrar el funcionamiento
        // Esto es temporal para demostrar las sublistas
        if (!actions.isEmpty()) {
            // Tomar las primeras acciones y simular que están pendientes
            for (int i = 0; i < qMin(3, actions.size()); i++) {
                const Action& action = actions[i];
                // Solo agregar como pendiente si no es para el tiempo actual
                if (action.cycle > 0) {
                    pendingActions[action.pid] = action;
                    qDebug() << "Acción pendiente simulada:" << action.pid << action.operation << action.resource << "en tiempo" << action.cycle;
                }
            }
        }
        
        // Actualizar inmediatamente el panel para mostrar las acciones pendientes
        updateSidePanel();
    }
    
    simulationTimer->start(1000); // 1 segundo por ciclo
}

void CountingSemaphoreScheduler::stopSimulation() {
    simulationTimer->stop();
    calculateMetrics();
}

void CountingSemaphoreScheduler::calculateMetrics() {
    // Calcular métricas simples
    int totalActions = actions.size();
    double avgExecutionTime = currentTime > 0 ? (double)totalActions / currentTime : 0;
    
    emit simulationFinished(avgExecutionTime);
}