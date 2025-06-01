#include <QString>

struct Process {
    QString pid;
    int burstTime;
    int arrivalTime;
    int priority;
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

struct ActiveMutexAction {
    QString pid;
    QString resource;
    QString actionType; // READ or WRITE
    int startCycle;
    int duration; // 1 ciclo para la acción
};

