#include "client.h"
#include <QApplication>
#include <QPushButton>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QGraphicsView>
#include <QDebug>

SimulatorClient::SimulatorClient(QWidget *parent) : QWidget(parent) {
    resize(800, 600);
    setWindowTitle("Simulator");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    mainLayout->setAlignment(Qt::AlignTop);
    mainLayout->setContentsMargins(50, 30, 50, 30);
    mainLayout->setSpacing(20);

    // Elementos y diseño de pantalla principal
    welcomeLabel = new QLabel("¡Bienvenido!", this);
    chooseLabel = new QLabel("Escoge qué tipo de simulación quieres hacer", this);
    welcomeLabel->setAlignment(Qt::AlignCenter);
    chooseLabel->setAlignment(Qt::AlignCenter);

    scheduleButton = new QPushButton("Calendarización", this);
    syncButton = new QPushButton("Sincronización", this);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(scheduleButton);
    buttonLayout->addWidget(syncButton);
    buttonLayout->addStretch();

    mainLayout->addWidget(welcomeLabel);
    mainLayout->addWidget(chooseLabel);
    mainLayout->addLayout(buttonLayout);

    ///////////////////////////////////////////////////////////////////

    // Elementos y diseño de pantalla de CALENDARIZACIÓN
    scheduleOptionsWidget = new QWidget(this);
    QVBoxLayout *scheduleLayout = new QVBoxLayout(scheduleOptionsWidget);

    titleLabel1 = new QLabel("Calendarización", this);
    instrLabel1 = new QLabel("Escoge 1 o varios tipos de calendarización para trabajar", this);
    titleLabel1->setAlignment(Qt::AlignCenter);
    instrLabel1->setAlignment(Qt::AlignCenter);

    fcfsCheckBox = new QCheckBox("First In First Out", this);
    sjfCheckBox = new QCheckBox("Shortest Job First", this);
    srtCheckBox = new QCheckBox("Shortest Remaining Time", this);
    rrCheckBox = new QCheckBox("Round Robin", this);
    priorityCheckBox = new QCheckBox("Priority", this);

    quantumLabel = new QLabel("Quantum:", this);
    quantumInput = new QLineEdit(this);
    quantumLabel->setVisible(false);
    quantumInput->setVisible(false);

    connect(rrCheckBox, &QCheckBox::toggled, [=](bool checked){
        quantumLabel->setVisible(checked);
        quantumInput->setVisible(checked);
    });

    addFileButton = new QPushButton("Añadir Archivo", this);
    schedulingSimButton = new QPushButton("Iniciar Simulación", this);
    returnButton = new QPushButton("Regresar", this);

    scheduleLayout->addWidget(titleLabel1);
    scheduleLayout->addWidget(instrLabel1);
    scheduleLayout->addWidget(fcfsCheckBox);
    scheduleLayout->addWidget(sjfCheckBox);
    scheduleLayout->addWidget(srtCheckBox);
    scheduleLayout->addWidget(rrCheckBox);
    scheduleLayout->addWidget(priorityCheckBox);

    QHBoxLayout *quantumLayout = new QHBoxLayout();
    quantumLayout->addWidget(quantumLabel);
    quantumLayout->addWidget(quantumInput);
    scheduleLayout->addLayout(quantumLayout);

    QHBoxLayout *buttonLayout2 = new QHBoxLayout();
    buttonLayout2->addStretch();
    buttonLayout2->addWidget(addFileButton);
    buttonLayout2->addWidget(schedulingSimButton);
    buttonLayout2->addWidget(returnButton);
    buttonLayout2->addStretch();
    scheduleLayout->addLayout(buttonLayout2);

    openFileLabel = new QLabel(this);
    openFileLabel->setAlignment(Qt::AlignCenter);
    openFileLabel->hide();
    scheduleLayout->addWidget(openFileLabel);

    mainLayout->addWidget(scheduleOptionsWidget);
    scheduleOptionsWidget->hide();

    ///////////////////////////////////////////////////////////////////

    // Elementos y diseño de pantalla de SINCRONIZACIÓN
    syncOptionsWidget = new QWidget(this);
    QVBoxLayout *syncLayout = new QVBoxLayout(syncOptionsWidget);

    titleLabel2 = new QLabel("<b>Sincronización<b/>", this);
    instrLabel2 = new QLabel("Escoge el tipo de sincronización para trabajar", this);
    titleLabel2->setAlignment(Qt::AlignCenter);
    instrLabel2->setAlignment(Qt::AlignCenter);

    mutexButton = new QPushButton("Mutex Lock", this);
    semaphoreButton = new QPushButton("Semaphore", this);
    returnButton2 = new QPushButton("Regresar", this);
    
    syncLayout->addWidget(titleLabel2);
    syncLayout->addWidget(instrLabel2);
    
    QHBoxLayout *buttonLayout3 = new QHBoxLayout();
    buttonLayout3->addStretch();
    buttonLayout3->addWidget(mutexButton);
    buttonLayout3->addWidget(semaphoreButton);
    buttonLayout3->addStretch();

    syncLayout->addLayout(buttonLayout3);
    syncLayout->addWidget(returnButton2);

    mainLayout->addWidget(syncOptionsWidget);
    syncOptionsWidget->hide();

    ///////////////////////////////////////////////////////////////////
    // Página Semaphore 
    semaphoreWidget = new QWidget(this);
    QVBoxLayout *semaphoreLayout = new QVBoxLayout(semaphoreWidget);
    

    QLabel *semaphoreTitle = new QLabel("Simulación con Semaphore", this);
    semaphoreTitle->setAlignment(Qt::AlignCenter);
    QFont semaphoreTitleFont = semaphoreTitle->font();
    semaphoreTitleFont.setBold(true);
    semaphoreTitleFont.setPointSize(14);
    semaphoreTitle->setFont(semaphoreTitleFont);
    semaphoreLayout->addWidget(semaphoreTitle);

    syncProcessStatusLabel = new QLabel("Procesos: No cargado", this);
    syncResourceStatusLabel = new QLabel("Recursos: No cargado", this);
    syncActionStatusLabel = new QLabel("Acciones: No cargado", this);
    QPushButton *loadProcessesBtn = new QPushButton("Cargar archivo de Procesos (.txt)", this);
    QPushButton *loadResourcesBtn = new QPushButton("Cargar archivo de Recursos (.txt)", this);
    QPushButton *loadActionsBtn = new QPushButton("Cargar archivo de Acciones (.txt)", this);

    semaphoreLayout->addWidget(syncProcessStatusLabel);
    semaphoreLayout->addWidget(syncResourceStatusLabel);
    semaphoreLayout->addWidget(syncActionStatusLabel);

    semaphoreLayout->addWidget(loadProcessesBtn);
    semaphoreLayout->addWidget(loadResourcesBtn);
    semaphoreLayout->addWidget(loadActionsBtn);

    connect(loadProcessesBtn, &QPushButton::clicked, this, &SimulatorClient::onLoadSyncProcessesClicked);
    connect(loadResourcesBtn, &QPushButton::clicked, this, &SimulatorClient::onLoadSyncResourcesClicked);
    connect(loadActionsBtn, &QPushButton::clicked, this, &SimulatorClient::onLoadSyncActionsClicked);

    QHBoxLayout *buttonLayout_4 = new QHBoxLayout();

    QPushButton *startSemSimBtn = new QPushButton("Iniciar Simulación Semaforo Binario", this);
    QPushButton *startSemSimBtn_2 = new QPushButton("Iniciar Simulación Semaforo de Conteo", this);

    // Conectar los botones a los nuevos métodos
    connect(startSemSimBtn, &QPushButton::clicked, this, &SimulatorClient::onBinarySemaphoreSimClicked);
    connect(startSemSimBtn_2, &QPushButton::clicked, this, &SimulatorClient::onCountingSemaphoreSimClicked);

    buttonLayout_4->addWidget(startSemSimBtn);
    buttonLayout_4->addWidget(startSemSimBtn_2);

    semaphoreLayout->addLayout(buttonLayout_4);
    
    QPushButton *semBackButton = new QPushButton("Regresar", this);
    semaphoreLayout->addWidget(semBackButton);

    connect(semBackButton, &QPushButton::clicked, [=]() {
        semaphoreWidget->hide();
        syncOptionsWidget->show();
    });

    mainLayout->addWidget(semaphoreWidget);
    semaphoreWidget->hide();
    
    /////////////////////////////////////////////// Página Mutex

    mutexWidget = new QWidget(this);
    QVBoxLayout *mutexLayout = new QVBoxLayout(mutexWidget);

    QLabel *mutexTitle = new QLabel("Simulación con Mutex", this);
    mutexTitle->setAlignment(Qt::AlignCenter);
    QFont titleFont = mutexTitle->font();
    titleFont.setBold(true);
    titleFont.setPointSize(14);
    mutexTitle->setFont(titleFont);
    mutexLayout->addWidget(mutexTitle);

    syncProcessStatusLabel2 = new QLabel("Procesos: No cargado", this);
    syncResourceStatusLabel2 = new QLabel("Recursos: No cargado", this);
    syncActionStatusLabel2 = new QLabel("Acciones: No cargado", this);
    
    QPushButton *loadProcessesBtn2 = new QPushButton("Cargar archivo de Procesos (.txt)", this);
    QPushButton *loadResourcesBtn2 = new QPushButton("Cargar archivo de Recursos (.txt)", this);
    QPushButton *loadActionsBtn2 = new QPushButton("Cargar archivo de Acciones (.txt)", this);

    mutexLayout->addWidget(syncProcessStatusLabel2);
    mutexLayout->addWidget(syncResourceStatusLabel2);
    mutexLayout->addWidget(syncActionStatusLabel2);

    mutexLayout->addWidget(loadProcessesBtn2);
    mutexLayout->addWidget(loadResourcesBtn2);
    mutexLayout->addWidget(loadActionsBtn2);

    connect(loadProcessesBtn2, &QPushButton::clicked, this, &SimulatorClient::onLoadSyncProcessesClicked);
    connect(loadResourcesBtn2, &QPushButton::clicked, this, &SimulatorClient::onLoadSyncResourcesClicked);
    connect(loadActionsBtn2, &QPushButton::clicked, this, &SimulatorClient::onLoadSyncActionsClicked);

    QPushButton *startSimMut = new QPushButton("Iniciar Simulación con Mutex", this);

    QPushButton *mutBackButton = new QPushButton("Regresar", this);
    mutexLayout->addWidget(startSimMut);
    mutexLayout->addWidget(mutBackButton);

    connect(mutBackButton, &QPushButton::clicked, [=]() {
        mutexWidget->hide();
        syncOptionsWidget->show();
    });

    mainLayout->addWidget(mutexWidget);
    mutexWidget->hide();

    connect(startSimMut, &QPushButton::clicked, this, &SimulatorClient::onMutexSimClicked);
    
    ///////////////////////////////////////////////////////////////////

    // Widget para la simulación con diagrama de Gantt
    simulationWidget = new QWidget(this);
    setupSimulationWidget();
    mainLayout->addWidget(simulationWidget);
    simulationWidget->hide();
    
    ///////////////////////////////////////////////////////////////////

    // Widget para simulación de semáforos
    semaphoreSimulationWidget = new QWidget(this);
    setupSemaphoreSimulationWidget();
    mainLayout->addWidget(semaphoreSimulationWidget);
    semaphoreSimulationWidget->hide();

    ///////////////////////////////////////////////////////////////////

    // Widget para simulación de semáforos
    mutexSimulationWidget = new QWidget(this);
    setupMutexSimulationWidget();
    mainLayout->addWidget(mutexSimulationWidget);
    mutexSimulationWidget->hide();

    ///////////////////////////////////////////////////////////////////

    // Widget para obtener las métricas de varios tipos de calendarización
    scheduleMetricsWidget = new QWidget(this);

    QVBoxLayout *scheduleMetricsLayout = new QVBoxLayout(scheduleMetricsWidget);
    
    QLabel *sMetricsTitle = new QLabel("Métricas de Calendarización", this);
    sMetricsTitle->setAlignment(Qt::AlignCenter);
    scheduleMetricsLayout->addWidget(sMetricsTitle);

    fifoTitleLabel = new QLabel("<b>First In First Out</b>", this);
    fifoMetricsLabel = new QLabel(this);
    sjfTitleLabel = new QLabel("<b>Shortest Job First</b>", this);
    sjfMetricsLabel = new QLabel(this);
    srtTitleLabel = new QLabel("<b>Shortest Remaining Time</b>", this);
    srtMetricsLabel = new QLabel(this);
    rrTitleLabel = new QLabel("<b>Round Robin</b>", this);
    rrMetricsLabel = new QLabel(this);
    priTitleLabel = new QLabel("<b>Priority</b>", this);
    priMetricsLabel = new QLabel("", this);

    scheduleMetricsLayout->addWidget(fifoTitleLabel);
    scheduleMetricsLayout->addWidget(fifoMetricsLabel);
    scheduleMetricsLayout->addWidget(sjfTitleLabel);
    scheduleMetricsLayout->addWidget(sjfMetricsLabel);
    scheduleMetricsLayout->addWidget(srtTitleLabel);
    scheduleMetricsLayout->addWidget(srtMetricsLabel);
    scheduleMetricsLayout->addWidget(rrTitleLabel);
    scheduleMetricsLayout->addWidget(rrMetricsLabel);
    scheduleMetricsLayout->addWidget(priTitleLabel);
    scheduleMetricsLayout->addWidget(priMetricsLabel);

    QPushButton *backButton = new QPushButton("Regresar al menú principal", this);
    scheduleMetricsLayout->addWidget(backButton);

    connect(backButton, &QPushButton::clicked, [=]() {
        scheduleMetricsWidget->hide();
        welcomeLabel->show();
        chooseLabel->show();
        scheduleButton->show();
        syncButton->show();
    });

    mainLayout->addWidget(scheduleMetricsWidget);
    scheduleMetricsWidget->hide();
    
    ///////////////////////////////////////////////////////////////////
    
    setLayout(mainLayout);

    // Configuración de conexiones entre señales y slots
    connect(scheduleButton, &QPushButton::clicked, this, &SimulatorClient::onScheduleClicked);
    connect(schedulingSimButton, &QPushButton::clicked, this, &SimulatorClient::onSchedulingSimClicked);
    connect(returnButton, &QPushButton::clicked, this, &SimulatorClient::onReturnClicked);
    connect(addFileButton, &QPushButton::clicked, this, &SimulatorClient::onAddFileClicked_Process);
    
    connect(syncButton, &QPushButton::clicked, this, &SimulatorClient::onSyncClicked);   
    connect(returnButton2, &QPushButton::clicked, this, &SimulatorClient::onReturnClicked);
    connect(semaphoreButton, &QPushButton::clicked, this, &SimulatorClient::OnSemaphoreClicked);
    connect(mutexButton, &QPushButton::clicked, this, &SimulatorClient::OnMutexClicked);
    
    // Inicializar schedulers de calendarización
    fifoScheduler = new FiFoScheduler(this);
    connect(fifoScheduler, &FiFoScheduler::simulationFinished, 
            this, &SimulatorClient::onSimulationFinished);

    sjfScheduler = new ShortestJobFirstScheduler(this);
    connect(sjfScheduler, &ShortestJobFirstScheduler::simulationFinished, 
            this, &SimulatorClient::onSimulationFinished);

    srtScheduler = new ShortestRemainingTimeScheduler(this);
    connect(srtScheduler, &ShortestRemainingTimeScheduler::simulationFinished, 
            this, &SimulatorClient::onSimulationFinished);

    rrScheduler = new RoundRobinScheduler(this);
    connect(rrScheduler, &RoundRobinScheduler::simulationFinished, 
            this, &SimulatorClient::onSimulationFinished);

    priorityScheduler = new PriorityScheduler(this);
    connect(priorityScheduler, &PriorityScheduler::simulationFinished, 
            this, &SimulatorClient::onSimulationFinished);
    
    // Inicializar schedulers de semáforos
    binarySemaphoreScheduler = new BinarySemaphoreScheduler(this);
    connect(binarySemaphoreScheduler, &BinarySemaphoreScheduler::simulationFinished,
            this, &SimulatorClient::onSemaphoreSimulationFinished);

    countingSemaphoreScheduler = new CountingSemaphoreScheduler(this);
    connect(countingSemaphoreScheduler, &CountingSemaphoreScheduler::simulationFinished,
            this, &SimulatorClient::onSemaphoreSimulationFinished);

    mutexSynchronizer = new MutexSynchronizer(this);
    connect(mutexSynchronizer, &MutexSynchronizer::simulationFinished,
            this, &SimulatorClient::onMutexSimulationFinished);
}

