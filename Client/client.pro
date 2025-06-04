QT += widgets

HEADERS += client.h \
           priority_scheduler.h \
           structures.h \ 
           rr_scheduler.h \
           fifo_scheduler.h \
           sjf_scheduler.h \
           srt_scheduler.h \
           counting_semaphore_scheduler.h \
           semaforo_conteo.h \
           mutex_synchronizer.h

SOURCES += client.cpp \
           priority_scheduler.cpp \
           rr_scheduler.cpp \
           fifo_scheduler.cpp \
           sjf_scheduler.cpp \
           srt_scheduler.cpp \
           counting_semaphore_scheduler.cpp \
           semaforo_conteo.cpp \
           mutex_synchronizer.cpp