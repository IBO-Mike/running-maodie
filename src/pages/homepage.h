#ifndef HOMEPAGE_H
#define HOMEPAGE_H

#include <QHash>
#include <QPixmap>
#include <QRectF>
#include <QVector>
#include <QWidget>

class QPushButton;
class QLabel;
class QTimer;
class QMouseEvent;
class QPainter;
class QPaintEvent;
class QResizeEvent;

class HomePage : public QWidget
{
    Q_OBJECT

public:
    explicit HomePage(QWidget *parent = nullptr);
    void resetHome();

signals:
    void startGameRequested(double backgroundOffset);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private slots:
    void updateFrame();
    void beginGameTransition();

private:
    enum class HomeState {
        Entering,
        Sit,
        Sleep,
        Falling,
        MovingToGame,
        Finished
    };

    QVector<QPixmap> loadFrames(const QString &action, int count) const;
    const QPixmap &currentCatFrame() const;
    QRect visibleBounds(const QPixmap &frame);
    void advanceAnimation(int frameDurationMs, bool loops);
    void drawBackground(QPainter &painter);
    void drawCat(QPainter &painter);
    void positionControls();
    void updateHighScoreLabel();

    QPixmap background;
    QVector<QPixmap> sitFrames;
    QVector<QPixmap> sleepFrames;
    QVector<QPixmap> fallFrames;
    QVector<QPixmap> runFrames;
    QHash<qint64, QRect> alphaBoundsCache;

    QWidget *logoSpace;
    QLabel *highScoreLabel;
    QPushButton *startButton;
    QPushButton *settingsButton;
    QTimer *timer;

    HomeState state = HomeState::Sit;
    int frameIndex = 0;
    int frameElapsedMs = 0;
    int inactiveElapsedMs = 0;
    double backgroundOffset = 0.0;
    double homeGroupOffsetX = 0.0;
    QPointF catAnchor;
    double fallVelocity = 0.0;
    QRectF catClickArea;
};

#endif // HOMEPAGE_H
