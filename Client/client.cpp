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

SimulatorClient::SimulatorClient(QWidget *parent) : QWidget(parent) {
    resize(800, 400);
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
    buttonLayout2->addWidget(returnButton);
    buttonLayout2->addStretch();
    scheduleLayout->addLayout(buttonLayout2);

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
    
    setLayout(mainLayout);

    // Configuración de conexiones entre señales y slots
    connect(scheduleButton, &QPushButton::clicked, this, &SimulatorClient::onScheduleClicked);   // Conectar al hacer clic
    connect(syncButton, &QPushButton::clicked, this, &SimulatorClient::onSyncClicked);   
    connect(returnButton, &QPushButton::clicked, this, &SimulatorClient::onReturnClicked);
    connect(returnButton2, &QPushButton::clicked, this, &SimulatorClient::onReturnClicked);
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
