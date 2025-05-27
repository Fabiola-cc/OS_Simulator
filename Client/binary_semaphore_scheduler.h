#ifndef BINARY_SEMAPHORE_SCHEDULER_H
#define BINARY_SEMAPHORE_SCHEDULER_H

#include <QObject>
#include <QTimer>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QMap>
#include <QQueue>
#include "structures.h"
#include "semaforo_binario.h"

class BinarySemaphoreScheduler : public QObject {
    Q_OBJECT

public:
    explicit BinarySemaphoreScheduler(QObject *parent = nullptr);
    void setupGanttChart(QGraphicsView *view);
    void setProcesses(const QList<Process>& processes);
    void setResources(const QList<Resource>& resources);
    void setActions(const QList<Action>& actions);
    void startSimulation();
    void stopSimulation();

signals:
    void simulationFinished(double avgExecutionTime);

private slots:
    void updateSimulation();

private:
    // Datos de entrada
    QList<Process> processes;
    QList<Resource> resources;
    QList<Action> actions;
    
    // Semáforos para cada recurso
    QMap<QString, semaphore> resourceSemaphores;
    
    // Estado de simulación
    QGraphicsView *ganttView;
    QGraphicsScene *ganttScene;
    QTimer *simulationTimer;
    
    int currentTime;
    QMap<QString, QColor> processColors;
    QMap<QString, int> processExecutionTimes;
    QMap<QString, int> processStartTimes;
    QQueue<Process> readyQueue;
    QQueue<Process> blockedQueue;
    
    // Estado actual de procesos
    Process currentProcess;
    bool hasCurrentProcess;
    int currentProcessRemainingTime;
    
    void assignProcessColors();
    void initializeResources();
    void processActions();
    Action getCurrentAction();
    bool hasActionsForCurrentTime();
    void executeCurrentProcess();
    void handleSemaphoreOperation(const Action& action);
    void calculateMetrics();
};

#endif // BINARY_SEMAPHORE_SCHEDULER_H