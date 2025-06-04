# 🖥️ Simulador de Algoritmos de Calendarización y Sincronización de Procesos

Un simulador completo e interactivo desarrollado en Qt/C++ que implementa múltiples algoritmos de calendarización de procesos y mecanismos de sincronización con visualización en tiempo real mediante diagramas de Gantt.

## 📋 Tabla de Contenidos

- [🎯 Características](#-características)
- [🛠️ Tecnologías](#️-tecnologías)
- [📦 Instalación](#-instalación)
- [🚀 Uso](#-uso)
- [📊 Algoritmos Implementados](#-algoritmos-implementados)
- [🔄 Mecanismos de Sincronización](#-mecanismos-de-sincronización)
- [📁 Estructura del Proyecto](#-estructura-del-proyecto)
- [📝 Formato de Archivos](#-formato-de-archivos)
- [🎮 Interfaz de Usuario](#-interfaz-de-usuario)
- [🤝 Contribuir](#-contribuir)
- [📄 Licencia](#-licencia)

## 🎯 Características

### ✨ Funcionalidades Principales

- **Simulación Visual Interactiva**: Diagramas de Gantt animados en tiempo real
- **Múltiples Algoritmos**: 5 algoritmos de calendarización diferentes
- **Sincronización Avanzada**: Mutex y semáforos de conteo con visualización
- **Análisis Comparativo**: Métricas detalladas para comparar rendimiento
- **Interfaz Intuitiva**: GUI moderna y fácil de usar
- **Carga de Datos**: Importación de procesos desde archivos de texto
- **Logs Detallados**: Seguimiento completo de la ejecución de la simulación

### 🎨 Características Visuales

- Diagramas de Gantt coloridos y dinámicos
- Panel lateral con información en tiempo real
- Indicadores visuales de estado de procesos
- Leyendas interactivas
- Scroll automático durante la simulación

## 🛠️ Tecnologías

- **Lenguaje**: C++17
- **Framework GUI**: Qt 5.15+
- **Gráficos**: Qt Graphics Framework

## 📦 Instalación

### Prerrequisitos

```bash
# Ubuntu/Debian
sudo apt-get install qt5-default qtcreator build-essential

# CentOS/RHEL/Fedora
sudo dnf install qt5-qtbase-devel qt5-qttools-devel gcc-c++

# macOS (usando Homebrew)
brew install qt@5

# Windows
# Descargar Qt desde https://www.qt.io/download
```

### Compilación

```bash
# Clonar el repositorio
git clone https://github.com/Fabiola-cc/OS_Simulator.git
cd OS_Simulator/Client

# Generar Makefile
qmake 

# Compilar
make

# Ejecutar
./cilent
```


## 🚀 Uso

### Inicio Rápido

1. **Ejecutar la aplicación**
2. **Seleccionar tipo de simulación**:
   - Calendarización de procesos
   - Sincronización (Mutex/Semáforos)
3. **Cargar archivo de datos** (formato especificado abajo)
4. **Configurar parámetros** (quantum para Round Robin)
5. **Iniciar simulación** y observar resultados

### Ejemplos de Archivos de Entrada

#### Procesos (`procesos.txt`)
```
P1,5,0,1
P2,3,1,2
P3,8,2,1
P4,2,3,3
```

#### Recursos (`recursos.txt`)
```
R1,2
R2,1
R3,3
```

#### Acciones (`acciones.txt`)
```
P1,READ,R1,0
P2,WRITE,R1,1
P1,READ,R2,2
P3,WRITE,R2,3
```

## 📊 Algoritmos Implementados

### 🔄 Algoritmos de Calendarización

| Algoritmo | Tipo | Características |
|-----------|------|----------------|
| **FIFO** (First In, First Out) | No expulsivo | Orden de llegada |
| **SJF** (Shortest Job First) | No expulsivo | Menor tiempo de ejecución |
| **SRT** (Shortest Remaining Time) | Expulsivo | Menor tiempo restante |
| **Round Robin** | Expulsivo | Quantum configurable |
| **Priority Scheduling** | Expulsivo | Basado en prioridades |

### 📈 Métricas Calculadas

- **Tiempo de Espera Promedio**: Tiempo que los procesos esperan en cola

## 🔄 Mecanismos de Sincronización

### 🔒 Mutex (Exclusión Mutua)

- **Acceso Exclusivo**: Un solo proceso por recurso
- **Estados Visuales**: 
  - 🟢 ACCESO (proceso usando recurso)
  - 🔴 WAITING (proceso esperando)
- **Operaciones**: READ, WRITE
- **Visualización**: Diagrama de Gantt con estado de recursos

### 🚦 Semáforos de Conteo

- **Múltiples Instancias**: Varios procesos pueden acceder simultaneamente
- **Contador Visual**: Estado en tiempo real de recursos disponibles
- **Panel Lateral**: Información detallada de procesos y colas
- **Log Completo**: Seguimiento detallado de operaciones P() y V()

#### Características del Panel Lateral
- Estado actual de semáforos con indicadores visuales
- Lista de procesos con acciones pendientes
- Información de recursos disponibles/en uso
- Tiempo actual de simulación

## 📁 Estructura del Proyecto

```
OS_Simulator/
├── Client/
│   ├── client.cpp/.h              # Interfaz principal
│   ├── structures.h               # Definición de estructuras
│   ├── fifo_scheduler.cpp/.h
│   ├── sjf_scheduler.cpp/.h
│   ├── srt_scheduler.cpp/.h
│   ├── rr_scheduler.cpp/.h
│   ├── priority_scheduler.cpp/.h
│   ├── mutex_synchronizer.cpp/.h
│   ├── counting_semaphore_scheduler.cpp/.h
│   ├── semaforo_conteo.cpp/.h
│   ├── simulator.pro 
│   └── main.cpp
├── examples/                      # Archivos de ejemplo
│   ├── procesos_ejemplo.txt
│   ├── recursos_ejemplo.txt
│   └── acciones_ejemplo.txt
└── README.md
```

### 🧩 Componentes Principales

#### Schedulers (Calendarizadores)
- `FiFoScheduler`: Implementa FIFO/FCFS
- `ShortestJobFirstScheduler`: Algoritmo SJF
- `ShortestRemainingTimeScheduler`: SRT con expulsión
- `RoundRobinScheduler`: Round Robin con quantum
- `PriorityScheduler`: Calendarización por prioridades

#### Synchronizers (Sincronización)
- `MutexSynchronizer`: Mutex con exclusión mutua
- `CountingSemaphoreScheduler`: Semáforos de conteo avanzados

#### GUI Components
- `SimulatorClient`: Ventana principal y navegación
- `QGraphicsScene/View`: Renderizado de diagramas de Gantt

## 📝 Formato de Archivos

### Estructura de Datos

#### Proceso
```cpp
struct Process {
    QString pid;        // Identificador del proceso
    int burstTime;      // Tiempo de ejecución
    int arrivalTime;    // Tiempo de llegada
    int priority;       // Prioridad (menor número = mayor prioridad)
};
```

#### Recurso
```cpp
struct Resource {
    QString name;       // Nombre del recurso
    int counter;        // Número de instancias disponibles
};
```

#### Acción
```cpp
struct Action {
    QString pid;        // Proceso que ejecuta la acción
    QString operation;  // Tipo de operación (READ/WRITE)
    QString resource;   // Recurso a acceder
    int cycle;          // Ciclo en que se ejecuta
};
```

### Validaciones

- **Consistencia de PIDs**: Todos los procesos referenciados deben existir
- **Recursos Válidos**: Las acciones solo pueden referenciar recursos existentes
- **Operaciones Permitidas**: Solo READ y WRITE para acciones
- **Mutex Constraints**: Recursos con contador = 1 para simulación mutex

## 🎮 Interfaz de Usuario

### 🏠 Pantalla Principal
- Selección entre calendarización y sincronización
- Navegación intuitiva entre módulos

### 📊 Simulación de Calendarización
- Vista única con diagrama de Gantt horizontal
- Etiquetas de tiempo automáticas
- Colores únicos por proceso
- Scroll automático durante ejecución

### 🔄 Simulación de Sincronización
- **Panel Izquierdo**: Diagrama de Gantt con estados
- **Panel Derecho**: Información en tiempo real
  - Estado de semáforos con indicadores visuales
  - Lista expandible de procesos
  - Acciones pendientes por proceso
  - Estado de recursos

### 📈 Pantalla de Resultados
- Métricas comparativas entre algoritmos
- Tiempo de espera promedio
- Gráficos de rendimiento
- Opción de exportar resultados


## 📧 Contacto

- **Desarrolladores**: 
* María José Villafuerte 22129
* Fabiola Contreras 22787
* Diego Duarte 22075


