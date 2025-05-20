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

SimulatorClient::SimulatorClient(QWidget *parent) : QWidget(parent) {
    resize(800, 600); // Aumentamos la altura para el diagrama de Gantt
    setWindowTitle("Simulator");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    mainLayout->setAlignment(Qt::AlignTop); // Todo arriba pero centrado horizontalmente
    mainLayout->setContentsMargins(50, 30, 50, 30); // Márgenes externos
    mainLayout->setSpacing(20); // Espacio entre secciones

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

    processListLabel = new QLabel(this);
    processListLabel->setAlignment(Qt::AlignLeft);
    mainLayout->addWidget(processListLabel);
    processListLabel->hide(); // ocultarlo al inicio

    ///////////////////////////////////////////////////////////////////

    // Elementos y diseño de pantalla de CALENDARIZACIÓN
    // Crear contenedor
    scheduleOptionsWidget = new QWidget(this);
    QVBoxLayout *scheduleLayout = new QVBoxLayout(scheduleOptionsWidget);

    // Título e instrucciones
    titleLabel1 = new QLabel("Calendarización", this);
    instrLabel1 = new QLabel("Escoge 1 o varios tipos de calendarización para trabajar", this);
    titleLabel1->setAlignment(Qt::AlignCenter);
    instrLabel1->setAlignment(Qt::AlignCenter);

    // Checkboxes
    fcfsCheckBox = new QCheckBox("First In First Out", this);
    sjfCheckBox = new QCheckBox("Shortest Job First", this);
    srtCheckBox = new QCheckBox("Shortest Remaining Time", this);
    rrCheckBox = new QCheckBox("Round Robin", this);
    priorityCheckBox = new QCheckBox("Priority", this);

    // Quantum
    quantumLabel = new QLabel("Quantum:", this);
    quantumInput = new QLineEdit(this);
    quantumLabel->setVisible(false);
    quantumInput->setVisible(false);

    connect(rrCheckBox, &QCheckBox::toggled, [=](bool checked){
        quantumLabel->setVisible(checked);
        quantumInput->setVisible(checked);
    });

    // Botones
    addFileButton = new QPushButton("Añadir Archivo", this);
    schedulingSimButton = new QPushButton("Iniciar Simulación", this);
    returnButton = new QPushButton("Regresar", this);

    // Agregar widgets al layout
    scheduleLayout->addWidget(titleLabel1);
    scheduleLayout->addWidget(instrLabel1);
    scheduleLayout->addWidget(fcfsCheckBox);
    scheduleLayout->addWidget(sjfCheckBox);
    scheduleLayout->addWidget(srtCheckBox);
    scheduleLayout->addWidget(rrCheckBox);
    scheduleLayout->addWidget(priorityCheckBox);

    // Quantum en layout horizontal
    QHBoxLayout *quantumLayout = new QHBoxLayout();
    quantumLayout->addWidget(quantumLabel);
    quantumLayout->addWidget(quantumInput);
    scheduleLayout->addLayout(quantumLayout);

    // Botones en layout horizontal
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

    // Agregar el contenedor al layout principal
    mainLayout->addWidget(scheduleOptionsWidget);

    // Ocultar inicialmente
    scheduleOptionsWidget->hide();

    ///////////////////////////////////////////////////////////////////

    // Elementos y diseño de pantalla de SINCRONIZACIÓN
    // Crear contenedor
    syncOptionsWidget = new QWidget(this);
    QVBoxLayout *syncLayout = new QVBoxLayout(syncOptionsWidget);

    // Título e instrucciones
    titleLabel2 = new QLabel("Sincronización", this);
    instrLabel2 = new QLabel("Escoge el tipo de sincronización para trabajar", this);
    titleLabel2->setAlignment(Qt::AlignCenter);
    instrLabel2->setAlignment(Qt::AlignCenter);

    // Botones
    mutexButton = new QPushButton("Mutex Lock", this);
    semaphoreButton = new QPushButton("Semaphore", this);
    returnButton2 = new QPushButton("Regresar", this);
    
    // Agregar widgets al layout
    syncLayout->addWidget(titleLabel2);
    syncLayout->addWidget(instrLabel2);
    
    // Botones en layout horizontal
    QHBoxLayout *buttonLayout3 = new QHBoxLayout();
    buttonLayout3->addStretch();
    buttonLayout3->addWidget(mutexButton);
    buttonLayout3->addWidget(semaphoreButton);
    buttonLayout3->addStretch();

    syncLayout->addLayout(buttonLayout3);
    syncLayout->addWidget(returnButton2);

    // Agregar el contenedor al layout principal
    mainLayout->addWidget(syncOptionsWidget);

    // Ocultar inicialmente
    syncOptionsWidget->hide();
    
    ///////////////////////////////////////////////////////////////////

    // NUEVO: Widget para la simulación con diagrama de Gantt
    simulationWidget = new QWidget(this);
    setupSimulationWidget();
    mainLayout->addWidget(simulationWidget);
    simulationWidget->hide();
    
    ///////////////////////////////////////////////////////////////////
    
    setLayout(mainLayout);

    // Configuración de conexiones entre señales y slots
    connect(scheduleButton, &QPushButton::clicked, this, &SimulatorClient::onScheduleClicked);   // Conectar al hacer clic
    connect(schedulingSimButton, &QPushButton::clicked, this, &SimulatorClient::onSchedulingSimClicked);
    connect(returnButton, &QPushButton::clicked, this, &SimulatorClient::onReturnClicked);
    connect(addFileButton, &QPushButton::clicked, this, &SimulatorClient::onAddFileClicked_Process);

    connect(syncButton, &QPushButton::clicked, this, &SimulatorClient::onSyncClicked);   
    connect(returnButton2, &QPushButton::clicked, this, &SimulatorClient::onReturnClicked);
    
    // Inicilizar el schedyler de RoundRobin
    rrScheduler = new RoundRobinScheduler(this);
    connect(rrScheduler, &RoundRobinScheduler::simulationFinished, 
            this, &SimulatorClient::onRRFinished);

    // Inicializar el scheduler de priority
    priorityScheduler = new PriorityScheduler(this);
    connect(priorityScheduler, &PriorityScheduler::simulationFinished, 
            this, &SimulatorClient::onPrioritySimulationFinished);
}

