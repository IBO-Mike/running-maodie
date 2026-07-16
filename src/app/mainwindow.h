#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class GamePage;
class QLabel;
class QGraphicsOpacityEffect;
class HomePage;
class MusicPlayerWidget;
class QPropertyAnimation;
class QResizeEvent;
class QStackedWidget;
class QWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void buildSplashOverlay();
    void positionSplashOverlay();
    void updateSplashLogoPixmap();
    void startSplashFade();
    void positionMusicPlayer();
    void prepareMusicPlayerForGame();
    void handlePlaylistVisibilityChanged(bool visible);

    QStackedWidget *pageStack;
    HomePage *homePage;
    GamePage *gamePage;
    MusicPlayerWidget *musicPlayerWidget;
    QWidget *splashOverlay;
    QLabel *splashLogoLabel;
    QGraphicsOpacityEffect *splashOpacityEffect;
    QPropertyAnimation *splashFadeAnimation;
    bool pausedByMusicPlayer = false;
};
#endif // MAINWINDOW_H
