#ifndef MUTEX_SYNCHRONIZER_H
#define MUTEX_SYNCHRONIZER_H

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

class MutexSynchronizer : public QObject {
    Q_OBJECT

public:
    explicit MutexSynchronizer(QObject *parent = nullptr);
    void setupGanttChart(QGraphicsView *view);
    void setProcesses(const QList<Process> &newProcesses);
    void setResources(const QList<Resource> &newResources);
    void setActions(const QList<Action> &newActions);
    void startSimulation();
    void stopSimulation();
    void processAction(const Action&);
    int resourceIndexByName(const QString&) const;

signals:
    void simulationFinished(double averageExecutionTime);

private slots:
    void updateSimulation();
    int processIndexByPid(const QString& pid) const;

private:
    QSet<QString> blockedLockProcesses;
    QGraphicsScene *ganttScene;
    QGraphicsView *ganttView;
    QTimer *simulationTimer;
    int currentTime;
    QGraphicsTextItem *currentTimeLabel;
    QGraphicsTextItem *resourceStatusLabel;


    QList<Process> processes;
    QList<Resource> resources;
    QList<Action> actions;
    QString currentMutexOwner; 

    QQueue<Process> readyQueue;
    QQueue<Process> blockedQueue;
    QMap<QString, QColor> processColors;
    QMap<QString, int> processExecutionTimes;
    QMap<QString, int> processStartTimes;

    Process currentProcess;
    int currentProcessRemainingTime;
    bool hasCurrentProcess;

    QMap<QString, bool> resourceMutexes;
    QMap<QString, Process*> processMap;
    QMap<QString, Resource*> resourceMap;
    QList<ActiveMutexAction> activeMutexActions;

    void assignProcessColors();
    void initializeResources();
    void processActions();
    void handleMutexOperation(const Action &action);
    void executeCurrentProcess();
    void calculateMetrics();
};

#endif
