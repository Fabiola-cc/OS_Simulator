#ifndef VISUALIZE_FILES_H
#define VISUALIZE_FILES_H

#include <QDialog>
#include <QTabWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include "structures.h"

class VisualizeFiles : public QDialog {
    Q_OBJECT

public:
    VisualizeFiles(const QList<Process>& processes,
                     const QList<Resource>& resources,
                     const QList<Action>& actions,
                     QWidget *parent = nullptr);

private:
    QTabWidget *tabWidget;
    QTableWidget *processTable;
    QTableWidget *resourceTable;
    QTableWidget *actionTable;

    void setupProcessTab(const QList<Process>& processes);
    void setupResourceTab(const QList<Resource>& resources);
    void setupActionTab(const QList<Action>& actions);
};

// Función auxiliar para ver solo procesos en una tabla individual
QWidget* createProcessTableOnly(const QList<Process>& processes);

#endif // VISUALIZE_FILES_H
