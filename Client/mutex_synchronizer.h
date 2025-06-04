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
#include <QTextEdit>
#include <QGraphicsProxyWidget>
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
    void setupInformationPanel();
    void setupLegend();
    void setupProcessRows(int startY);
    void setupTimeGrid(int startY, int endY);
    void drawAccessBar(const QString& pid, int index, int time, QColor color, const QString& operation); 

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
    QMap<QString, bool> resourceMutexes;

    QGraphicsTextItem *processLogLabel;
    QGraphicsRectItem *processLogBackground;
    QStringList processLogHistory;
    QTextEdit* logWidget;



    QList<Process> processes;
    QList<Resource> resources;
    QList<Action> actions;
    QMap<QString, QString> resourceOwners; 

    QQueue<Process> readyQueue;
    QMap<QString, QColor> processColors;
    QMap<QString, int> processExecutionTimes;
    QMap<QString, int> processStartTimes;

    Process currentProcess;
    int currentProcessRemainingTime;
    bool hasCurrentProcess;

    QMap<QString, Process*> processMap;
    QMap<QString, Resource*> resourceMap;
    QList<ActiveMutexAction> activeMutexActions;
    QQueue<Action> blockedQueue;

    QGraphicsTextItem *semaphoreStatusLabel;
    QGraphicsTextItem *blockedQueueLabel;
    QGraphicsTextItem *currentProcessLabel;
    QGraphicsTextItem *resourceIndicatorLabel;
    QGraphicsTextItem* resourceUsageLabel;

    void assignProcessColors();
    void initializeResources();
    void processActions();
    void handleMutexOperation(const Action &action);
    void executeCurrentProcess();
    void calculateMetrics();
    void appendLog(const QString& line);
};

#endif
