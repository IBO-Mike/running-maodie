#ifndef GAMEPAGE_H
#define GAMEPAGE_H

#include <QWidget>

class QLabel;
class GameController;
class GameCanvas;
class HudWidget;
class QKeyEvent;
class QResizeEvent;

class GamePage : public QWidget
{
    Q_OBJECT

public:
    explicit GamePage(QWidget *parent = nullptr);
    void startGame(double initialBackgroundOffset = 0.0);
    void setExternalPaused(bool paused);
    bool isPaused() const;
    bool isGameEnded() const;

signals:
    void backToHomeRequested();
    void gameStarted();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void setPaused(bool paused);

    GameCanvas *gameCanvas;
    GameController *gameController;
    HudWidget *hudWidget;
    QWidget *pauseOverlay;
    QWidget *gameOverOverlay;
    QLabel *finalScoreLabel;
    bool paused = false;
    bool gameEnded = false;
};

#endif // GAMEPAGE_H
