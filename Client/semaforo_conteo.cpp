#include "semaforo_conteo.h"

void P(Semaphore& s, const Process& process, bool& blocked) {
    s.value = s.value - 1;
    if (s.value < 0) {
        // Agregar proceso a la cola
        s.q.push(process);
        blocked = true;
    }
    else {
        blocked = false;
    }
}

Process V(Semaphore& s, bool& processReleased) {
    Process releasedProcess;
    s.value = s.value + 1;
    
    if (s.value <= 0) {
        // Remover proceso de la cola
        releasedProcess = s.q.front();
        s.q.pop();
        processReleased = true;
    }
    else {
        processReleased = false;
    }
    
    return releasedProcess;
}