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
        // Operador de comparación
    bool operator==(const Resource& other) const {
        return name == other.name && 
               counter == other.counter;
    }
};

struct Action {
    QString pid;
    QString operation; // e.g. READ, WRITE
    QString resource;
    int cycle;
        
    // Operador de comparación
    bool operator==(const Action& other) const {
        return pid == other.pid && 
               operation == other.operation && 
               resource == other.resource && 
               cycle == other.cycle;
    }
};

struct ActiveMutexAction {
    QString pid;
    QString resource;
    QString actionType; // READ or WRITE
    int startCycle;
    int duration;
};


#endif // STRUCTURES_H