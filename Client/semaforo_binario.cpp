#include "semaforo_binario.h"

void P(semaphore& s, const Process& process, bool& blocked) {
    if (s.value == 1) {
        s.value = 0;
        blocked = false;
    }
    else {
        // Agregar proceso a la cola de espera
        s.q.push(process);
        blocked = true;
    }
}

Process V(semaphore& s, bool& processReleased) {
    Process releasedProcess;
    
    if (s.q.empty()) {
        s.value = 1;
        processReleased = false;
    }
    else {
        // Seleccionar proceso de la cola de espera
        releasedProcess = s.q.front();
        s.q.pop();
        processReleased = true;
        // El semáforo permanece en 0 ya que inmediatamente se asigna a otro proceso
    }
    
    return releasedProcess;
}