void SimulatorClient::OnSemaphoreClicked() {
    syncOptionsWidget->hide();
    semaphoreWidget->show();
}

void SimulatorClient::onLoadSyncProcessesClicked() {
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Seleccionar archivo de procesos para sincronización",
        "",
        "Archivos de texto (*.txt);;Todos los archivos (*)"
    );

    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            syncProcessList.clear();
            
            QTextStream in(&file);
            int processCount = 0;
            
            while (!in.atEnd()) {
                QString line = in.readLine();
                QStringList fields = line.split(",");

                if (fields.size() == 4) {
                    Process p;
                    p.pid = fields[0].trimmed();
                    p.burstTime = fields[1].trimmed().toInt();
                    p.arrivalTime = fields[2].trimmed().toInt();
                    p.priority = fields[3].trimmed().toInt();

                    syncProcessList.append(p);
                    processCount++;
                }
            }
            file.close();
            
            syncProcessStatusLabel->setText(QString("Procesos: %1 cargados").arg(processCount));
            syncProcessStatusLabel2->setText(QString("Procesos: %1 cargados").arg(processCount));
            
            QMessageBox::information(this, "Archivo cargado", 
                QString("Se han cargado %1 procesos desde el archivo.").arg(processCount));
        } else {
            QMessageBox::warning(this, "Error", "No se pudo abrir el archivo.");
        }
    }
}

