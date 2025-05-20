#include "rr_scheduler.h"
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QScrollBar>
#include <QRandomGenerator>
#include <QColor>
#include <algorithm>
#include <climits> 

RoundRobinScheduler::RoundRobinScheduler(QObject *parent) : QObject(parent) {
    ganttScene = new QGraphicsScene(this);
    simulationTimer = new QTimer(this);
    currentTime = 0;
    quantum = 2;  // Default quantum
    quantumCounter = 0;
    hasCurrentProcess = false;

    connect(simulationTimer, &QTimer::timeout, this, &RoundRobinScheduler::updateSimulation);
}

void RoundRobinScheduler::setupGanttChart(QGraphicsView *view) {
    ganttView = view;
    ganttView->setScene(ganttScene);
    ganttView->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    ganttView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    
    ganttScene->clear();
    ganttScene->setSceneRect(0, 0, 5000, 100);
    ganttScene->addLine(0, 30, 5000, 30, QPen(Qt::white));

    for (int i = 0; i <= 100; i++) {
        int x = i * 30;
        ganttScene->addLine(x, 0, x, 60, QPen(Qt::white));
        QGraphicsTextItem *timeLabel = ganttScene->addText(QString::number(i));
        timeLabel->setPos(x - 5, 65);
    }
}

void RoundRobinScheduler::setProcesses(const QList<Process>& newProcesses) {
    allProcesses = newProcesses;
    std::sort(allProcesses.begin(), allProcesses.end(),
              [](const Process& a, const Process& b) { return a.arrivalTime < b.arrivalTime; });

    remainingBurst.clear();
    for (const Process& p : allProcesses) {
        remainingBurst[p.pid] = p.burstTime;
    }

    assignProcessColors();
}

void RoundRobinScheduler::assignProcessColors() {
    processColors.clear();
    for (const Process& p : allProcesses) {
        QColor color;
        do {
            color = QColor(
                QRandomGenerator::global()->bounded(50, 200),
                QRandomGenerator::global()->bounded(50, 200),
                QRandomGenerator::global()->bounded(50, 200)
            );
        } while (color.lightness() > 200);
        processColors[p.pid] = color;
    }
}

void RoundRobinScheduler::setQuantum(int q) {
    quantum = q;
}

void RoundRobinScheduler::startSimulation() {
    currentTime = 0;
    quantumCounter = 0;
    readyQueue.clear();
    completionTimes.clear();
    waitingTimes.clear();
    hasCurrentProcess = false;
    simulationTimer->start(500);
}

void RoundRobinScheduler::stopSimulation() {
    simulationTimer->stop();
    calculateMetrics();
    emit simulationFinished(waitingTimes.size() > 0
                            ? std::accumulate(waitingTimes.begin(), waitingTimes.end(), 0.0,
                                              [](double sum, auto item) { return sum + item; }) /
                              waitingTimes.size()
                            : 0.0);
}

void RoundRobinScheduler::updateSimulation() {
    // Agregar nuevos procesos al readyQueue
    for (const Process& p : allProcesses) {
        if (p.arrivalTime == currentTime) {
            readyQueue.enqueue(p);
        }
    }

    // Si no hay proceso actual, tomar uno nuevo
    if (!hasCurrentProcess && !readyQueue.isEmpty()) {
        currentProcess = readyQueue.dequeue();
        quantumCounter = 0;
        hasCurrentProcess = true;
    }

    if (hasCurrentProcess) {
        // Ejecutar proceso actual
        remainingBurst[currentProcess.pid]--;
        quantumCounter++;

        // Dibujar ejecución
        int y = 30;
        ganttScene->addRect(currentTime * 30, y, 30, 30,
                            QPen(Qt::black), QBrush(processColors[currentProcess.pid]));
        QGraphicsTextItem *textItem = ganttScene->addText(currentProcess.pid);
        textItem->setPos(currentTime * 30 + 5, y + 5);

        // Si terminó
        if (remainingBurst[currentProcess.pid] == 0) {
            completionTimes[currentProcess.pid] = currentTime + 1;
            hasCurrentProcess = false;
        }
        // Si se agotó el quantum
        else if (quantumCounter >= quantum) {
            readyQueue.enqueue(currentProcess);
            hasCurrentProcess = false;
        }
    } else {
        // Dibujar idle
        int y = 30;
        ganttScene->addRect(currentTime * 30, y, 30, 30,
                            QPen(Qt::black), QBrush(Qt::lightGray));
        QGraphicsTextItem *idleText = ganttScene->addText("idle");
        idleText->setPos(currentTime * 30 + 5, y + 5);
    }

    // Verificar si todos terminaron
    bool allDone = std::all_of(remainingBurst.begin(), remainingBurst.end(),
                                [](int timeLeft) { return timeLeft == 0; });
    if (allDone) {
        stopSimulation();
        return;
    }

    currentTime++;
    ganttView->ensureVisible(currentTime * 30, 0, 100, 0);
}

void RoundRobinScheduler::calculateMetrics() {
    for (const Process& p : allProcesses) {
        int turnaround = completionTimes[p.pid] - p.arrivalTime;
        waitingTimes[p.pid] = turnaround - p.burstTime;
    }
}
