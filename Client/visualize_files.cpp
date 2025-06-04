#include "visualize_files.h"

VisualizeFiles::VisualizeFiles(const QList<Process>& processes,
                                   const QList<Resource>& resources,
                                   const QList<Action>& actions,
                                   QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Información cargada");
    resize(650, 400);

    tabWidget = new QTabWidget(this);

    setupProcessTab(processes);
    setupResourceTab(resources);
    setupActionTab(actions);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(tabWidget);
    setLayout(mainLayout);
}

void VisualizeFiles::setupProcessTab(const QList<Process>& processes) {
    processTable = new QTableWidget(this);
    processTable->setColumnCount(4);
    processTable->setHorizontalHeaderLabels({"PID", "Burst Time", "Arrival Time", "Priority"});
    processTable->setRowCount(processes.size());

    for (int i = 0; i < processes.size(); ++i) {
        processTable->setItem(i, 0, new QTableWidgetItem(processes[i].pid));
        processTable->setItem(i, 1, new QTableWidgetItem(QString::number(processes[i].burstTime)));
        processTable->setItem(i, 2, new QTableWidgetItem(QString::number(processes[i].arrivalTime)));
        processTable->setItem(i, 3, new QTableWidgetItem(QString::number(processes[i].priority)));
    }

    tabWidget->addTab(processTable, "Procesos");
}

void VisualizeFiles::setupResourceTab(const QList<Resource>& resources) {
    resourceTable = new QTableWidget(this);
    resourceTable->setColumnCount(2);
    resourceTable->setHorizontalHeaderLabels({"Nombre", "Contador"});
    resourceTable->setRowCount(resources.size());

    for (int i = 0; i < resources.size(); ++i) {
        resourceTable->setItem(i, 0, new QTableWidgetItem(resources[i].name));
        resourceTable->setItem(i, 1, new QTableWidgetItem(QString::number(resources[i].counter)));
    }

    tabWidget->addTab(resourceTable, "Recursos");
}


void VisualizeFiles::setupActionTab(const QList<Action>& actions) {
    actionTable = new QTableWidget(this);
    actionTable->setColumnCount(4);
    actionTable->setHorizontalHeaderLabels({"PID", "Operación", "Recurso", "Ciclo"});
    actionTable->setRowCount(actions.size());

    for (int i = 0; i < actions.size(); ++i) {
        actionTable->setItem(i, 0, new QTableWidgetItem(actions[i].pid));
        actionTable->setItem(i, 1, new QTableWidgetItem(actions[i].operation));
        actionTable->setItem(i, 2, new QTableWidgetItem(actions[i].resource));
        actionTable->setItem(i, 3, new QTableWidgetItem(QString::number(actions[i].cycle)));
    }

    tabWidget->addTab(actionTable, "Acciones");
}

// Función auxiliar si solo quieres una tabla simple de procesos
QWidget* createProcessTableOnly(const QList<Process>& processes) {
    QWidget* widget = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(widget);
    QTableWidget* table = new QTableWidget(widget);
    
    table->setColumnCount(4);
    table->setHorizontalHeaderLabels({"PID", "Burst Time", "Arrival Time", "Priority"});
    table->setRowCount(processes.size());

    for (int i = 0; i < processes.size(); ++i) {
        table->setItem(i, 0, new QTableWidgetItem(processes[i].pid));
        table->setItem(i, 1, new QTableWidgetItem(QString::number(processes[i].burstTime)));
        table->setItem(i, 2, new QTableWidgetItem(QString::number(processes[i].arrivalTime)));
        table->setItem(i, 3, new QTableWidgetItem(QString::number(processes[i].priority)));
    }

    layout->addWidget(table);
    widget->setLayout(layout);
    return widget;
}
