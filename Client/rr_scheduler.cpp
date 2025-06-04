#include "rr_scheduler.h"
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QScrollBar>
#include <QRandomGenerator>
#include <QColor>
#include <algorithm>
#include <climits> 
#include <iostream>

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
    std::cout << "=== Tiempo actual: " << currentTime << " ===" << std::endl;

    // Agregar nuevos procesos al readyQueue
    for (const Process& p : allProcesses) {
        if (p.arrivalTime == currentTime) {
            readyQueue.enqueue(p);
            std::cout << "Proceso " << p.pid.toStdString() << " llegó y se añadió a la cola de listos." << std::endl;
            std::cout << "ReadyQueue: [ ";
            for (const Process& p : std::as_const(readyQueue))
                std::cout << p.pid.toStdString() << " ";
            std::cout << "]" << std::endl;
        }
    }

    // Insertar procesos que fueron pospuestos para este ciclo
    while (!postponedQueue.isEmpty()) {
        Process postponed = postponedQueue.dequeue();
        readyQueue.enqueue(postponed);
        std::cout << "Proceso " << postponed.pid.toStdString() << " reinsertado en la cola de listos tras agotar quantum." << std::endl;
    }


    // Si no hay proceso actual, tomar uno nuevo
    if (!hasCurrentProcess && !readyQueue.isEmpty()) {
        std::cout << "ReadyQueue: [ ";
        for (const Process& p : std::as_const(readyQueue))
            std::cout << p.pid.toStdString() << " ";
        std::cout << "]" << std::endl;

        currentProcess = readyQueue.dequeue();
        quantumCounter = 0;
        hasCurrentProcess = true;
        std::cout << "Proceso " << currentProcess.pid.toStdString() << " seleccionado para ejecución." << std::endl;
    }

    if (hasCurrentProcess) {
        remainingBurst[currentProcess.pid]--;

        std::cout << "Ejecutando proceso " << currentProcess.pid.toStdString()
                  << " | Quantum usado: " << quantumCounter
                  << " | Burst restante: " << remainingBurst[currentProcess.pid] << std::endl;

        // Dibujar ejecución
        int y = 30;
        ganttScene->addRect(currentTime * 30, y, 30, 30,
                            QPen(Qt::black), QBrush(processColors[currentProcess.pid]));
        QGraphicsTextItem *textItem = ganttScene->addText(currentProcess.pid);
        textItem->setPos(currentTime * 30 + 5, y + 5);

        quantumCounter++;  // Primero se incrementa

        if (remainingBurst[currentProcess.pid] == 0) {
            std::cout << "Proceso " << currentProcess.pid.toStdString() << " ha finalizado." << std::endl;
            completionTimes[currentProcess.pid] = currentTime + 1;
            hasCurrentProcess = false;
        } else if (quantumCounter >= quantum) {  // Compara después de incrementar
            postponedQueue.enqueue(currentProcess);
            hasCurrentProcess = false;
        }


    } else {
        // Dibujar idle
        std::cout << "CPU está inactiva en este ciclo." << std::endl;
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
        std::cout << "Todos los procesos han finalizado. Deteniendo simulación." << std::endl;
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

double RoundRobinScheduler::simulateWithoutGUI() {
    int time = 0;
    QQueue<Process> queue;
    QQueue<Process> postponedQueue;  // Para reinserción diferida
    QMap<QString, int> localRemainingBurst;
    QMap<QString, int> localCompletionTimes;

    // Inicializar burst restante
    for (const Process& p : allProcesses) {
        localRemainingBurst[p.pid] = p.burstTime;
    }

    QList<Process> pendingProcesses = allProcesses;
    bool hasCurrent = false;
    Process currentProc;
    int localQuantumCounter = 0;

    while (true) {
        for (auto it = pendingProcesses.begin(); it != pendingProcesses.end();) {
            if (it->arrivalTime == time) {
                queue.enqueue(*it);
                it = pendingProcesses.erase(it);
            } else {
                ++it;
            }
        }

        // Insertar procesos que agotaron quantum en el ciclo anterior (van al final)
        while (!postponedQueue.isEmpty()) {
            queue.enqueue(postponedQueue.dequeue());
        }


        // Tomar nuevo proceso si no hay uno actual
        if (!hasCurrent && !queue.isEmpty()) {
            currentProc = queue.dequeue();
            localQuantumCounter = 0;
            hasCurrent = true;
        }

        if (hasCurrent) {
            localRemainingBurst[currentProc.pid]--;
            localQuantumCounter++;

            if (localRemainingBurst[currentProc.pid] == 0) {
                localCompletionTimes[currentProc.pid] = time + 1;
                hasCurrent = false;
            } else if (localQuantumCounter >= quantum) {
                postponedQueue.enqueue(currentProc);  // Reinsertar en el siguiente ciclo
                hasCurrent = false;
            }
        }

        // Verificar si todos terminaron
        bool done = std::all_of(localRemainingBurst.begin(), localRemainingBurst.end(),
                                [](int left) { return left == 0; });
        if (done) break;

        time++;
    }

    // Calcular métricas
    QMap<QString, int> localWaitingTimes;
    for (const Process& p : allProcesses) {
        int turnaround = localCompletionTimes[p.pid] - p.arrivalTime;
        localWaitingTimes[p.pid] = turnaround - p.burstTime;
    }

    double avgWaiting = 0.0;
    if (!localWaitingTimes.isEmpty()) {
        int total = std::accumulate(localWaitingTimes.begin(), localWaitingTimes.end(), 0,
                                    [](int sum, int val) { return sum + val; });
        avgWaiting = static_cast<double>(total) / localWaitingTimes.size();
    }

    return avgWaiting;
}
