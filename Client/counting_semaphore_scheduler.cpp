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
    
    // Calcular dimensiones - AUMENTADO para acomodar sublistas
    int headerHeight = 140;
    int processRowHeight = 80; // Incrementado para sublistas
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
        QString processInfo = QString("%1 | AT:%2 BT:%3 P:%4")
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
    
    // Amarillo - Acción pendiente
    ganttScene->addRect(580, legendY + 3, 20, 15, QPen(Qt::black), QBrush(QColor(255, 255, 0)));
    QGraphicsTextItem *pendingText = ganttScene->addText("⏳ ACCIÓN PENDIENTE", QFont("Arial", 8));
    pendingText->setPos(605, legendY);
    pendingText->setDefaultTextColor(Qt::black);
}

void CountingSemaphoreScheduler::setupProcessRows(int startY) {
    for (int i = 0; i < processes.size(); i++) {
        int y = startY + (i * 80); // Incrementado espacio entre filas
        
        // Línea separadora
        ganttScene->addLine(0, y, 6000, y, QPen(QColor(200, 200, 200)));
        
        // Etiqueta del proceso con fondo Y PROPIEDADES
        QGraphicsRectItem *labelBg = ganttScene->addRect(5, y + 5, 190, 50, // Aumentado el ancho a 190
            QPen(Qt::black), QBrush(processColors[processes[i].pid]));
        
        // Nombre del proceso
        QGraphicsTextItem *processLabel = ganttScene->addText(
            processes[i].pid, QFont("Arial", 12, QFont::Bold));
        processLabel->setPos(10, y + 8);
        processLabel->setDefaultTextColor(Qt::white);
        
        // Propiedades del proceso (AT, BT, Priority)
        QString properties = QString("AT:%1 BT:%2 P:%3")
            .arg(processes[i].arrivalTime)
            .arg(processes[i].burstTime)
            .arg(processes[i].priority);
        
        QGraphicsTextItem *propertiesLabel = ganttScene->addText(
            properties, QFont("Arial", 9));
        propertiesLabel->setPos(10, y + 28);
        propertiesLabel->setDefaultTextColor(Qt::white);
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
    // Avanzar al siguiente ciclo
    currentTime++;
    qDebug() << "\n🔄 Iniciando ciclo" << currentTime;
    
    // Procesar acciones para este nuevo ciclo
    processCurrentCycleActions();
    
    // Dibujar estados de todos los procesos para este ciclo EN EL GANTT
    drawAllProcessStates();
    
    // Actualizar información
    updateInformationLabels();
    
    // Actualizar panel lateral
    updateSidePanel();
    
    // Verificar si la simulación ha terminado
    if (checkSimulationComplete()) {
        qDebug() << "🏁 Simulación completada en ciclo" << currentTime;
        stopSimulation();
        return;
    }
    
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
        return;
    }
    
    // Actualizar label principal del proceso con estado y colores
    QString state = getProcessCurrentState(pid);
    QString stateSymbol = getProcessStateSymbol(state);
    
    // Obtener información básica del proceso CON TODAS LAS PROPIEDADES
    QString processInfo = QString("%1 %2").arg(stateSymbol).arg(pid);
    for (const Process& p : processes) {
        if (p.pid == pid) {
            processInfo = QString("%1 %2 | AT:%3 BT:%4 P:%5 | %6")
                .arg(stateSymbol)
                .arg(p.pid)
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
    
    // Si hay acciones pendientes, mostrar el container y agregar las acciones
    if (!processPendingActions.isEmpty()) {
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
        }
        
        // MOSTRAR el container
        pendingContainer->show();
        pendingContainer->setVisible(true);
        pendingContainer->update();
    } else {
        // No hay acciones pendientes, ocultar el container
        pendingContainer->hide();
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
    qDebug() << "=== Procesando ciclo" << currentTime << "===";
    
    // Limpiar estados del ciclo anterior al inicio
    currentCycleAccess.clear();
    currentCycleWaiting.clear();
    
    // PRIMERO: Liberar recursos de acciones que terminaron
    for (auto it = activeActions.begin(); it != activeActions.end(); ) {
        if (it.value().endTime <= currentTime) {
            // Liberar recurso
            resourceCounters[it.value().resource]++;
            qDebug() << "Liberando recurso" << it.value().resource << "del proceso" << it.key() 
                     << "en tiempo" << currentTime;
            it = activeActions.erase(it);
        } else {
            ++it;
        }
    }
    
    // SEGUNDO: Procesar acciones pendientes (que ya estaban esperando)
    for (auto it = pendingActions.begin(); it != pendingActions.end(); ) {
        Action pendingAction = it.value();
        QString pid = it.key();
        
        qDebug() << "Verificando acción pendiente de" << pid << "para recurso" << pendingAction.resource;
        
        // Verificar si el recurso está disponible ahora
        if (resourceCounters[pendingAction.resource] > 0) {
            // Conceder acceso
            resourceCounters[pendingAction.resource]--;
            currentCycleAccess[pid] = pendingAction;
            
            // Registrar acción activa (durará 1 ciclo)
            ActiveAction activeAction;
            activeAction.pid = pid;
            activeAction.resource = pendingAction.resource;
            activeAction.operation = pendingAction.operation;
            activeAction.startTime = currentTime;
            activeAction.endTime = currentTime + 1; // Acción dura 1 ciclo
            activeActions[pid] = activeAction;
            
            qDebug() << "✅ Concediendo acceso pendiente a" << pid << "para" << pendingAction.resource 
                     << "en tiempo" << currentTime;
            
            it = pendingActions.erase(it);
        } else {
            // Sigue esperando
            currentCycleWaiting[pid] = pendingAction;
            qDebug() << "❌ Proceso" << pid << "sigue esperando" << pendingAction.resource 
                     << "en tiempo" << currentTime << "(disponibles:" << resourceCounters[pendingAction.resource] << ")";
            ++it;
        }
    }
    
    // TERCERO: Procesar nuevas acciones programadas para este ciclo
    for (const Action& action : actions) {
        if (action.cycle == currentTime) {
            QString pid = action.pid;
            
            qDebug() << "Nueva acción programada:" << pid << "solicita" << action.resource << "en ciclo" << currentTime;
            
            // Si ya procesamos una acción para este proceso en este ciclo, hacer pendiente la nueva
            if (currentCycleAccess.contains(pid)) {
                pendingActions[pid] = action;
                qDebug() << "⏳ Proceso" << pid << "ya tiene acceso, poniendo nueva acción como pendiente";
                continue;
            }
            
            // Verificar si el recurso está disponible
            if (resourceCounters[action.resource] > 0) {
                // Conceder acceso
                resourceCounters[action.resource]--;
                currentCycleAccess[pid] = action;
                
                // Registrar acción activa
                ActiveAction activeAction;
                activeAction.pid = pid;
                activeAction.resource = action.resource;
                activeAction.operation = action.operation;
                activeAction.startTime = currentTime;
                activeAction.endTime = currentTime + 1; // Acción dura 1 ciclo
                activeActions[pid] = activeAction;
                
                qDebug() << "✅ Concediendo acceso nuevo a" << pid << "para" << action.resource 
                         << "en tiempo" << currentTime;
            } else {
                // Recurso no disponible, hacer pendiente
                pendingActions[pid] = action;
                currentCycleWaiting[pid] = action;
                qDebug() << "❌ Proceso" << pid << "esperando" << action.resource 
                         << "en tiempo" << currentTime << "(recurso no disponible, disponibles:" 
                         << resourceCounters[action.resource] << ")";
            }
        }
    }
    
    qDebug() << "Estado final del ciclo" << currentTime << ":";
    qDebug() << "- Accesos concedidos:" << currentCycleAccess.size();
    qDebug() << "- Procesos esperando:" << currentCycleWaiting.size();
    qDebug() << "- Acciones pendientes totales:" << pendingActions.size();
    
    for (const Resource& r : resources) {
        qDebug() << "- Recurso" << r.name << ":" << resourceCounters[r.name] << "/" << r.counter << "disponibles";
    }
}


void CountingSemaphoreScheduler::drawAllProcessStates() {
    // NO procesar acciones aquí - ya se procesaron en processCurrentCycleActions()
    
    // Solo dibujar el estado actual de cada proceso EN EL GANTT
    for (int i = 0; i < processes.size(); i++) {
        const Process& p = processes[i];
        int x = 200 + (currentTime * 40);
        int y = 140 + (i * 80) + 10;
        
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
        
        // Dibujar rectángulo del estado principal EN EL GANTT
        QGraphicsRectItem *stateRect = ganttScene->addRect(x, y, 35, 25, 
            QPen(Qt::black, 2), QBrush(stateColor));
        stateRect->setToolTip(tooltip);
        
        // Dibujar símbolo del estado EN EL GANTT
        QGraphicsTextItem *symbolText = ganttScene->addText(
            stateText, QFont("Arial", 10, QFont::Bold));
        symbolText->setPos(x + 8, y + 5);
        symbolText->setDefaultTextColor(Qt::black);
        
        // Dibujar sublistas de acciones pendientes
        drawProcessPendingActionsInGantt(p.pid, x, y + 30);
    }
}

void CountingSemaphoreScheduler::drawProcessPendingActionsInGantt(const QString& pid, int x, int startY) {
    // Obtener todas las acciones pendientes para este proceso
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
            // Limitar a las próximas 3 acciones para no saturar el diagrama
            if (processPendingActions.size() >= 3) break;
        }
    }
    
    // Si hay acciones pendientes, dibujarlas como sublistas
    if (!processPendingActions.isEmpty()) {
        int subListY = startY;
        
        // Dibujar fondo de la sublista
        QGraphicsRectItem *subListBg = ganttScene->addRect(x - 2, subListY - 2, 
            39, (processPendingActions.size() * 15) + 4, 
            QPen(QColor(255, 193, 7), 2), QBrush(QColor(255, 248, 225)));
        subListBg->setToolTip(QString("Acciones pendientes para %1").arg(pid));
        
        // Dibujar cada acción pendiente
        for (int j = 0; j < processPendingActions.size(); j++) {
            const Action& action = processPendingActions[j];
            int actionY = subListY + (j * 15);
            
            QColor actionColor;
            QString actionSymbol;
            QString actionTooltip;
            
            if (action.cycle <= currentTime) {
                // Acción que está esperando recursos
                actionColor = QColor(255, 193, 7); // Amarillo
                actionSymbol = "⏳";
                actionTooltip = QString("Proceso: %1\nEsperando recurso: %2\nOperación: %3")
                    .arg(pid)
                    .arg(action.resource)
                    .arg(action.operation);
            } else {
                // Acción programada para el futuro
                actionColor = QColor(108, 117, 125); // Gris
                actionSymbol = "📅";
                actionTooltip = QString("Proceso: %1\nProgramada para t=%2\nRecurso: %3\nOperación: %4")
                    .arg(pid)
                    .arg(action.cycle)
                    .arg(action.resource)
                    .arg(action.operation);
            }
            
            // Dibujar rectángulo pequeño para la acción pendiente
            QGraphicsRectItem *actionRect = ganttScene->addRect(x, actionY, 35, 12, 
                QPen(Qt::black, 1), QBrush(actionColor));
            actionRect->setToolTip(actionTooltip);
            
            // Dibujar símbolo o texto de la acción
            QGraphicsTextItem *actionText = ganttScene->addText(
                QString("%1:%2").arg(action.resource).arg(action.operation.left(1).toUpper()), 
                QFont("Arial", 7));
            actionText->setPos(x + 2, actionY);
            actionText->setDefaultTextColor(Qt::black);
            actionText->setToolTip(actionTooltip);
        }
    }
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
    activeActions.clear(); // Limpiar acciones activas también
    
    initializeResources();
    
    // Crear los widgets de procesos si el panel lateral existe
    if (sidePanel && processContainer) {
        createProcessWidgets();
        // NO simular acciones pendientes aquí - dejar que el flujo normal las procese
        updateSidePanel();
    }
    
    // Procesar inmediatamente las acciones del ciclo 0 ANTES de iniciar el timer
    processCurrentCycleActions();
    drawAllProcessStates();
    updateInformationLabels();
    updateSidePanel();
    
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