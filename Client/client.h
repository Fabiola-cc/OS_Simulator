#ifndef CLIENT_H
#define CLIENT_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QLineEdit>

class SimulatorClient : public QWidget {
    Q_OBJECT

public:
    explicit SimulatorClient(QWidget *parent = nullptr);

public slots:
    void onScheduleClicked();
    void onSyncClicked();
    void onCheckBoxMarked();
    void onReturnClicked();

private:
    // Labels
    QLabel *welcomeLabel;
    QLabel *chooseLabel;
    QLabel *titleLabel1;
    QLabel *instrLabel1;
    QLabel *titleLabel2;
    QLabel *instrLabel2;

    // Buttons
    QPushButton *scheduleButton;
    QPushButton *syncButton;
    QPushButton *addFileButton;
    QPushButton *returnButton;
    QPushButton *returnButton2;
    QPushButton *mutexButton;
    QPushButton *semaphoreButton;

    // Checkboxes
    QCheckBox *fcfsCheckBox;
    QCheckBox *sjfCheckBox;
    QCheckBox *srtCheckBox;
    QCheckBox *rrCheckBox;
    QCheckBox *priorityCheckBox;

    // Entrada para Quantum
    QLabel *quantumLabel;
    QLineEdit *quantumInput;

    // Contenedores
    QWidget *scheduleOptionsWidget;
    QWidget *syncOptionsWidget;
};

#endif // CLIENT_H
