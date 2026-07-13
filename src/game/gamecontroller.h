#ifndef GAMECONTROLLER_H
#define GAMECONTROLLER_H

#include "entities/player.h"
#include "entities/obstacle.h"
#include "animation/playeranimator.h"

#include <QObject>
#include <QHash>
#include <QPixmap>
#include <QPolygonF>
#include <QRect>
#include <QRectF>
#include <QString>
#include <QVector>

class CollisionSystem;
class ScoreSystem;
class SpawnSystem;
class QTimer;

class GameController : public QObject
{
    Q_OBJECT

public:
    explicit GameController(QObject *parent = nullptr);

    void start();
    void resume();
    void pause();
    void stop();
    void jump();
    void attack();
    bool isRunning() const;

signals:
    void frameAdvanced(double worldDistance);
    void playerPositionChanged(const QPointF &position);
    void playerFrameChanged(const QPixmap &frame);
    void attackHitboxCreated(const QRectF &bounds);
    void coinsChanged(const QVector<QRectF> &coins);
    void coinFrameChanged(const QPixmap &frame);
    void powerupsChanged(const QVector<QRectF> &powerups);
    void scoreChanged(int score);
    void obstaclesChanged(const QVector<Obstacle> &obstacles);
    void haqiEffectChanged(const QPixmap &frame, const QPointF &anchor,
                           bool visible);
    void bigHaqiEffectChanged(const QPixmap &frame, const QRectF &target,
                              bool visible);
    void atomicBreathEffectChanged(const QPixmap &frame, const QRectF &target,
                                   const QPointF &anchor, double angleDegrees,
                                   bool visible);
    void explosionEffectsChanged(const QVector<QPixmap> &frames,
                                 const QVector<QRectF> &targets);
    void screenEffectsChanged(const QPointF &shakeOffset,
                              double redOverlayAlpha);
    void gameOver(int finalScore);

private slots:
    void updateFrame();

private:
    void resetCoins();
    void updateWorldObjects(double worldDistance);
    void appendObstacle(double x);
    void appendRandomCoin(double x);
    void appendRandomPowerup(double x);
    QRectF playerCollisionBounds() const;
    QRect visibleBounds(const QPixmap &frame) const;
    QPointF playerMouthPosition() const;
    void loadHaqiFrames();
    void loadBigHaqiFrames();
    void loadAtomicBreathFrames();
    void loadCoinFrames();
    void loadExplosionClips();
    void updateCoinAnimation();
    void startHaqiEffect();
    void updateHaqiEffect();
    void applyHaqiAttack();
    void startBigHaqiEffect();
    void updateBigHaqiEffect();
    void applyBigHaqiAttack();
    void startAtomicBreathEffect();
    void updateAtomicBreathEffect();
    void applyAtomicBreathAttack();
    void updateBigHaqiPlayerMotion();
    void updatePowerups();
    QVector<QRectF> powerupBounds() const;
    double currentSpeedMultiplier() const;
    QRectF bigHaqiTargetRect() const;
    QRectF atomicBreathTargetRect(const QPixmap &frame) const;
    double atomicBreathAngleDegrees() const;
    QPolygonF atomicBreathAttackPolygon(const QRectF &target,
                                        double angleDegrees) const;
    QPixmap orientedAtomicBreathFrame(const QPixmap &frame) const;
    void spawnVehicleExplosion(const Obstacle &obstacle);
    int obstacleDestroyScore(const Obstacle &obstacle) const;
    void updateExplosionEffects();
    void publishExplosionEffects();
    QVector<QPixmap> explosionClip(const QString &preferredName) const;
    double laneCenterY(int laneIndex) const;
    double rightmostObstacleX() const;
    double rightmostCoinX() const;
    void loadObstaclePixmaps();
    void publishWorldObjects();

    QTimer *frameTimer;
    CollisionSystem *collisionSystem;
    SpawnSystem *spawnSystem;
    ScoreSystem *scoreSystem;
    Player player;
    PlayerAnimator playerAnimator;
    QVector<QRectF> coins;
    struct Powerup {
        QRectF bounds;
        int elapsedMs = 0;
    };
    QVector<Powerup> powerups;
    QVector<Obstacle> obstacles;
    QVector<QPixmap> carPixmaps;
    QVector<QPixmap> planePixmaps;
    QVector<QPixmap> haqiFrames;
    QVector<QPixmap> bigHaqiFrames;
    QVector<QPixmap> atomicBreathFrames;
    QVector<QPixmap> coinFrames;
    QHash<QString, QVector<QPixmap>> explosionClips;
    mutable QHash<qint64, QRect> playerVisibleBoundsCache;

    struct ActiveExplosion {
        QVector<QPixmap> frames;
        QPointF anchor;
        bool bottomAnchor = false;
        int frameIndex = 0;
        int frameElapsedMs = 0;
    };
    QVector<ActiveExplosion> activeExplosions;

    bool haqiActive = false;
    int haqiFrameIndex = 0;
    int haqiFrameElapsedMs = 0;
    int haqiHitCount = 0;
    bool bigHaqiActive = false;
    int bigHaqiFrameIndex = 0;
    int bigHaqiFrameElapsedMs = 0;
    int bigHaqiElapsedMs = 0;
    double bigHaqiFlightPeriodMs = 300.0;
    double bigHaqiFlightPhase = 0.0;
    bool atomicBreathActive = false;
    int atomicBreathFrameIndex = 0;
    int atomicBreathElapsedMs = 0;
    int normalAttackCount = 0;
    bool playerFacingRight = true;
    int coinFrameIndex = 0;
    int coinFrameElapsedMs = 0;
    bool dying = false;
};

#endif // GAMECONTROLLER_H
