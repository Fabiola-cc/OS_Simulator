#include "mutex_synchronizer.h"
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QScrollBar>
#include <QRandomGenerator>
#include <QColor>
#include <algorithm>
#include <pthread.h>
#include <unistd.h>  
#include <iostream>
#include <QSet>

pthread_mutex_t globalMutex = PTHREAD_MUTEX_INITIALIZER;

struct ThreadArgs {
    Process* process;
    MutexSynchronizer* synchronizer;
};



MutexSynchronizer::MutexSynchronizer(QObject *parent) : QObject(parent) {
    ganttScene = new QGraphicsScene(this);
    simulationTimer = new QTimer(this);
    currentTime = 0;
    hasCurrentProcess = false;
    currentProcessRemainingTime = 0;

    connect(simulationTimer, &QTimer::timeout, this, &MutexSynchronizer::updateSimulation);
}

void MutexSynchronizer::setupGanttChart(QGraphicsView *view) {
    ganttView = view;
    ganttView->setScene(ganttScene);
    ganttView->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    ganttView->setMinimumSize(500, 300); 
    ganttView->resize(500, 300);
    ganttView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    ganttScene->clear();

    currentTimeLabel = ganttScene->addText("Current Time: 0");
    currentTimeLabel->setPos(10, 0);

    resourceStatusLabel = ganttScene->addText("Resource R1: Libre");
    resourceStatusLabel->setPos(200, 0);

    int sceneHeight = 30 + processes.size() * 40 + 40; 
    ganttScene->setSceneRect(0, 0, 5000, sceneHeight);

    for (int i = 0; i < processes.size(); i++) {
        int y = 30 + i * 40;
        ganttScene->addLine(0, y, 5000, y, QPen(Qt::white));

        QString pidText = processes[i].pid;

        QGraphicsTextItem *processLabel = ganttScene->addText(pidText);
        processLabel->setPos(5, y);
    }

    // Etiquetas de tiempo desplazadas 30 px a la derecha para no superponerse con procesos
    int timeLabelY = 30 + processes.size() * 40;
    for (int i = 0; i <= 100; i++) {
        int x = i * 30;
        ganttScene->addLine(x, timeLabelY, x, sceneHeight, QPen(Qt::white));

        QGraphicsTextItem *timeLabel = ganttScene->addText(QString::number(i));
        timeLabel->setPos(x + 30 - 5, timeLabelY);  // Muevo +30 px a la derecha
    }
}



void MutexSynchronizer::setProcesses(const QList<Process>& newProcesses) {
    processes = newProcesses;
    std::sort(processes.begin(), processes.end(), [](const Process& a, const Process& b) {
        return a.arrivalTime < b.arrivalTime;
    });
    assignProcessColors();
}

void MutexSynchronizer::setResources(const QList<Resource>& newResources) {
    resources = newResources;
    initializeResources();
}

void MutexSynchronizer::setActions(const QList<Action>& newActions) {
    actions = newActions;
    std::sort(actions.begin(), actions.end(), [](const Action& a, const Action& b) {
        return a.cycle < b.cycle;
    });
}

void MutexSynchronizer::assignProcessColors() {
    processColors.clear();
    for (const Process& p : processes) {
        QColor color;
        do {
            color = QColor(QRandomGenerator::global()->bounded(50, 200),
                           QRandomGenerator::global()->bounded(50, 200),
                           QRandomGenerator::global()->bounded(50, 200));
        } while (color.lightness() > 200);
        processColors[p.pid] = color;
    }
}

int MutexSynchronizer::processIndexByPid(const QString& pid) const {
    for (int i = 0; i < processes.size(); ++i) {
        if (processes[i].pid == pid) {
            return i;
        }
    }
    return -1; // si no lo encuentra
}


void MutexSynchronizer::initializeResources() {
    resourceMutexes.clear();
    for (const Resource& r : resources) {
        resourceMutexes[r.name] = false; // false = unlocked
    }
}

void MutexSynchronizer::startSimulation() {
    readyQueue.clear();
    blockedQueue.clear();
    processExecutionTimes.clear();
    processStartTimes.clear();

    currentTime = 0;

    for (const Process& p : processes) {
        processExecutionTimes[p.pid] = 0;
        processStartTimes[p.pid] = -1;
    }

    simulationTimer->start(800);  // cada 800 ms avanza un ciclo
}



