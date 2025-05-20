#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <QString>

struct Process {
    QString pid;
    int burstTime;
    int arrivalTime;
    int priority;
    
    // Definir el operador de comparación para poder usar indexOf
    bool operator==(const Process& other) const {
        return pid == other.pid && 
               burstTime == other.burstTime && 
               arrivalTime == other.arrivalTime && 
               priority == other.priority;
    }
};

struct Resource {
    QString name;
    int counter;
};

struct Action {
    QString pid;
    QString operation; // e.g. READ, WRITE
    QString resource;
    int cycle;
};

#endif // STRUCTURES_H