void SimulatorClient::onLoadSyncResourcesClicked() {
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Seleccionar archivo de recursos para sincronización",
        "",
        "Archivos de texto (*.txt);;Todos los archivos (*)"
    );

    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            syncResourceList.clear();
            
            QTextStream in(&file);
            int resourceCount = 0;
            
            while (!in.atEnd()) {
                QString line = in.readLine();
                QStringList fields = line.split(",");

                if (fields.size() == 2) {
                    Resource r;
                    r.name = fields[0].trimmed();
                    r.counter = fields[1].trimmed().toInt();

                    syncResourceList.append(r);
                    resourceCount++;
                }
            }
            file.close();
            
            syncResourceStatusLabel->setText(QString("Recursos: %1 cargados").arg(resourceCount));
            syncResourceStatusLabel2->setText(QString("Recursos: %1 cargados").arg(resourceCount));
            
            QMessageBox::information(this, "Archivo cargado", 
                QString("Se han cargado %1 recursos desde el archivo.").arg(resourceCount));
        } else {
            QMessageBox::warning(this, "Error", "No se pudo abrir el archivo.");
        }
    }
}

void SimulatorClient::onLoadSyncActionsClicked() {
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Seleccionar archivo de acciones para sincronización",
        "",
        "Archivos de texto (*.txt);;Todos los archivos (*)"
    );

    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            syncActionList.clear();
            
            QTextStream in(&file);
            int actionCount = 0;
            
            while (!in.atEnd()) {
                QString line = in.readLine();
                QStringList fields = line.split(",");

                if (fields.size() == 4) {
                    Action a;
                    a.pid = fields[0].trimmed();
                    a.operation = fields[1].trimmed();
                    a.resource = fields[2].trimmed();
                    a.cycle = fields[3].trimmed().toInt();

                    syncActionList.append(a);
                    actionCount++;
                }
            }
            file.close();
            
            syncActionStatusLabel->setText(QString("Acciones: %1 cargadas").arg(actionCount));
            syncActionStatusLabel2->setText(QString("Acciones: %1 cargadas").arg(actionCount));
            
            QMessageBox::information(this, "Archivo cargado", 
                QString("Se han cargado %1 acciones desde el archivo.").arg(actionCount));
        } else {
            QMessageBox::warning(this, "Error", "No se pudo abrir el archivo.");
        }
    }
}

