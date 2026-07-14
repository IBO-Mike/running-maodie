#include "mainwindow.h"
#include "pages/gamepage.h"
#include "pages/homepage.h"
#include "ui/musicplayerwidget.h"

#include <QResizeEvent>
#include <QStackedWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , pageStack(new QStackedWidget(this))
    , homePage(new HomePage(pageStack))
    , gamePage(new GamePage(pageStack))
    , musicPlayerWidget(new MusicPlayerWidget(pageStack))
{
    setWindowTitle(QStringLiteral("Running Maodie"));
    resize(1100, 700);
    setMinimumSize(800, 520);
    setCentralWidget(pageStack);

    pageStack->addWidget(homePage);
    pageStack->addWidget(gamePage);
    pageStack->setCurrentWidget(homePage);
    positionMusicPlayer();
    musicPlayerWidget->startPlaybackIfAvailable();
    connect(musicPlayerWidget, &MusicPlayerWidget::playlistVisibilityChanged,
            this, &MainWindow::handlePlaylistVisibilityChanged);

    connect(homePage, &HomePage::startGameRequested,
            this, [this](double backgroundOffset) {
        pageStack->setCurrentWidget(gamePage);
        gamePage->startGame(backgroundOffset);
    });

    connect(gamePage, &GamePage::gameStarted,
            this, &MainWindow::prepareMusicPlayerForGame);

    connect(gamePage, &GamePage::backToHomeRequested, this, [this] {
        homePage->resetHome();
        pageStack->setCurrentWidget(homePage);
        musicPlayerWidget->setGameMode(false);
        positionMusicPlayer();
    });
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    positionMusicPlayer();
}

void MainWindow::positionMusicPlayer()
{
    if (!musicPlayerWidget)
        return;

    const int playerX = musicPlayerWidget->isCollapsedToHandle()
        ? pageStack->width() - musicPlayerWidget->collapsedHandleWidth()
        : qMax(18,
               pageStack->width()
                   - musicPlayerWidget->width()
                   - (musicPlayerWidget->isGameMode() ? 0 : 18));
    musicPlayerWidget->move(playerX, 18);
    const int playlistTop = musicPlayerWidget->y()
        + musicPlayerWidget->height();
    musicPlayerWidget->setPlaylistPanelGeometry(
        playlistTop,
        qMax(120, pageStack->height() - playlistTop));
    musicPlayerWidget->raise();
}

void MainWindow::prepareMusicPlayerForGame()
{
    pausedByMusicPlayer = false;
    positionMusicPlayer();
    musicPlayerWidget->setGameMode(true);
}

void MainWindow::handlePlaylistVisibilityChanged(bool visible)
{
    if (pageStack->currentWidget() != gamePage || gamePage->isGameEnded())
        return;

    if (visible) {
        pausedByMusicPlayer = !gamePage->isPaused();
        gamePage->setExternalPaused(true);
    } else if (pausedByMusicPlayer) {
        gamePage->setExternalPaused(false);
        pausedByMusicPlayer = false;
    }
}
