#ifndef COUNTING_SEMAPHORE_SCHEDULER_H
#define COUNTING_SEMAPHORE_SCHEDULER_H

#include <QObject>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QTimer>
#include <QQueue>
#include <QMap>
#include <QString>
#include <QColor>
#include <QSet>
#include "structures.h"
#include "semaforo_conteo.h"

class CountingSemaphoreScheduler : public QObject {
    Q_OBJECT

public:
    // Enumeración para estados de procesos
    enum ProcessState {
        INACTIVE,           // Proceso aún no ha llegado
        READY,              // En cola de listos
        EXECUTING,          // Ejecutándose actualmente
        BLOCKED,            // Bloqueado esperando semáforo
        ACCESSING_RESOURCE  // Accediendo al recurso
    };

    explicit CountingSemaphoreScheduler(QObject *parent = nullptr);
    void setupGanttChart(QGraphicsView *view);
    void setProcesses(const QList<Process> &newProcesses);
    void setResources(const QList<Resource> &newResources);
    void setActions(const QList<Action> &newActions);
    void startSimulation();
    void stopSimulation();

signals:
    void simulationFinished(double averageExecutionTime);

private slots:
    void updateSimulation();

private:
    // Componentes gráficos principales
    QGraphicsScene *ganttScene;
    QGraphicsView *ganttView;
    QTimer *simulationTimer;

    // Labels informativos específicos para conteo
    QGraphicsTextItem *currentTimeLabel;
    QGraphicsTextItem *semaphoreStatusLabel;
    QGraphicsTextItem *blockedQueueLabel;
    QGraphicsTextItem *currentProcessLabel;
    QGraphicsTextItem *resourceIndicatorLabel;

    // Estado de la simulación
    int currentTime;
    Process currentProcess;
    int currentProcessRemainingTime;
    bool hasCurrentProcess;

    // Datos de la simulación
    QList<Process> processes;
    QList<Resource> resources;
    QList<Action> actions;

    // Colas de procesos
    QQueue<Process> readyQueue;
    QQueue<Process> blockedQueue;

    // Mapas de seguimiento
    QMap<QString, QColor> processColors;
    QMap<QString, int> processExecutionTimes;
    QMap<QString, int> processStartTimes;
    QMap<QString, Semaphore> resourceSemaphores;
    
    // Tracking específico para recursos de conteo
    QMap<QString, int> processResourceUsage;  // Cuántos recursos usa cada proceso

    // === MÉTODOS DE CONFIGURACIÓN VISUAL ===
    void setupInformationPanel();
    void setupLegend();
    void setupProcessRows(int startY);
    void setupTimeGrid(int startY, int endY);

    // === MÉTODOS DE VISUALIZACIÓN ===
    void drawAllProcessStates();
    void updateInformationLabels();
    void updateResourceIndicator();
    
    // === MÉTODOS DE ESTADO ===
    ProcessState getProcessState(const QString& pid);
    QColor getStateColor(ProcessState state);
    QString getStateSymbol(ProcessState state);
    QString getStateName(ProcessState state);
    int getResourcesUsedByProcess(const QString& pid);

    // === MÉTODOS DE LÓGICA ===
    void assignProcessColors();
    void initializeResources();
    void processActions();
    void handleSemaphoreOperation(const Action &action);
    void executeCurrentProcess();
    void calculateMetrics();
    bool checkSimulationComplete();
};

#endif // COUNTING_SEMAPHORE_SCHEDULER_H