bool SimulatorClient::validateSyncData() {
    if (syncProcessList.isEmpty()) {
        QMessageBox::warning(this, "Error", "Debe cargar un archivo de procesos.");
        return false;
    }
    
    if (syncResourceList.isEmpty()) {
        QMessageBox::warning(this, "Error", "Debe cargar un archivo de recursos.");
        return false;
    }
    
    if (syncActionList.isEmpty()) {
        QMessageBox::warning(this, "Error", "Debe cargar un archivo de acciones.");
        return false;
    }
    
    return true;
}

void SimulatorClient::onBinarySemaphoreSimClicked() {
    if (!validateSyncData()) {
        return;
    }

    // Ocultar pantalla actual y mostrar simulación
    semaphoreWidget->hide();
    semaphoreSimulationWidget->show();

    // Configurar el scheduler
    binarySemaphoreScheduler->setProcesses(syncProcessList);
    binarySemaphoreScheduler->setResources(syncResourceList);
    binarySemaphoreScheduler->setActions(syncActionList);
    binarySemaphoreScheduler->setupGanttChart(semaphoreGanttView);
    
    // Iniciar simulación
    binarySemaphoreScheduler->startSimulation();
}

void SimulatorClient::onCountingSemaphoreSimClicked() {
    if (!validateSyncData()) {
        return;
    }

    // Ocultar pantalla actual y mostrar simulación
    semaphoreWidget->hide();
    semaphoreSimulationWidget->show();

    // Configurar el scheduler
    countingSemaphoreScheduler->setProcesses(syncProcessList);
    countingSemaphoreScheduler->setResources(syncResourceList);
    countingSemaphoreScheduler->setActions(syncActionList);
    countingSemaphoreScheduler->setupGanttChart(semaphoreGanttView);
    
    // Iniciar simulación
    countingSemaphoreScheduler->startSimulation();
}

