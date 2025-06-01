#include "mutex_synchronizer.h"
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QScrollBar>
#include <QRandomGenerator>
#include <QColor>
#include <algorithm>

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
    ganttScene->setSceneRect(0, 0, 5000, 200);

    for (int i = 0; i < processes.size(); i++) {
        int y = 30 + i * 40;
        ganttScene->addLine(0, y, 5000, y, QPen(Qt::white));
        QGraphicsTextItem *processLabel = ganttScene->addText(processes[i].pid);
        processLabel->setPos(-25, y - 15);
    }

    for (int i = 0; i <= 100; i++) {
        int x = i * 30;
        ganttScene->addLine(x, 0, x, 200, QPen(Qt::white));
        QGraphicsTextItem *timeLabel = ganttScene->addText(QString::number(i));
        timeLabel->setPos(x - 5, 210);
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

void MutexSynchronizer::initializeResources() {
    resourceMutexes.clear();
    for (const Resource& r : resources) {
        resourceMutexes[r.name] = false; // false = unlocked
    }
}

void MutexSynchronizer::startSimulation() {
    currentTime = 0;
    hasCurrentProcess = false;
    currentProcessRemainingTime = 0;
    readyQueue.clear();
    blockedQueue.clear();
    processExecutionTimes.clear();
    processStartTimes.clear();


    for (const Process& p : processes) {
        processExecutionTimes[p.pid] = 0;
        processStartTimes[p.pid] = -1;
    }

    simulationTimer->start(800);
}

void MutexSynchronizer::stopSimulation() {
    simulationTimer->stop();
    calculateMetrics();
}

void MutexSynchronizer::updateSimulation() {
    for (const Process& p : processes) {
        if (p.arrivalTime == currentTime) {
            readyQueue.enqueue(p);
            if (processStartTimes[p.pid] == -1) {
                processStartTimes[p.pid] = currentTime;
            }
        }
    }

    processActions();

    if (!hasCurrentProcess && !readyQueue.isEmpty()) {
        currentProcess = readyQueue.dequeue();
        currentProcessRemainingTime = currentProcess.burstTime;
        hasCurrentProcess = true;
    }

    if (hasCurrentProcess) {
        executeCurrentProcess();
    }

    bool allDone = true;
    for (const Process& p : processes) {
        if (processExecutionTimes[p.pid] < p.burstTime) {
            allDone = false;
            break;
        }
    }

    if (allDone && readyQueue.isEmpty() && blockedQueue.isEmpty()) {
        stopSimulation();
        return;
    }

    currentTime++;
    ganttView->ensureVisible(currentTime * 30, 0, 100, 0);
}

void MutexSynchronizer::processActions() {
    // Primero, libera mutex de acciones que terminaron este ciclo
    for (int i = activeMutexActions.size() - 1; i >= 0; i--) {
        ActiveMutexAction &ama = activeMutexActions[i];
        if (currentTime >= ama.startCycle + ama.duration) {
            // UNLOCK recurso
            resourceMutexes[ama.resource] = false;
            int processIndex = processIndexByPid(ama.pid);
            if (processIndex != -1) {
                int y = 30 + processIndex * 40;
                int x = currentTime * 30;
                ganttScene->addRect(x, y, 50, 30, QPen(Qt::black), QBrush(Qt::green));
                ganttScene->addText("U")->setPos(x + 3, y + 5);
            }
            activeMutexActions.removeAt(i);

            // Desbloquear primer proceso bloqueado (si hay alguno)
            if (!blockedQueue.isEmpty()) {
                readyQueue.enqueue(blockedQueue.dequeue());
            }
        }
    }

    // Procesar acciones programadas para el ciclo actual
    for (const Action& action : actions) {
        if (action.cycle == currentTime) {
            handleMutexOperation(action);
        }
    }
}



void MutexSynchronizer::handleMutexOperation(const Action& action) {
    Process process;
    bool found = false;
    for (const Process& p : processes) {
        if (p.pid == action.pid) {
            process = p;
            found = true;
            break;
        }
    }
    if (!found) return;

    int processIndex = processIndexByPid(action.pid);
    if (processIndex == -1) return;
    int y = 30 + processIndex * 40;
    int x = currentTime * 30;

    if (action.operation == "LOCK") {
        if (!resourceMutexes[action.resource]) {
            resourceMutexes[action.resource] = true;
            // Registrar acción activa con duración 1 ciclo (o más si quieres)
            activeMutexActions.append({action.pid, action.resource, action.operation, currentTime, 1});

            ganttScene->addRect(x, y, 30, 30, QPen(Qt::black), QBrush(Qt::yellow));
            ganttScene->addText("L-" + action.resource)->setPos(x + 3, y + 5);
        } else {
            // Recurso ocupado -> proceso bloqueado
            blockedQueue.enqueue(process);
            ganttScene->addRect(x, y, 30, 30, QPen(Qt::black), QBrush(Qt::red));
            ganttScene->addText("B-" + action.resource)->setPos(x + 1, y + 5);
        }
    }
    else if (action.operation == "UNLOCK") {
        resourceMutexes[action.resource] = false;
        ganttScene->addRect(x, y, 30, 30, QPen(Qt::black), QBrush(Qt::green));
        ganttScene->addText("U-" + action.resource)->setPos(x + 3, y + 5);

        // Desbloquear proceso bloqueado si hay
        if (!blockedQueue.isEmpty()) {
            Process unblocked = blockedQueue.dequeue();
            readyQueue.enqueue(unblocked);
        }
    }
    else if (action.operation == "READ") {
        // Se puede leer solo si está bloqueado por el proceso o el recurso libre
        if (!resourceMutexes[action.resource] || (resourceMutexes[action.resource] && currentProcess.pid == action.pid)) {
            ganttScene->addRect(x, y, 30, 30, QPen(Qt::black), QBrush(Qt::cyan));
            ganttScene->addText("R-" + action.resource)->setPos(x + 6, y + 5);
        } else {
            // Bloqueo si recurso ocupado por otro
            blockedQueue.enqueue(process);
            ganttScene->addRect(x, y, 30, 30, QPen(Qt::black), QBrush(Qt::red));
            ganttScene->addText("B-" + action.resource)->setPos(x + 1, y + 5);
        }
    }
    else if (action.operation == "WRITE") {
        // Similar a READ
        if (!resourceMutexes[action.resource] || (resourceMutexes[action.resource] && currentProcess.pid == action.pid)) {
            ganttScene->addRect(x, y, 30, 30, QPen(Qt::black), QBrush(Qt::magenta));
            ganttScene->addText("W-" + action.resource)->setPos(x + 5, y + 5);
        } else {
            blockedQueue.enqueue(process);
            ganttScene->addRect(x, y, 30, 30, QPen(Qt::black), QBrush(Qt::red));
            ganttScene->addText("B-" + action.resource)->setPos(x + 1, y + 5);
        }
    }
}



int MutexSynchronizer::processIndexByPid(const QString& pid) const {
    for (int i = 0; i < processes.size(); i++) {
        if (processes[i].pid == pid) return i;
    }
    return -1;
}

void MutexSynchronizer::executeCurrentProcess() {
    if (!hasCurrentProcess) return;

    currentProcessRemainingTime--;
    processExecutionTimes[currentProcess.pid]++;

    int processIndex = processIndexByPid(currentProcess.pid);
    if (processIndex != -1) {
        int y = 30 + processIndex * 40;
        ganttScene->addRect(currentTime * 30, y, 30, 30,
                            QPen(Qt::black), QBrush(processColors[currentProcess.pid]));
        ganttScene->addText(currentProcess.pid)->setPos(currentTime * 30 + 5, y + 5);
    }

    if (currentProcessRemainingTime <= 0) {
        hasCurrentProcess = false;
    }
}

void MutexSynchronizer::calculateMetrics() {
    double totalExecutionTime = 0;
    for (const QString& pid : processExecutionTimes.keys()) {
        totalExecutionTime += processExecutionTimes[pid];
    }

    double avgExecutionTime = totalExecutionTime / processes.size();
    emit simulationFinished(avgExecutionTime);
}