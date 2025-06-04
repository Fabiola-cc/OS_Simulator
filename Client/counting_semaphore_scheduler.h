#ifndef COUNTING_SEMAPHORE_SCHEDULER_H
#define COUNTING_SEMAPHORE_SCHEDULER_H

#include <QObject>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QTimer>
#include <QMap>
#include <QString>
#include <QColor>
#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QScrollArea>
#include <QWidget>
#include <QDebug>
#include "structures.h"
#include <qtextedit.h>

class CountingSemaphoreScheduler : public QObject {
    Q_OBJECT

public:
    explicit CountingSemaphoreScheduler(QObject *parent = nullptr);
    void setupGanttChart(QGraphicsView *view);
    void setupSidePanel(QWidget *parent);
    void setProcesses(const QList<Process> &newProcesses);
    void setResources(const QList<Resource> &newResources);
    void setActions(const QList<Action> &newActions);
    void startSimulation();
    void stopSimulation();
    QString formatLogForDisplay();  // Para formatear el log para mostrar


signals:
    void simulationFinished(double averageExecutionTime);

private slots:
    void updateSimulation();

private:
    // Estructura para acciones activas (que están usando recursos)
    struct ActiveAction {
        QString pid;
        QString resource;
        QString operation;
        int startTime;
        int endTime;
    };

    QWidget *logDisplayWidget;
    QTextEdit *logTextEdit;
    void setupLogDisplayWidget();
    void showSimulationLog();

    QStringList simulationLog;  // Para almacenar todo el log
    void addToLog(const QString& message);  // Para agregar mensajes al log
    
    // Componentes gráficos
    QGraphicsScene *ganttScene;
    QGraphicsView *ganttView;
    QTimer *simulationTimer;
    
    // Labels informativos del Gantt
    QGraphicsTextItem *currentTimeLabel;
    QGraphicsTextItem *resourceStatusLabel;
    QGraphicsTextItem *pendingActionsLabel;
    
    // Panel lateral - Componentes principales
    QWidget *sidePanel;
    QScrollArea *processScrollArea;
    QWidget *processContainer;
    QVBoxLayout *processContainerLayout;
    QListWidget *resourceListWidget;
    QLabel *semaphoreStatusLabel;
    QLabel *currentTimeDisplayLabel;
    
    // Widgets individuales para cada proceso
    QMap<QString, QWidget*> processWidgets;
    QMap<QString, QLabel*> processLabels;
    QMap<QString, QWidget*> pendingActionContainers;
    QMap<QString, QVBoxLayout*> pendingActionLayouts;
    
    // Datos de la simulación
    QList<Process> processes;
    QList<Resource> resources;
    QList<Action> actions;
    
    // Estado de la simulación
    int currentTime;
    QMap<QString, QColor> processColors;
    QMap<QString, int> resourceCounters; // Recursos disponibles por nombre
    
    // Gestión de acciones
    QMap<QString, Action> pendingActions;        // Acciones pendientes por PID
    QMap<QString, Action> currentCycleAccess;    // Accesos concedidos en el ciclo actual
    QMap<QString, Action> currentCycleWaiting;   // Procesos esperando en el ciclo actual
    QMap<QString, ActiveAction> activeActions;   // Acciones que están usando recursos actualmente
    
    // Métodos privados principales
    void assignProcessColors();
    void initializeResources();
    void processCurrentCycleActions();
    void drawAllProcessStates();
    void drawProcessPendingActionsInGantt(const QString& pid, int x, int startY); // NUEVA FUNCIÓN
    void updateInformationLabels();
    void updateSidePanel();
    void createProcessWidgets();
    void updateProcessPendingActions(const QString& pid);
    QString getProcessCurrentState(const QString& pid);
    QColor getProcessStateColor(const QString& state);
    QString getProcessStateSymbol(const QString& state);
    bool checkSimulationComplete();
    
    // Métodos de configuración de la interfaz
    void setupInformationPanel();
    void setupLegend();
    void setupProcessRows(int startY);
    void setupTimeGrid(int startY, int endY);
    void calculateMetrics();
};

#endif // COUNTING_SEMAPHORE_SCHEDULER_H