void SimulatorClient::onMutexSimClicked() {
    if (!validateSyncData()) {
        return;
    }

    // Ocultar pantalla actual y mostrar simulación
    mutexWidget->hide();
    mutexSimulationWidget->show();

    // Configurar el scheduler
    mutexSynchronizer->setProcesses(syncProcessList);
    mutexSynchronizer->setResources(syncResourceList);
    mutexSynchronizer->setActions(syncActionList);
    mutexSynchronizer->setupGanttChart(mutGanttView);
    
    // Iniciar simulación
    mutexSynchronizer->startSimulation();
}

void SimulatorClient::onSemaphoreSimulationFinished(double avgExecutionTime) {
    QString metrics = QString("Tiempo promedio de ejecución: %1 unidades de tiempo").arg(avgExecutionTime);
    semaphoreMetricsLabel->setText(metrics);
    
    QMessageBox::information(this, "Simulación de Semáforo completada", 
                           "La simulación ha finalizado.\n" + metrics);
}

void SimulatorClient::onMutexSimulationFinished(double avgExecutionTime) {
    QString metrics = QString("Tiempo promedio de ejecución: %1 unidades de tiempo").arg(avgExecutionTime);
    semaphoreMetricsLabel->setText(metrics);
    
    QMessageBox::information(this, "Simulación de Mutex completada", 
                           "La simulación ha finalizado.\n" + metrics);
}

