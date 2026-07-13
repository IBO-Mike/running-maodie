#include "gamepage.h"
#include "game/gamecanvas.h"
#include "game/gamecontroller.h"
#include "ui/hudwidget.h"

#include <QKeyEvent>
#include <QLabel>
#include <QPushButton>
#include <QResizeEvent>
#include <QSettings>
#include <QVBoxLayout>

GamePage::GamePage(QWidget *parent)
    : QWidget(parent)
    , gameCanvas(new GameCanvas(this))
    , gameController(new GameController(this))
    , hudWidget(new HudWidget(this))
    , pauseOverlay(new QWidget(this))
    , gameOverOverlay(new QWidget(this))
    , finalScoreLabel(new QLabel(gameOverOverlay))
{
    setFocusPolicy(Qt::StrongFocus);
    setStyleSheet(R"(
        QWidget#pauseOverlay {
            background-color: rgba(31, 42, 46, 225);
            border: 2px solid rgba(255, 255, 255, 100);
            border-radius: 22px;
        }
        QWidget#gameOverOverlay {
            background-color: rgba(73, 34, 30, 235);
            border: 2px solid rgba(255, 255, 255, 100);
            border-radius: 22px;
        }
        QLabel#pauseTitle {
            color: white;
            font-size: 34px;
            font-weight: 800;
        }
        QLabel#gameOverTitle {
            color: white;
            font-size: 34px;
            font-weight: 800;
        }
        QLabel#finalScore {
            color: #ffd180;
            font-size: 22px;
            font-weight: 700;
        }
        HudWidget {
            color: white;
            background-color: rgba(25, 35, 40, 150);
            border-radius: 12px;
            font-size: 20px;
            font-weight: 700;
        }
        QPushButton {
            min-width: 160px;
            min-height: 46px;
            border: 0;
            border-radius: 14px;
            color: white;
            background-color: #ef8b61;
            font-size: 17px;
        }
    )");

    pauseOverlay->setObjectName("pauseOverlay");
    pauseOverlay->setFixedSize(380, 270);
    gameOverOverlay->setObjectName("gameOverOverlay");
    gameOverOverlay->setFixedSize(400, 330);

    auto *pauseTitle = new QLabel(QStringLiteral("游戏暂停"), pauseOverlay);
    pauseTitle->setObjectName("pauseTitle");
    pauseTitle->setAlignment(Qt::AlignCenter);
    auto *resumeButton =
        new QPushButton(QStringLiteral("继续游戏"), pauseOverlay);
    auto *backButton =
        new QPushButton(QStringLiteral("返回主页"), pauseOverlay);

    auto *pauseLayout = new QVBoxLayout(pauseOverlay);
    pauseLayout->setContentsMargins(45, 34, 45, 34);
    pauseLayout->setSpacing(16);
    pauseLayout->addWidget(pauseTitle);
    pauseLayout->addStretch();
    pauseLayout->addWidget(resumeButton, 0, Qt::AlignCenter);
    pauseLayout->addWidget(backButton, 0, Qt::AlignCenter);

    auto *gameOverTitle =
        new QLabel(QStringLiteral("游戏结束"), gameOverOverlay);
    gameOverTitle->setObjectName("gameOverTitle");
    gameOverTitle->setAlignment(Qt::AlignCenter);
    finalScoreLabel->setObjectName("finalScore");
    finalScoreLabel->setAlignment(Qt::AlignCenter);
    auto *restartButton =
        new QPushButton(QStringLiteral("重新开始"), gameOverOverlay);
    auto *gameOverBackButton =
        new QPushButton(QStringLiteral("返回主页"), gameOverOverlay);

    auto *gameOverLayout = new QVBoxLayout(gameOverOverlay);
    gameOverLayout->setContentsMargins(45, 34, 45, 34);
    gameOverLayout->setSpacing(16);
    gameOverLayout->addWidget(gameOverTitle);
    gameOverLayout->addWidget(finalScoreLabel);
    gameOverLayout->addStretch();
    gameOverLayout->addWidget(restartButton, 0, Qt::AlignCenter);
    gameOverLayout->addWidget(gameOverBackButton, 0, Qt::AlignCenter);

    connect(gameController, &GameController::frameAdvanced,
            gameCanvas, &GameCanvas::advanceWorld);
    connect(gameController, &GameController::playerPositionChanged,
            gameCanvas, &GameCanvas::setPlayerPosition);
    connect(gameController, &GameController::playerFrameChanged,
            gameCanvas, &GameCanvas::setPlayerFrame);
    connect(gameController, &GameController::coinsChanged,
            gameCanvas, &GameCanvas::setCoins);
    connect(gameController, &GameController::coinFrameChanged,
            gameCanvas, &GameCanvas::setCoinFrame);
    connect(gameController, &GameController::powerupsChanged,
            gameCanvas, &GameCanvas::setPowerups);
    connect(gameController, &GameController::magnetPowerupsChanged,
            gameCanvas, &GameCanvas::setMagnetPowerups);
    connect(gameController, &GameController::magnetFrameChanged,
            gameCanvas, &GameCanvas::setMagnetFrame);
    connect(gameController, &GameController::obstaclesChanged,
            gameCanvas, &GameCanvas::setObstacles);
    connect(gameController, &GameController::haqiEffectChanged,
            gameCanvas, &GameCanvas::setHaqiEffect);
    connect(gameController, &GameController::bigHaqiEffectChanged,
            gameCanvas, &GameCanvas::setBigHaqiEffect);
    connect(gameController, &GameController::atomicBreathEffectChanged,
            gameCanvas, &GameCanvas::setAtomicBreathEffect);
    connect(gameController, &GameController::explosionEffectsChanged,
            gameCanvas, &GameCanvas::setExplosionEffects);
    connect(gameController, &GameController::screenEffectsChanged,
            gameCanvas, &GameCanvas::setScreenEffects);
    connect(gameController, &GameController::scoreChanged,
            hudWidget, &HudWidget::setScore);
    connect(resumeButton, &QPushButton::clicked,
            this, [this] {
                setPaused(false);
                setFocus();
            });
    connect(backButton, &QPushButton::clicked,
            this, [this] {
                gameController->stop();
                pauseOverlay->hide();
                emit backToHomeRequested();
            });
    connect(restartButton, &QPushButton::clicked,
            this, [this] { startGame(); });
    connect(gameOverBackButton, &QPushButton::clicked,
            this, [this] {
                gameOverOverlay->hide();
                emit backToHomeRequested();
            });
    connect(gameController, &GameController::gameOver,
            this, [this](int finalScore) {
                gameEnded = true;
                paused = false;
                pauseOverlay->hide();
                QSettings settings;
                const int previousHighScore =
                    settings.value(QStringLiteral("scores/highScore"), 0)
                        .toInt();
                const bool isNewHighScore = finalScore > previousHighScore;
                const int currentHighScore = isNewHighScore
                    ? finalScore
                    : previousHighScore;
                if (isNewHighScore)
                    settings.setValue(QStringLiteral("scores/highScore"),
                                      finalScore);

                finalScoreLabel->setText(
                    isNewHighScore
                        ? QStringLiteral("最终得分：%1\n新最高得分：%2")
                              .arg(finalScore)
                              .arg(currentHighScore)
                        : QStringLiteral("最终得分：%1\n最高得分：%2")
                              .arg(finalScore)
                              .arg(currentHighScore));
                gameOverOverlay->show();
                gameOverOverlay->raise();
            });

    pauseOverlay->hide();
    gameOverOverlay->hide();
    gameCanvas->lower();
}

