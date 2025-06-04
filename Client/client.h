#ifndef CLIENT_H
#define CLIENT_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QLineEdit>
#include <QGraphicsView>
#include <QTextEdit>
#include <QClipboard>
#include "structures.h"
#include "fifo_scheduler.h"
#include "sjf_scheduler.h"
#include "srt_scheduler.h"
#include "priority_scheduler.h"
#include "rr_scheduler.h"
#include "counting_semaphore_scheduler.h"
#include "mutex_synchronizer.h"

class SimulatorClient : public QWidget {
    Q_OBJECT

public:
    explicit SimulatorClient(QWidget *parent = nullptr);

public slots:
    void onScheduleClicked();
    void onSyncClicked();
    void onCheckBoxMarked();
    void onReturnClicked();
    void onSchedulingSimClicked();
    void onAddFileClicked_Process();
    void onSimulationFinished(double avgWaitingTime);
    void OnMutexClicked();
    void onCountingSemaphoreSimClicked();
    void onSemaphoreSimulationFinished(double avgExecutionTime);
    void onMutexSimClicked();
    void onMutexSimulationFinished(double avgExecutionTime);  // ← LÍNEA 34 PROBLEMÁTICA

private:
    // General
    QList<QString> schedulingTypesToUse;
    QList<Process> processList;
    QList<Resource> resources;
    QList<Action> actions;
    QList<Process> syncProcessList;
    QList<Resource> syncResourceList;
    QList<Action> syncActionList;

    // Labels
    QLabel *welcomeLabel;
    QLabel *chooseLabel;
    QLabel *titleLabel1;
    QLabel *instrLabel1;
    QLabel *titleLabel2;
    QLabel *instrLabel2;
    QLabel *openFileLabel;
    QLabel *metricsLabel;
    
    // Labels Para mostrar las métricas de varios calendarizadores
    QLabel *fifoMetricsLabel;
    QLabel *fifoTitleLabel;
    QLabel *sjfMetricsLabel;
    QLabel *sjfTitleLabel;
    QLabel *srtMetricsLabel;
    QLabel *srtTitleLabel;
    QLabel *rrMetricsLabel;
    QLabel *rrTitleLabel;
    QLabel *priMetricsLabel;
    QLabel *priTitleLabel;

    // Buttons
    QPushButton *scheduleButton;
    QPushButton *syncButton;
    QPushButton *addFileButton;
    QPushButton *schedulingSimButton;
    QPushButton *returnButton;
    QPushButton *returnButton2;
    QPushButton *mutexButton;
    QPushButton *semaphoreButton;
    QPushButton *mutBackButton;
    QPushButton *startSimMut;

    // Checkboxes
    QCheckBox *fcfsCheckBox;
    QCheckBox *sjfCheckBox;
    QCheckBox *srtCheckBox;
    QCheckBox *rrCheckBox;
    QCheckBox *priorityCheckBox;

    // Entrada para Quantum
    QLabel *quantumLabel;
    QLineEdit *quantumInput;

    // Contenedores
    QWidget *scheduleOptionsWidget;
    QWidget *syncOptionsWidget;
    QWidget *simulationWidget;
    QWidget *scheduleMetricsWidget;
    QWidget *mutexWidget;
    QWidget *semaphoreWidget;   
    QWidget *semaphoreSimulationWidget;
    QWidget *mutexSimulationWidget;

    // Componentes para visualización de simulación
    QGraphicsView *ganttView;
    QGraphicsView *semaphoreGanttView;
    QGraphicsView *mutGanttView;
    
    // Schedulers
    FiFoScheduler *fifoScheduler;
    ShortestJobFirstScheduler *sjfScheduler;
    ShortestRemainingTimeScheduler *srtScheduler;
    RoundRobinScheduler *rrScheduler;
    PriorityScheduler *priorityScheduler;
    
    // Synchronizers
    CountingSemaphoreScheduler *countingSemaphoreScheduler;
    MutexSynchronizer *mutexSynchronizer;

    QLabel *syncProcessStatusLabel;
    QLabel *syncResourceStatusLabel;
    QLabel *syncActionStatusLabel;
    QLabel *syncProcessStatusLabel2;
    QLabel *syncResourceStatusLabel2;
    QLabel *syncActionStatusLabel2;
    QLabel *semaphoreMetricsLabel;
    QLabel *mutMetricsLabel;

    // Variables para el log
    QWidget *logDisplayWidget;
    QTextEdit *logTextEdit;

    // Métodos privados
    void setupSimulationWidget();
    void setupSemaphoreSimulationWidget();
    void setupMutexSimulationWidget();
    void calculateSchedulingMetrics();
    void runFiFoSimulation();
    void runSJFSimulation();
    void runSRTSimulation();
    void runPrioritySimulation();
    void runRRSimulation();
    void OnSemaphoreClicked();
    void onLoadMutProcessesClicked();
    void onLoadSyncProcessesClicked();
    void onLoadSyncResourcesClicked();
    void onLoadSyncActionsClicked();
    bool validateSyncData();
    bool validateSyncDataMutex();
    
    // Métodos para el log
    void setupLogDisplayWidget();
    void showSimulationLog();
};

#endif // CLIENT_H