void SimulatorClient::setupSemaphoreSimulationWidget() {
    QVBoxLayout *semSimLayout = new QVBoxLayout(semaphoreSimulationWidget);
    
    // Título de la simulación
    QLabel *semSimTitle = new QLabel("Simulación de Sincronización con Semáforos", this);
    semSimTitle->setAlignment(Qt::AlignCenter);
    QFont font = semSimTitle->font();
    font.setBold(true);
    font.setPointSize(14);
    semSimTitle->setFont(font);
    semSimLayout->addWidget(semSimTitle);
    
    // Vista para el diagrama de Gantt de semáforos
    semaphoreGanttView = new QGraphicsView(this);
    semaphoreGanttView->setFixedHeight(220);
    semSimLayout->addWidget(semaphoreGanttView);
    
    // Etiqueta para mostrar métricas
    semaphoreMetricsLabel = new QLabel(this);
    semaphoreMetricsLabel->setAlignment(Qt::AlignCenter);
    semSimLayout->addWidget(semaphoreMetricsLabel);
    
    // Botón para regresar
    QPushButton *semBackButton = new QPushButton("Regresar al menú de semáforos", this);
    semSimLayout->addWidget(semBackButton);
    
    connect(semBackButton, &QPushButton::clicked, [=]() {
        semaphoreSimulationWidget->hide();
        semaphoreWidget->show();
    });
}

void SimulatorClient::setupMutexSimulationWidget() {
    QVBoxLayout *mutSimLayout = new QVBoxLayout(mutexSimulationWidget);
    
    // Título de la simulación
    QLabel *mutSimTitle = new QLabel("Simulación de Sincronización con Mutex", this);
    mutSimTitle->setAlignment(Qt::AlignCenter);
    QFont font = mutSimTitle->font();
    font.setBold(true);
    font.setPointSize(14);
    mutSimTitle->setFont(font);
    mutSimLayout->addWidget(mutSimTitle);
    
    // Vista para el diagrama de Gantt de mutex
    mutGanttView = new QGraphicsView(this);
    mutGanttView->setFixedHeight(220);
    mutSimLayout->addWidget(mutGanttView);
    
    // Etiqueta para mostrar métricas
    mutMetricsLabel = new QLabel(this);
    mutMetricsLabel->setAlignment(Qt::AlignCenter);
    mutSimLayout->addWidget(mutMetricsLabel);
    
    // Botón para regresar
    QPushButton *mutBackButton = new QPushButton("Regresar al menú de Mutex", this);
    mutSimLayout->addWidget(mutBackButton);
    
    connect(mutBackButton, &QPushButton::clicked, [=]() {
        mutexSimulationWidget->hide();
        mutexWidget->show();
    });
}

