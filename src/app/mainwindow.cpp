#include "mainwindow.h"
#include "pages/gamepage.h"
#include "pages/homepage.h"
#include "ui/musicplayerwidget.h"

#include <QGraphicsOpacityEffect>
#include <QLabel>
#include <QPixmap>
#include <QPropertyAnimation>
#include <QResizeEvent>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>

namespace {
constexpr int SplashHoldMs = 4000;
constexpr int SplashFadeMs = 1800;
constexpr int SplashLogoMinWidth = 260;
constexpr int SplashLogoMaxWidth = 440;
constexpr double SplashLogoWidthRatio = 0.36;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , pageStack(new QStackedWidget(this))
    , homePage(new HomePage(pageStack))
    , gamePage(new GamePage(pageStack))
    , musicPlayerWidget(new MusicPlayerWidget(pageStack))
    , splashOverlay(nullptr)
    , splashLogoLabel(nullptr)
    , splashOpacityEffect(nullptr)
    , splashFadeAnimation(nullptr)
{
    setWindowTitle(QStringLiteral("Running Maodie"));
    resize(1100, 700);
    setMinimumSize(800, 520);
    setCentralWidget(pageStack);

    pageStack->addWidget(homePage);
    pageStack->addWidget(gamePage);
    pageStack->setCurrentWidget(homePage);
    positionMusicPlayer();
    buildSplashOverlay();
    QTimer::singleShot(SplashHoldMs, this, &MainWindow::startSplashFade);

    connect(musicPlayerWidget, &MusicPlayerWidget::playlistVisibilityChanged,
            this, &MainWindow::handlePlaylistVisibilityChanged);

    connect(homePage, &HomePage::startGameRequested,
            this, [this](double backgroundOffset) {
        pageStack->setCurrentWidget(gamePage);
        gamePage->startGame(backgroundOffset);
    });

    connect(gamePage, &GamePage::gameStarted,
            this, &MainWindow::prepareMusicPlayerForGame);
    connect(gamePage, &GamePage::pauseChanged,
            this, [this](bool paused) {
        if (!paused && pageStack->currentWidget() == gamePage)
            musicPlayerWidget->collapseForGameplay(true);
    });

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
    positionSplashOverlay();
}

void MainWindow::buildSplashOverlay()
{
    splashOverlay = new QWidget(pageStack);
    splashOverlay->setObjectName(QStringLiteral("splashOverlay"));
    splashOverlay->setStyleSheet(QStringLiteral(
        "QWidget#splashOverlay { background-color: black; }"));

    splashLogoLabel = new QLabel(splashOverlay);
    splashLogoLabel->setAlignment(Qt::AlignCenter);
    splashLogoLabel->setAttribute(Qt::WA_TransparentForMouseEvents);

    auto *layout = new QVBoxLayout(splashOverlay);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addStretch(1);
    layout->addWidget(splashLogoLabel, 0, Qt::AlignCenter);
    layout->addStretch(1);

    splashOpacityEffect = new QGraphicsOpacityEffect(splashOverlay);
    splashOpacityEffect->setOpacity(1.0);
    splashOverlay->setGraphicsEffect(splashOpacityEffect);

    splashFadeAnimation =
        new QPropertyAnimation(splashOpacityEffect, "opacity", this);
    splashFadeAnimation->setDuration(SplashFadeMs);
    splashFadeAnimation->setStartValue(1.0);
    splashFadeAnimation->setEndValue(0.0);
    splashFadeAnimation->setEasingCurve(QEasingCurve::InOutCubic);
    connect(splashFadeAnimation, &QPropertyAnimation::finished, this, [this] {
        splashOverlay->hide();
        splashOverlay->deleteLater();
        splashOverlay = nullptr;
        splashLogoLabel = nullptr;
        splashOpacityEffect = nullptr;
        positionMusicPlayer();
        musicPlayerWidget->startPlaybackIfAvailable();
    });

    positionSplashOverlay();
    splashOverlay->show();
    splashOverlay->raise();
}

void MainWindow::positionSplashOverlay()
{
    if (!splashOverlay)
        return;

    splashOverlay->setGeometry(pageStack->rect());
    updateSplashLogoPixmap();
    splashOverlay->raise();
}

void MainWindow::updateSplashLogoPixmap()
{
    if (!splashLogoLabel)
        return;

    const QPixmap logo(QStringLiteral(":/images/ui/logo_cutout.png"));
    if (logo.isNull()) {
        splashLogoLabel->setText(QStringLiteral("Running Maodie"));
        splashLogoLabel->setStyleSheet(QStringLiteral(
            "color: white; font-size: 40px; font-weight: 800;"));
        return;
    }

    const int targetWidth =
        qBound(SplashLogoMinWidth,
               int(pageStack->width() * SplashLogoWidthRatio),
               SplashLogoMaxWidth);
    splashLogoLabel->setPixmap(logo.scaled(targetWidth,
                                           pageStack->height() / 3,
                                           Qt::KeepAspectRatio,
                                           Qt::SmoothTransformation));
}

void MainWindow::startSplashFade()
{
    if (!splashFadeAnimation || !splashOverlay)
        return;

    splashFadeAnimation->start();
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
    if (splashOverlay && splashOverlay->isVisible())
        splashOverlay->raise();
}

void MainWindow::prepareMusicPlayerForGame()
{
    pausedByMusicPlayer = false;
    positionMusicPlayer();
    musicPlayerWidget->collapseForGameplay(true);
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
