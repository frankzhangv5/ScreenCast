#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "QtAwesome.h"

#include <QMainWindow>

namespace Ui
{
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

public slots:
    void styleChanged(int index);

private:
    Ui::MainWindow* ui;
    fa::QtAwesome* awesome;
};

#endif // MAINWINDOW_H