void SimulatorClient::setupSimulationWidget() {
    QVBoxLayout *simLayout = new QVBoxLayout(simulationWidget);
    
    QLabel *simTitle = new QLabel("Simulación de Calendarización", this);
    simTitle->setAlignment(Qt::AlignCenter);
    simLayout->addWidget(simTitle);
    
    ganttView = new QGraphicsView(this);
    ganttView->setFixedHeight(120);
    simLayout->addWidget(ganttView);
    
    metricsLabel = new QLabel(this);
    metricsLabel->setAlignment(Qt::AlignCenter);
    simLayout->addWidget(metricsLabel);
    
    QPushButton *backButton = new QPushButton("Regresar al menú principal", this);
    simLayout->addWidget(backButton);
    
    connect(backButton, &QPushButton::clicked, [=]() {
        simulationWidget->hide();
        welcomeLabel->show();
        chooseLabel->show();
        scheduleButton->show();
        syncButton->show();
    });
}

void SimulatorClient::onScheduleClicked() {
    welcomeLabel->hide();
    chooseLabel->hide();
    scheduleButton->hide();
    syncButton->hide();
    
    scheduleOptionsWidget->show();
}

void SimulatorClient::onSyncClicked() {
    welcomeLabel->hide();
    chooseLabel->hide();
    scheduleButton->hide();
    syncButton->hide();

    syncOptionsWidget->show();
}

void SimulatorClient::onReturnClicked() {
    welcomeLabel->show();
    chooseLabel->show();
    scheduleButton->show();
    syncButton->show();

    scheduleOptionsWidget->hide();
    syncOptionsWidget->hide();
}

void SimulatorClient::OnMutexClicked() {
   syncOptionsWidget->hide();
   mutexWidget->show();
}

void SimulatorClient::onCheckBoxMarked() {
    schedulingTypesToUse.clear();
    
    if (fcfsCheckBox->isChecked()) schedulingTypesToUse.append("First In First Out");
    if (sjfCheckBox->isChecked()) schedulingTypesToUse.append("Shortest Job First");
    if (srtCheckBox->isChecked()) schedulingTypesToUse.append("Shortest Remaining Time");
    if (rrCheckBox->isChecked()) schedulingTypesToUse.append("Round Robin");
    if (priorityCheckBox->isChecked()) schedulingTypesToUse.append("Priority");
}

void SimulatorClient::onSchedulingSimClicked() {
    onCheckBoxMarked();
    
    if (schedulingTypesToUse.isEmpty()) {
        QMessageBox::warning(this, "Error", "Debe seleccionar al menos un tipo de calendarización.");
        return;
    }
    
    if (processList.isEmpty()) {
        QMessageBox::warning(this, "Error", "Debe añadir al menos un archivo de procesos.");
        return;
    }
    
    scheduleOptionsWidget->hide();

    if (schedulingTypesToUse.length() == 1) {
        simulationWidget->show();

        if (schedulingTypesToUse.contains("Priority")) {
            runPrioritySimulation();
        } else if (schedulingTypesToUse.contains("Round Robin")){
            runRRSimulation();
        } else if (schedulingTypesToUse.contains("First In First Out")){
            runFiFoSimulation();
        } else if (schedulingTypesToUse.contains("Shortest Job First")){
            runSJFSimulation();
        } else if (schedulingTypesToUse.contains("Shortest Remaining Time")){
            runSRTSimulation();
        }
    } else {
        calculateSchedulingMetrics();
    }
}

