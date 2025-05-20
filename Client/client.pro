QT += widgets

HEADERS += client.h \
           priority_scheduler.h \
           structures.h \ 
           rr_scheduler.h \
           fifo_scheduler.h

SOURCES += client.cpp \
           priority_scheduler.cpp \
           rr_scheduler.cpp \
           fifo_scheduler.cpp