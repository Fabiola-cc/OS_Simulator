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
    ganttView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    ganttScene->clear();

    int sceneHeight = 30 + processes.size() * 40 + 40; // extra 40 for time labels
    ganttScene->setSceneRect(0, 0, 5000, sceneHeight);

    for (int i = 0; i < processes.size(); i++) {
        int y = 30 + i * 40;
        ganttScene->addLine(0, y, 5000, y, QPen(Qt::white));

        QString pidText = processes[i].pid; // use directly if already QString
        QGraphicsTextItem *processLabel = ganttScene->addText(pidText);
        processLabel->setPos(-25, y - 15);
    }

    int timeLabelY = 30 + processes.size() * 40; // position below last line
    for (int i = 0; i <= 100; i++) {
        int x = i * 30;
        ganttScene->addLine(x, 0, x, sceneHeight, QPen(Qt::white));

        QGraphicsTextItem *timeLabel = ganttScene->addText(QString::number(i));
        timeLabel->setPos(x - 5, timeLabelY);
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

        if (processExecutionTimes[p.pid] >= p.burstTime) {
            continue;  // proceso terminado
        }

        allDone = false;

        // Acción actual a evaluar
        Action* action = nullptr;

        // Si proceso está bloqueado intentando LOCK, forzamos que la acción sea LOCK
        if (blockedLockProcesses.contains(p.pid)) {
            static Action pendingLockAction;
            pendingLockAction.pid = p.pid;
            pendingLockAction.operation = "LOCK";
            pendingLockAction.resource = ""; // Pon aquí el recurso si es necesario
            pendingLockAction.cycle = currentTime;
            action = &pendingLockAction;
        } else {
            // Buscar acción original en ciclo currentTime
            for (Action& a : actions) {
                if (a.pid == p.pid && a.cycle == currentTime) {
                    action = &a;
                    break;
                }
            }
        }

        bool canAdvance = false;
        bool locked = false;
        bool unlocked = false;

        if (action) {
            std::cout << "Proceso " << p.pid.toStdString()
                      << " tiene acción " << action->operation.toStdString()
                      << " en ciclo " << currentTime << std::endl;

            if (action->operation == "LOCK") {
                if (currentMutexOwner == "") {
                    currentMutexOwner = p.pid;
                    std::cout << "Mutex adquirido por proceso " << p.pid.toStdString() << std::endl;
                    canAdvance = true;
                    locked = true;
                    blockedLockProcesses.remove(p.pid);  // Ya adquirió mutex, quita bloqueo
                } else if (currentMutexOwner == p.pid) {
                    // Ya tiene mutex
                    canAdvance = true;
                    blockedLockProcesses.remove(p.pid);  // Ya no está bloqueado
                } else {
                    std::cout << "Mutex ocupado por proceso "
                              << currentMutexOwner.toStdString() << ", proceso "
                              << p.pid.toStdString() << " bloqueado" << std::endl;
                    canAdvance = false;
                    blockedLockProcesses.insert(p.pid);  // Sigue bloqueado intentando LOCK
                }
            } else if (action->operation == "WRITE") {
                if (currentMutexOwner == p.pid) {
                    canAdvance = true;
                } else {
                    std::cout << "Proceso " << p.pid.toStdString()
                              << " no tiene mutex, no puede escribir" << std::endl;
                    canAdvance = false;
                }
            } else if (action->operation == "UNLOCK") {
                if (currentMutexOwner == p.pid) {
                    currentMutexOwner = "";
                    std::cout << "Mutex liberado por proceso " << p.pid.toStdString() << std::endl;
                    canAdvance = true;
                    unlocked = true;
                } else {
                    canAdvance = false;
                }
            } else {
                // otras acciones o sin impacto en mutex
                canAdvance = true;
            }
        } else {
            // No hay acción, puede avanzar si mutex libre o si es dueño
            canAdvance = true;
        }

        if (canAdvance) {
            if (locked){
                int x = currentTime * 30;
                int y = 30 + i * 40;
                QColor color = Qt::green;
                ganttScene->addRect(x, y, 30, 30, QPen(Qt::black), QBrush(color));
                ganttScene->addText("L")->setPos(x + 5, y + 5);
            }
            else if (unlocked){
                int x = currentTime * 30;
                int y = 30 + i * 40;
                QColor color = Qt::blue;
                ganttScene->addRect(x, y, 30, 30, QPen(Qt::black), QBrush(color));
                ganttScene->addText("U")->setPos(x + 5, y + 5);
            }
            else{
                int x = currentTime * 30;
                int y = 30 + i * 40;
                QColor color = processColors[p.pid];
                ganttScene->addRect(x, y, 30, 30, QPen(Qt::black), QBrush(color));
                ganttScene->addText(p.pid)->setPos(x + 5, y + 5);
            }
         
            processExecutionTimes[p.pid]++;
            std::cout << "Ejecutando proceso " << p.pid.toStdString()
                      << ", ciclo " << processExecutionTimes[p.pid] << "/"
                      << p.burstTime << std::endl;
        } else {
            int x = currentTime * 30;
            int y = 30 + i * 40;

            QColor color = Qt::red;
            ganttScene->addRect(x, y, 30, 30, QPen(Qt::black), QBrush(color));
            ganttScene->addText("B")->setPos(x + 5, y + 5);

            std::cout << "Proceso " << p.pid.toStdString()
                      << " no avanza en ciclo " << currentTime << std::endl;
        }
    }

    if (allDone) {
        std::cout << "Todos los procesos terminaron. Deteniendo simulación." << std::endl;
        simulationTimer->stop();
        calculateMetrics();
    }

    currentTime++;  // avanza tiempo global al final
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