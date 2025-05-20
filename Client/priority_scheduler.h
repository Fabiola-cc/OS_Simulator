#ifndef PRIORITY_SCHEDULER_H
#define PRIORITY_SCHEDULER_H

#include <QObject>
#include <QList>
#include <QColor>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QTimer>
#include <QMap>
#include "structures.h"

class PriorityScheduler : public QObject {
    Q_OBJECT

public:
    explicit PriorityScheduler(QObject *parent = nullptr);
    
    // Configura el diagrama de Gantt
    void setupGanttChart(QGraphicsView *view);
    
    // Establece la lista de procesos a programar
    void setProcesses(const QList<Process>& processes);
    
    // Inicia la simulación
    void startSimulation();
    
    // Detiene la simulación
    void stopSimulation();

signals:
    // Señal emitida cuando la simulación termina
    void simulationFinished(double avgWaitingTime);
    
private slots:
    // Slot que se ejecuta en cada ciclo de la simulación
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
    Process* getNextProcessByPriority();
    
    // Método para dibujar el diagrama de Gantt
    void drawGanttChart();
    
    // Método para asignar colores a los procesos
    void assignProcessColors();
    
    // Método para calcular métricas finales
    void calculateMetrics();
};

#endif // PRIORITY_SCHEDULER_H