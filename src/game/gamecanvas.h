#ifndef GAMECANVAS_H
#define GAMECANVAS_H

#include "entities/obstacle.h"

#include <QPixmap>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QVector>
#include <QWidget>

class GameCanvas : public QWidget
{
    Q_OBJECT

public:
    explicit GameCanvas(QWidget *parent = nullptr);

    void reset(double initialBackgroundOffset = 0.0);

public slots:
    void advanceWorld(double distance);
    void setPlayerPosition(const QPointF &position);
    void setPlayerFrame(const QPixmap &frame);
    void setCoins(const QVector<QRectF> &coins);
    void setCoinFrame(const QPixmap &frame);
    void setPowerups(const QVector<QRectF> &powerups);
    void setPowerupFrame(const QPixmap &frame);
    void setMagnetPowerups(const QVector<QRectF> &powerups);
    void setMagnetFrame(const QPixmap &frame);
    void setObstacles(const QVector<Obstacle> &obstacles);
    void setHaqiEffect(const QPixmap &frame, const QPointF &anchor,
                       bool visible);
    void setBigHaqiEffect(const QPixmap &frame, const QRectF &target,
                          const QPointF &anchor, double angleDegrees,
                          bool visible);
    void setAtomicBreathEffect(const QPixmap &frame, const QRectF &target,
                               const QPointF &anchor, double angleDegrees,
                               bool visible);
    void setExplosionEffects(const QVector<QPixmap> &frames,
                             const QVector<QRectF> &targets);
    void setScreenEffects(const QPointF &shakeOffset, double redOverlayAlpha);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QPixmap background;
    double backgroundOffset = 0.0;
    QPointF playerPosition;
    QPixmap playerFrame;
    QRect playerVisibleBounds;
    QPixmap haqiFrame;
    QPointF haqiAnchor;
    bool haqiVisible = false;
    QVector<QPixmap> explosionFrames;
    QVector<QRectF> explosionTargets;
    QPixmap coinFrame;
    QVector<QRectF> coinBounds;
    QVector<QRectF> powerupBounds;
    QPixmap powerupFrame;
    QVector<QRectF> magnetPowerupBounds;
    QPixmap magnetFrame;
    QPixmap bigHaqiFrame;
    QRectF bigHaqiTarget;
    QPointF bigHaqiAnchor;
    double bigHaqiAngleDegrees = 0.0;
    bool bigHaqiVisible = false;
    QPixmap atomicBreathFrame;
    QRectF atomicBreathTarget;
    QPointF atomicBreathAnchor;
    double atomicBreathAngleDegrees = 0.0;
    bool atomicBreathVisible = false;
    QPointF screenShakeOffset;
    double screenRedOverlayAlpha = 0.0;
    QVector<Obstacle> obstacles;
};

#endif // GAMECANVAS_H
