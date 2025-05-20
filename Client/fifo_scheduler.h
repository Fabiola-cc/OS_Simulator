#ifndef FIFO_SCHEDULER_H
#define FIFO_SCHEDULER_H

#include <QObject>
#include <QTimer>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QMap>
#include <QQueue>
#include "structures.h"

class FiFoScheduler : public QObject {
    Q_OBJECT

public:
    explicit FiFoScheduler(QObject *parent = nullptr);
    void setupGanttChart(QGraphicsView *view);
    void setProcesses(const QList<Process>& newProcesses);
    void startSimulation();
    void stopSimulation();
    double simulateWithoutGUI();

signals:
    void simulationFinished(double averageWaitingTime);

private slots:
    void updateSimulation();

private:
    // Lista de procesos originales
    QList<Process> processes;
    
    // Lista de procesos ordenados por su tiempo de llegada
    QList<Process> arrivalSortedProcesses;
    
    // Escena para el diagrama de Gantt
    QGraphicsScene *ganttScene;
    
    // Vista para el diagrama de Gantt
    QGraphicsView *ganttView;
    
    // Timer para controlar la animación
    QTimer *simulationTimer;
    
    // Estado actual de la simulación
    int nextIndex;
    int currentTime;
    int currentProcessIndex;
    bool simulationRunning;
    
    // Proceso actualmente en ejecución
    Process *currentProcess;
    
    // Colores asignados a cada proceso
    QMap<QString, QColor> processColors;
    
    // Métricas
    QMap<QString, int> waitingTimes;
    QMap<QString, int> completionTimes;
    
    // Método para calcular el siguiente proceso a ejecutar basado en prioridad
    Process* getNextProcessByOrder();
    
    // Método para asignar colores a los procesos
    void assignProcessColors();
    
    // Método para calcular métricas finales
    void calculateMetrics();
};

#endif // FIFO_SCHEDULER_H