void SimulatorClient::setupSimulationWidget() {
    QVBoxLayout *simLayout = new QVBoxLayout(simulationWidget);
    
    // Título de la simulación
    QLabel *simTitle = new QLabel("Simulación de Calendarización", this);
    simTitle->setAlignment(Qt::AlignCenter);
    simLayout->addWidget(simTitle);
    
    // Vista para el diagrama de Gantt
    ganttView = new QGraphicsView(this);
    ganttView->setFixedHeight(120);
    simLayout->addWidget(ganttView);
    
    // Etiqueta para mostrar métricas
    metricsLabel = new QLabel(this);
    metricsLabel->setAlignment(Qt::AlignCenter);
    simLayout->addWidget(metricsLabel);
    
    // Botón para regresar
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

// lógica cuando el botón "Calendarización" es presionado
void SimulatorClient::onScheduleClicked() {
    // Ocultar controles iniciales
    welcomeLabel->hide();
    chooseLabel->hide();
    scheduleButton->hide();
    syncButton->hide();
    
    scheduleOptionsWidget->show();
}

// lógica cuando el botón "Sincronización" es presionado
void SimulatorClient::onSyncClicked() {
    // Ocultar controles inciales
    welcomeLabel->hide();
    chooseLabel->hide();
    scheduleButton->hide();
    syncButton->hide();

    syncOptionsWidget->show();
}

void SimulatorClient::onReturnClicked() {
    // Mostrar controles inciales
    welcomeLabel->show();
    chooseLabel->show();
    scheduleButton->show();
    syncButton->show();

    scheduleOptionsWidget->hide();
    syncOptionsWidget->hide();
}

void SimulatorClient::onCheckBoxMarked() {
    schedulingTypesToUse.clear(); // Limpiar la lista primero
    
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
    
    // Ocultar pantalla de opciones
    scheduleOptionsWidget->hide();
    
    // Mostrar pantalla de simulación
    simulationWidget->show();
    
    // Iniciar la simulación correspondiente
    if (schedulingTypesToUse.contains("Priority")) {
        runPrioritySimulation();
    } else if (schedulingTypesToUse.contains("Round Robin")){
        runRRSimulation();
    } else {
        // Aquí se implementaría la lógica para los otros algoritmos
        QMessageBox::information(this, "Información", "La simulación para el algoritmo seleccionado no está implementada aún.");
        simulationWidget->hide();
        welcomeLabel->show();
        chooseLabel->show();
        scheduleButton->show();
        syncButton->show();
    }
    
}

void SimulatorClient::runRRSimulation() {
    // Configurar el scheduler con los procesos
    rrScheduler->setProcesses(processList);
    int quantum = quantumInput->text().toInt();
    rrScheduler->setQuantum(quantum);
    
    // Configurar el diagrama de Gantt
    rrScheduler->setupGanttChart(ganttView);
    
    // Iniciar la simulación
    rrScheduler->startSimulation();
}

void SimulatorClient::onRRFinished(double avgWaitingTime) {
    // Mostrar métricas
    QString metrics = QString("Tiempo promedio de espera: %1 unidades de tiempo").arg(avgWaitingTime);
    metricsLabel->setText(metrics);
    
    QMessageBox::information(this, "Simulación completada", 
                           "La simulación de Round Robin ha finalizado.\n" + metrics);
}

void SimulatorClient::runPrioritySimulation() {
    // Configurar el scheduler con los procesos
    priorityScheduler->setProcesses(processList);
    
    // Configurar el diagrama de Gantt
    priorityScheduler->setupGanttChart(ganttView);
    
    // Iniciar la simulación
    priorityScheduler->startSimulation();
}

void SimulatorClient::onPrioritySimulationFinished(double avgWaitingTime) {
    // Mostrar métricas
    QString metrics = QString("Tiempo promedio de espera: %1 unidades de tiempo").arg(avgWaitingTime);
    metricsLabel->setText(metrics);
    
    QMessageBox::information(this, "Simulación completada", 
                           "La simulación de Priority ha finalizado.\n" + metrics);
}

void SimulatorClient::onAddFileClicked_Process() {
    // Abrir diálogo para seleccionar archivo
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Seleccionar archivo de procesos",
        "",
        "Archivos de texto (*.txt);;Todos los archivos (*)"
    );

    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            // Limpiar lista de procesos anterior
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
            
            // Mostrar información de los procesos cargados
            QString info = QString("Se han cargado %1 procesos desde el archivo.").arg(processList.size());
            QMessageBox::information(this, "Archivo cargado", info);
        } else {
            QMessageBox::warning(this, "Error", "No se pudo abrir el archivo.");
        }
    }
}