void GamePage::startGame(double initialBackgroundOffset)
{
    paused = false;
    gameEnded = false;
    pauseOverlay->hide();
    gameOverOverlay->hide();
    gameCanvas->reset(initialBackgroundOffset);
    gameController->start();
    setFocus();
}

void GamePage::keyPressEvent(QKeyEvent *event)
{
    if (!gameEnded && event->key() == Qt::Key_Escape) {
        setPaused(!paused);
        event->accept();
        return;
    }

    if (!paused && !event->isAutoRepeat()
        && (event->key() == Qt::Key_Space
            || event->key() == Qt::Key_Up)) {
        gameController->jump();
        event->accept();
        return;
    }

    if (!paused && !event->isAutoRepeat()
        && event->key() == Qt::Key_J) {
        gameController->attack();
        event->accept();
        return;
    }

    QWidget::keyPressEvent(event);
}

void GamePage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    gameCanvas->setGeometry(rect());
    hudWidget->setGeometry(20, 20, 190, 55);
    pauseOverlay->move((width() - pauseOverlay->width()) / 2,
                       (height() - pauseOverlay->height()) / 2);
    gameOverOverlay->move((width() - gameOverOverlay->width()) / 2,
                          (height() - gameOverOverlay->height()) / 2);
}

void GamePage::setPaused(bool value)
{
    paused = value;
    if (paused) {
        gameController->pause();
        pauseOverlay->show();
        pauseOverlay->raise();
    } else {
        pauseOverlay->hide();
        gameController->resume();
    }
}