void SimulatorClient::calculateSchedulingMetrics() {
    fifoTitleLabel->hide();
    fifoMetricsLabel->hide();
    sjfTitleLabel->hide();
    sjfMetricsLabel->hide();
    srtTitleLabel->hide();
    srtMetricsLabel->hide();
    rrTitleLabel->hide();
    rrMetricsLabel->hide();
    priTitleLabel->hide();
    priMetricsLabel->hide();

    if (schedulingTypesToUse.contains("First In First Out")){
        fifoScheduler->setProcesses(processList);
        double avgWaitingTime = fifoScheduler->simulateWithoutGUI();
        QString fifo_avgWT = QString("Tiempo promedio de espera: %1 unidades de tiempo\n").arg(avgWaitingTime);
        fifoMetricsLabel->setText(fifo_avgWT);
        fifoTitleLabel->show();
        fifoMetricsLabel->show();
    }
    
    if (schedulingTypesToUse.contains("Shortest Job First")){
        sjfScheduler->setProcesses(processList);
        double sjf_avgWT = sjfScheduler->simulateWithoutGUI();
        QString textSJF = QString("Tiempo promedio de espera: %1 unidades de tiempo\n").arg(sjf_avgWT);
        sjfMetricsLabel->setText(textSJF);
        sjfTitleLabel->show();
        sjfMetricsLabel->show();
    }
    
    if (schedulingTypesToUse.contains("Shortest Remaining Time")){
        srtScheduler->setProcesses(processList);
        double srt_avgWT = srtScheduler->simulateWithoutGUI();
        QString textSRT = QString("Tiempo promedio de espera: %1 unidades de tiempo\n").arg(srt_avgWT);
        srtMetricsLabel->setText(textSRT);
        srtTitleLabel->show();
        srtMetricsLabel->show();
    }
    
    if (schedulingTypesToUse.contains("Round Robin")){
        rrScheduler->setProcesses(processList);
        int quantum = quantumInput->text().toInt();
        rrScheduler->setQuantum(quantum);
        double rr_avgTime = rrScheduler->simulateWithoutGUI();
        QString rr_avgWT = QString("Tiempo promedio de espera: %1 unidades de tiempo\n").arg(rr_avgTime);
        rrMetricsLabel->setText(rr_avgWT);
        rrTitleLabel->show();
        rrMetricsLabel->show();
    }
    
    if (schedulingTypesToUse.contains("Priority")) {
        priorityScheduler->setProcesses(processList);
        double pr_avgWT = srtScheduler->simulateWithoutGUI();
        QString pri_avgWT = QString("Tiempo promedio de espera: %1 unidades de tiempo").arg(pr_avgWT);
        priMetricsLabel->setText(pri_avgWT);
        priTitleLabel->show();
        priMetricsLabel->show();
    }  

    scheduleMetricsWidget->show();
}

void SimulatorClient::onSimulationFinished(double avgWaitingTime) {
    QString metrics = QString("Tiempo promedio de espera: %1 unidades de tiempo").arg(avgWaitingTime);
    metricsLabel->setText(metrics);
    
    QMessageBox::information(this, "Simulación completada", 
                           "La simulación ha finalizado.\n" + metrics);
}

void SimulatorClient::runFiFoSimulation() {
    fifoScheduler->setProcesses(processList);
    fifoScheduler->setupGanttChart(ganttView);
    fifoScheduler->startSimulation();
}

void SimulatorClient::runSJFSimulation() {
    sjfScheduler->setProcesses(processList);
    sjfScheduler->setupGanttChart(ganttView);
    sjfScheduler->startSimulation();
}

void SimulatorClient::runSRTSimulation() {
    srtScheduler->setProcesses(processList);
    srtScheduler->setupGanttChart(ganttView);
    srtScheduler->startSimulation();
}

void SimulatorClient::runRRSimulation() {
    rrScheduler->setProcesses(processList);
    int quantum = quantumInput->text().toInt();
    rrScheduler->setQuantum(quantum);
    rrScheduler->setupGanttChart(ganttView);
    rrScheduler->startSimulation();
}

void SimulatorClient::runPrioritySimulation() {
    priorityScheduler->setProcesses(processList);
    priorityScheduler->setupGanttChart(ganttView);
    priorityScheduler->startSimulation();
}

void SimulatorClient::onAddFileClicked_Process() {
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Seleccionar archivo de procesos",
        "",
        "Archivos de texto (*.txt);;Todos los archivos (*)"
    );

    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            processList.clear();
            
            openFileLabel->setText("Usando el archivo: " + fileName);
            openFileLabel->show();

            QTextStream in(&file);
            while (!in.atEnd()) {
                QString line = in.readLine();
                QStringList fields = line.split(",");

                if (fields.size() == 4) {
                    Process p;
                    p.pid = fields[0].trimmed();
                    p.burstTime = fields[1].trimmed().toInt();
                    p.arrivalTime = fields[2].trimmed().toInt();
                    p.priority = fields[3].trimmed().toInt();

                    processList.append(p);
                }
            }
            file.close();
            
            QString info = QString("Se han cargado %1 procesos desde el archivo.").arg(processList.size());
            QMessageBox::information(this, "Archivo cargado", info);
        } else {
            QMessageBox::warning(this, "Error", "No se pudo abrir el archivo.");
        }
    }
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    SimulatorClient client;
    client.show();
    return app.exec();
}