void SimulatorClient::onAddFileClicked_Actions() {
    // Abrir diálogo para seleccionar archivo
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Seleccionar archivo de procesos",
        "",
        "Archivos de texto (*.txt);;Todos los archivos (*)"
    );

    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            while (!in.atEnd()) {
                QString line = in.readLine();
                QStringList fields = line.split(",");

                if (fields.size() == 4) {
                    Action a;
                    a.pid = fields[0].trimmed();
                    a.operation = fields[1].trimmed();
                    a.resource = fields[2].trimmed();
                    a.cycle = fields[3].trimmed().toInt();
                    actions.append(a);
                }
            }
            file.close();    
        } else {
            QMessageBox::warning(this, "Error", "No se pudo abrir el archivo.");
        }
    }
}

void SimulatorClient::onAddFileClicked_Resources() {
    // Abrir diálogo para seleccionar archivo
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Seleccionar archivo de procesos",
        "",
        "Archivos de texto (*.txt);;Todos los archivos (*)"
    );

    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            while (!in.atEnd()) {
                QString line = in.readLine();
                QStringList fields = line.split(",");

                if (fields.size() == 2) {
                    Resource r;
                    r.name = fields[0].trimmed();
                    r.counter = fields[1].trimmed().toInt();
                    resources.append(r);
                }
            }
            file.close();
        } else {
            QMessageBox::warning(this, "Error", "No se pudo abrir el archivo.");
        }
    }
}

/**
 * @brief Punto de entrada principal de la aplicación
 */
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    SimulatorClient client;
    client.show();
    return app.exec();
}