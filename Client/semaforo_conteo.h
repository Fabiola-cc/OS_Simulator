#ifndef SEMAFORO_CONTEO_H
#define SEMAFORO_CONTEO_H

#include <queue>
#include "structures.h"

struct Semaphore {
    int value;
    std::queue<Process> q; // Cola de procesos bloqueados
    
    Semaphore(int initialValue = 1) : value(initialValue) {}
};

// Operación P (wait/down)
void P(Semaphore& s, const Process& process, bool& blocked);

// Operación V (signal/up)
Process V(Semaphore& s, bool& processReleased);

#endif // SEMAFORO_CONTEO_H