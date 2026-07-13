#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class GamePage;
class HomePage;
class QStackedWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    QStackedWidget *pageStack;
    HomePage *homePage;
    GamePage *gamePage;
};
#endif // MAINWINDOW_H
