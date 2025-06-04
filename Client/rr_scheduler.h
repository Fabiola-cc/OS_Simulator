#ifndef ROUND_ROBIN_SCHEDULER_H
#define ROUND_ROBIN_SCHEDULER_H

#include <QObject>
#include <QTimer>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QMap>
#include <QQueue>
#include "structures.h"

class RoundRobinScheduler : public QObject {
    Q_OBJECT

public:
    explicit RoundRobinScheduler(QObject *parent = nullptr);
    void setupGanttChart(QGraphicsView *view);
    void setProcesses(const QList<Process>& newProcesses);
    void startSimulation();
    void stopSimulation();
    void setQuantum(int q);
    double simulateWithoutGUI();

signals:
    void simulationFinished(double averageWaitingTime);

private slots:
    void updateSimulation();

private:
    QGraphicsView *ganttView;
    QGraphicsScene *ganttScene;
    QTimer *simulationTimer;

    QList<Process> allProcesses;
    QMap<QString, QColor> processColors;
    QMap<QString, int> remainingBurst;
    QMap<QString, int> completionTimes;
    QMap<QString, int> waitingTimes;
    QQueue<Process> readyQueue;

    int currentTime;
    int quantum;
    int quantumCounter;
    Process currentProcess;
    bool hasCurrentProcess;
    QQueue<Process> postponedQueue;

    void assignProcessColors();
    void calculateMetrics();
};

#endif // ROUND_ROBIN_SCHEDULER_H
