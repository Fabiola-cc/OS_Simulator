#ifndef SEMAFORO_BINARIO_H
#define SEMAFORO_BINARIO_H

#include <queue>
#include "structures.h"

struct semaphore {
    int value; // 0 o 1
    std::queue<Process> q; // Cola de procesos bloqueados
    
    semaphore() : value(1) {}
};

// Operación P (wait/down)
void P(semaphore& s, const Process& process, bool& blocked);

// Operación V (signal/up)  
Process V(semaphore& s, bool& processReleased);

#endif // SEMAFORO_BINARIO_H