// En tu clase MutexSynchronizer, agrega:
// QSet<QString> blockedLockProcesses;

void MutexSynchronizer::updateSimulation() {
    bool allDone = true;

    std::cout << "Tiempo actual: " << currentTime << std::endl;

    for (int i = 0; i < processes.size(); ++i) {
        Process& p = processes[i];

        if (p.arrivalTime > currentTime) {
            allDone = false;
            std::cout << "Proceso " << p.pid.toStdString()
                      << " aún no ha llegado (llega en t=" << p.arrivalTime << ")" << std::endl;
            continue;
        }

        allDone = false;

        Action* action = nullptr;

        if (blockedLockProcesses.contains(p.pid)) {
            static Action pendingLockAction;
            pendingLockAction.pid = p.pid;
            pendingLockAction.operation = "ADQUIRE";
            pendingLockAction.resource = "";
            pendingLockAction.cycle = currentTime;
            action = &pendingLockAction;
        } else {
            for (Action& a : actions) {
                if (a.pid == p.pid && a.cycle == currentTime) {
                    action = &a;
                    break;
                }
            }
        }

        bool locked = false;
        bool unlocked = false;
        bool blocked = false;

        if (action) {
            std::cout << "Proceso " << p.pid.toStdString()
                      << " tiene acción " << action->operation.toStdString()
                      << " en ciclo " << currentTime << std::endl;

            if (action->operation == "ADQUIRE") {
                if (currentMutexOwner == "") {
                    currentMutexOwner = p.pid;
                    std::cout << "Mutex adquirido por proceso " << p.pid.toStdString() << std::endl;
                    locked = true;
                    blockedLockProcesses.remove(p.pid);
                } else if (currentMutexOwner == p.pid) {
                    blockedLockProcesses.remove(p.pid);
                } else {
                    blockedLockProcesses.insert(p.pid);
                    blocked = true;
                }
            } else if (action->operation == "RELEASE") {
                if (currentMutexOwner == p.pid) {
                    currentMutexOwner = "";
                    unlocked = true;
                }
            }
        }

        int x = (currentTime * 30) + 30;
        int y = 30 + i * 40;

        if (locked) {
            QColor color = Qt::green;
            ganttScene->addRect(x, y, 30, 30, QPen(Qt::black), QBrush(color));
            ganttScene->addText("A")->setPos(x + 5, y + 5);
        } else if (unlocked) {
            QColor color = Qt::blue;
            ganttScene->addRect(x, y, 30, 30, QPen(Qt::black), QBrush(color));
            ganttScene->addText("R")->setPos(x + 5, y + 5);
        } else if (blocked) {
            QColor color = Qt::red;
            ganttScene->addRect(x, y, 30, 30, QPen(Qt::black), QBrush(color));
            ganttScene->addText("B")->setPos(x + 5, y + 5);
        }
    }

    allDone = true;
    for (int i = 0; i < processes.size(); ++i) {
        const Process& p = processes[i];

        if (blockedLockProcesses.contains(p.pid)) {
            allDone = false;
            break;
        }

        bool hasPending = false;
        for (const Action& a : actions) {
            if (a.pid == p.pid && a.cycle >= currentTime) {
                hasPending = true;
                break;
            }
        }

        if (hasPending) {
            allDone = false;
            break;
        }
    }

    if (currentTimeLabel) {
        currentTimeLabel->setPlainText(QString("Current Time: %1").arg(currentTime));
    }

    if (resourceStatusLabel) {
        if (currentMutexOwner.isEmpty()) {
            resourceStatusLabel->setPlainText("Resource R1: Libre");
        } else {
            resourceStatusLabel->setPlainText(QString("Resource R1: Ocupado por %1").arg(currentMutexOwner));
        }
    }
   

    if (allDone) {
        std::cout << "Todos los procesos terminaron. Deteniendo simulación." << std::endl;
        stopSimulation();
    }

    currentTime++;
}





void MutexSynchronizer::stopSimulation() {
    simulationTimer->stop();
    calculateMetrics();
}


void MutexSynchronizer::calculateMetrics() {
    double totalExecutionTime = 0;
    for (const QString& pid : processExecutionTimes.keys()) {
        totalExecutionTime += processExecutionTimes[pid];
    }

    double avgExecutionTime = totalExecutionTime / processes.size();
    emit simulationFinished(avgExecutionTime);
}