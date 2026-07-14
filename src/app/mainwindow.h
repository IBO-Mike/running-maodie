#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class GamePage;
class HomePage;
class MusicPlayerWidget;
class QResizeEvent;
class QStackedWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void positionMusicPlayer();
    void prepareMusicPlayerForGame();
    void handlePlaylistVisibilityChanged(bool visible);

    QStackedWidget *pageStack;
    HomePage *homePage;
    GamePage *gamePage;
    MusicPlayerWidget *musicPlayerWidget;
    bool pausedByMusicPlayer = false;
};
#endif // MAINWINDOW_H
