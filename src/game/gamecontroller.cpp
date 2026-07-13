#include "gamecontroller.h"
#include "config/gameconfig.h"
#include "systems/collisionsystem.h"
#include "systems/scoresystem.h"
#include "systems/spawnsystem.h"

#include <QTimer>
#include <QDir>
#include <QImage>
#include <QPainterPath>
#include <QRandomGenerator>
#include <QTransform>
#include <QtMath>

#include <limits>

namespace {
double randomBetween(double minValue, double maxValue)
{
    return minValue
        + QRandomGenerator::global()->generateDouble()
            * (maxValue - minValue);
}
}

GameController::GameController(QObject *parent)
    : QObject(parent)
    , frameTimer(new QTimer(this))
    , collisionSystem(new CollisionSystem(this))
    , spawnSystem(new SpawnSystem(this))
    , scoreSystem(new ScoreSystem(this))
{
    frameTimer->setInterval(GameConfig::FrameIntervalMs);
    loadObstaclePixmaps();
    loadHaqiFrames();
    loadBigHaqiFrames();
    loadAtomicBreathFrames();
    loadCoinFrames();
    loadMagnetFrames();
    loadExplosionClips();
    connect(frameTimer, &QTimer::timeout,
            this, &GameController::updateFrame);
    connect(scoreSystem, &ScoreSystem::scoreChanged,
            this, &GameController::scoreChanged);
}

void GameController::start()
{
    player.reset();
    playerAnimator.reset();
    haqiActive = false;
    haqiFrameIndex = 0;
    haqiFrameElapsedMs = 0;
    haqiHitCount = 0;
    bigHaqiActive = false;
    bigHaqiFrameIndex = 0;
    bigHaqiFrameElapsedMs = 0;
    bigHaqiElapsedMs = 0;
    atomicBreathActive = false;
    atomicBreathFrameIndex = 0;
    atomicBreathElapsedMs = 0;
    playerFacingRight = true;
    coinFrameIndex = 0;
    coinFrameElapsedMs = 0;
    magnetFrameIndex = 0;
    magnetFrameElapsedMs = 0;
    magnetEffectRemainingMs = 0;
    activeExplosions.clear();
    playerAnimator.update(
        0,
        true,
        0.0,
        GameConfig::ObstacleSpeed,
        0.0);
    dying = false;
    scoreSystem->reset();
    spawnSystem->reset();
    resetCoins();
    emit playerPositionChanged(player.position());
    emit playerFrameChanged(playerAnimator.currentFrame());
    if (!coinFrames.isEmpty())
        emit coinFrameChanged(coinFrames[coinFrameIndex]);
    if (!magnetFrames.isEmpty())
        emit magnetFrameChanged(magnetFrames[magnetFrameIndex]);
    emit haqiEffectChanged(QPixmap(), playerMouthPosition(), false);
    emit bigHaqiEffectChanged(QPixmap(), QRectF(), playerMouthPosition(),
                              0.0, false);
    emit atomicBreathEffectChanged(QPixmap(), QRectF(), playerMouthPosition(),
                                   0.0, false);
    publishExplosionEffects();
    emit coinsChanged(coins);
    emit powerupsChanged(powerupBounds());
    emit magnetPowerupsChanged(magnetPowerupBounds());
    publishWorldObjects();
    frameTimer->start();
}

void GameController::resume()
{
    frameTimer->start();
}

void GameController::pause()
{
    frameTimer->stop();
}

void GameController::stop()
{
    frameTimer->stop();
}

void GameController::jump()
{
    if (!frameTimer->isActive())
        return;

    player.jump();
}

void GameController::attack()
{
    if (!frameTimer->isActive())
        return;

    playerAnimator.requestAttack();
    emit playerFrameChanged(playerAnimator.currentFrame());

    startHaqiEffect();
}

bool GameController::isRunning() const
{
    return frameTimer->isActive();
}

void GameController::updateFrame()
{
    updateExplosionEffects();

    if (dying) {
        if (haqiActive) {
            haqiActive = false;
            emit haqiEffectChanged(QPixmap(), playerMouthPosition(), false);
        }
        if (bigHaqiActive) {
            bigHaqiActive = false;
            emit bigHaqiEffectChanged(QPixmap(), QRectF(),
                                      playerMouthPosition(), 0.0, false);
            emit screenEffectsChanged(QPointF(), 0.0);
        }
        if (atomicBreathActive) {
            atomicBreathActive = false;
            emit atomicBreathEffectChanged(QPixmap(), QRectF(),
                                           playerMouthPosition(), 0.0, false);
        }
        playerAnimator.update(
            GameConfig::FrameIntervalMs,
            player.isOnGround(),
            player.verticalSpeed(),
            0.0,
            player.airborneHeight());
        emit playerFrameChanged(playerAnimator.currentFrame());

        if (playerAnimator.isCurrentActionFinished()) {
            frameTimer->stop();
            emit gameOver(scoreSystem->score());
        }
        return;
    }

    for (QRectF &coin : coins)
        coin.translate(-GameConfig::ObstacleSpeed * currentSpeedMultiplier(), 0);
    for (Powerup &powerup : powerups)
        powerup.bounds.translate(-GameConfig::ObstacleSpeed * currentSpeedMultiplier(), 0);
    for (Powerup &powerup : magnetPowerups)
        powerup.bounds.translate(-GameConfig::ObstacleSpeed * currentSpeedMultiplier(), 0);
    for (Obstacle &obstacle : obstacles)
        obstacle.moveByOwnSpeed(currentSpeedMultiplier());
    updatePowerups();
    updateMagnetEffect();
    updateCoinAnimation();
    updateMagnetAnimation();

    if (bigHaqiActive)
        updateBigHaqiPlayerMotion();
    else
        player.update();
    playerAnimator.update(
        GameConfig::FrameIntervalMs,
        player.isOnGround(),
        player.verticalSpeed(),
        GameConfig::ObstacleSpeed,
        player.airborneHeight());
    emit playerFrameChanged(playerAnimator.currentFrame());
    updateHaqiEffect();
    updateBigHaqiEffect();
    updateAtomicBreathEffect();

    if (playerAnimator.takeAttackActivation()) {
        const QRectF attackBounds(
            GameConfig::PlayerX + GameConfig::PlayerWidth - 4,
            player.position().y() + 18,
            44,
            34);
        emit attackHitboxCreated(attackBounds);
    }

    updateWorldObjects(0.0);
    if (dying)
        return;

    if (!frameTimer->isActive())
        return;

    emit playerPositionChanged(player.position());
    emit frameAdvanced(GameConfig::BackgroundSpeed * currentSpeedMultiplier());
    spawnSystem->update();
    collisionSystem->update();
    scoreSystem->update();
}

void GameController::resetCoins()
{
    coins.clear();
    powerups.clear();
    magnetPowerups.clear();
    obstacles.clear();

    double nextCoinX = 420;
    while (nextCoinX < GameConfig::WindowWidth + 700) {
        appendRandomCoin(nextCoinX);
        nextCoinX += QRandomGenerator::global()->bounded(
            GameConfig::CoinMinGap,
            GameConfig::CoinMaxGap + 1);
    }

    double nextX = 780;
    while (nextX < GameConfig::WindowWidth + 700) {
        appendObstacle(nextX);
        nextX = rightmostObstacleX()
            + QRandomGenerator::global()->bounded(
                GameConfig::ObstacleMinGap,
                GameConfig::ObstacleMaxGap + 1);
    }
}

void GameController::updateWorldObjects(double worldDistance)
{
    for (QRectF &coin : coins)
        coin.translate(-worldDistance, 0);
    for (Powerup &powerup : powerups)
        powerup.bounds.translate(-worldDistance, 0);
    for (Powerup &powerup : magnetPowerups)
        powerup.bounds.translate(-worldDistance, 0);
    for (Obstacle &obstacle : obstacles)
        obstacle.moveLeft(worldDistance);

    const QRectF playerBounds = playerCollisionBounds();
    for (const Obstacle &obstacle : obstacles) {
        if (obstacle.collidesWith(playerBounds)) {
            dying = true;
            playerAnimator.requestHurt(0, true);
            emit playerFrameChanged(playerAnimator.currentFrame());
            emit playerPositionChanged(player.position());
            publishWorldObjects();
            return;
        }
    }

    for (qsizetype index = coins.size() - 1; index >= 0; --index) {
        if (collisionSystem->intersects(playerBounds, coins[index])) {
            coins.removeAt(index);
            scoreSystem->addPoints(1);
        } else if (coins[index].right() < 0) {
            coins.removeAt(index);
        }
    }

    for (qsizetype index = powerups.size() - 1; index >= 0; --index) {
        if (collisionSystem->intersects(playerBounds, powerups[index].bounds)) {
            powerups.removeAt(index);
            startBigHaqiEffect();
            emit powerupsChanged(powerupBounds());
        } else if (powerups[index].bounds.right() < 0) {
            powerups.removeAt(index);
            emit powerupsChanged(powerupBounds());
        }
    }

    for (qsizetype index = magnetPowerups.size() - 1; index >= 0; --index) {
        if (collisionSystem->intersects(playerBounds,
                                        magnetPowerups[index].bounds)) {
            magnetPowerups.removeAt(index);
            magnetEffectRemainingMs = GameConfig::MagnetEffectDurationMs;
            emit magnetPowerupsChanged(magnetPowerupBounds());
        } else if (magnetPowerups[index].bounds.right() < 0) {
            magnetPowerups.removeAt(index);
            emit magnetPowerupsChanged(magnetPowerupBounds());
        }
    }

    for (qsizetype index = obstacles.size() - 1; index >= 0; --index) {
        if (obstacles[index].isOutsideScreen())
            obstacles.removeAt(index);
    }

    while (obstacles.isEmpty()
           || rightmostObstacleX() < GameConfig::WindowWidth + 700) {
        const double nextX = obstacles.isEmpty()
            ? GameConfig::WindowWidth + 300
            : rightmostObstacleX()
                + QRandomGenerator::global()->bounded(
                    GameConfig::ObstacleMinGap,
                    GameConfig::ObstacleMaxGap + 1);
        appendObstacle(nextX);
    }

    while (coins.isEmpty()
           || rightmostCoinX() < GameConfig::WindowWidth + 700) {
        const double nextCoinX = coins.isEmpty()
            ? GameConfig::WindowWidth + 180
            : rightmostCoinX()
                + QRandomGenerator::global()->bounded(
                    GameConfig::CoinMinGap,
                    GameConfig::CoinMaxGap + 1);
        appendRandomCoin(nextCoinX);
    }

    emit powerupsChanged(powerupBounds());
    emit magnetPowerupsChanged(magnetPowerupBounds());
    publishWorldObjects();
}

void GameController::appendObstacle(double x)
{
    Obstacle groundObstacle;
    const double carSpeed = randomBetween(
        GameConfig::CarObstacleMinSpeed,
        GameConfig::CarObstacleMaxSpeed);
    if (!carPixmaps.isEmpty()) {
        const int imageIndex = QRandomGenerator::global()->bounded(
            carPixmaps.size());
        groundObstacle = Obstacle(carPixmaps[imageIndex], x, carSpeed);
    } else {
        const int width = QRandomGenerator::global()->bounded(
            GameConfig::ObstacleMinWidth,
            GameConfig::ObstacleMaxWidth + 1);
        const QRectF obstacleBounds(
            x,
            GameConfig::GroundLineY - GameConfig::ObstacleHeight,
            width,
            GameConfig::ObstacleHeight);
        groundObstacle = Obstacle(
            obstacleBounds,
            ObstacleKind::GroundVehicle,
            carSpeed);
    }

    const bool shouldAddAirborne =
        QRandomGenerator::global()->bounded(100)
        < GameConfig::AirborneObstacleChancePercent;
    if (shouldAddAirborne) {
        const double airborneX =
            x + QRandomGenerator::global()->bounded(80, 260);
        const int laneIndex = QRandomGenerator::global()->bounded(
            GameConfig::AirborneObstacleHighestLane,
            GameConfig::AirborneObstacleLowestLane + 1);
        const double planeSpeed = randomBetween(
            GameConfig::AirborneObstacleMinSpeed,
            GameConfig::AirborneObstacleMaxSpeed);
        if (!planePixmaps.isEmpty()) {
            const int imageIndex = QRandomGenerator::global()->bounded(
                planePixmaps.size());
            const QPixmap &plane = planePixmaps[imageIndex];
            const double planeHeight =
                plane.height() * GameConfig::AirborneObstacleVisualScale;
            const QRectF airborneBounds(
                airborneX,
                laneCenterY(laneIndex) - planeHeight / 2.0,
                plane.width() * GameConfig::AirborneObstacleVisualScale,
                planeHeight);
            Obstacle airborneObstacle(
                plane,
                airborneBounds,
                ObstacleKind::Airborne,
                planeSpeed);
            const double amplitude = randomBetween(
                GameConfig::AirborneVerticalMinAmplitude,
                GameConfig::AirborneVerticalMaxAmplitude);
            const double periodMs = randomBetween(
                GameConfig::AirborneVerticalMinPeriodMs,
                GameConfig::AirborneVerticalMaxPeriodMs);
            airborneObstacle.configureVerticalMotion(
                laneCenterY(laneIndex),
                amplitude,
                2.0 * M_PI / periodMs,
                randomBetween(0.0, 2.0 * M_PI),
                GameConfig::AirborneVerticalTopLimitY,
                GameConfig::AirborneVerticalBottomLimitY - planeHeight);
            obstacles.append(airborneObstacle);
        } else {
            const QRectF airborneBounds(
                airborneX,
                laneCenterY(laneIndex)
                    - GameConfig::AirborneObstacleHeight / 2.0,
                GameConfig::AirborneObstacleWidth,
                GameConfig::AirborneObstacleHeight);
            Obstacle airborneObstacle(
                airborneBounds,
                ObstacleKind::Airborne,
                planeSpeed);
            const double amplitude = randomBetween(
                GameConfig::AirborneVerticalMinAmplitude,
                GameConfig::AirborneVerticalMaxAmplitude);
            const double periodMs = randomBetween(
                GameConfig::AirborneVerticalMinPeriodMs,
                GameConfig::AirborneVerticalMaxPeriodMs);
            airborneObstacle.configureVerticalMotion(
                laneCenterY(laneIndex),
                amplitude,
                2.0 * M_PI / periodMs,
                randomBetween(0.0, 2.0 * M_PI),
                GameConfig::AirborneVerticalTopLimitY,
                GameConfig::AirborneVerticalBottomLimitY
                    - GameConfig::AirborneObstacleHeight);
            obstacles.append(airborneObstacle);
        }
    }

    if (QRandomGenerator::global()->bounded(100)
        < GameConfig::BigHaqiPowerupChancePercent) {
        appendRandomPowerup(
            x + QRandomGenerator::global()->bounded(
                GameConfig::BigHaqiPowerupMinOffsetX,
                GameConfig::BigHaqiPowerupMaxOffsetX + 1));
    }

    if (QRandomGenerator::global()->bounded(100)
        < GameConfig::MagnetPowerupChancePercent) {
        appendRandomMagnetPowerup(
            x + QRandomGenerator::global()->bounded(
                GameConfig::MagnetPowerupMinOffsetX,
                GameConfig::MagnetPowerupMaxOffsetX + 1));
    }

    obstacles.append(groundObstacle);
}

void GameController::appendRandomCoin(double x)
{
    const int laneIndex = QRandomGenerator::global()->bounded(
        GameConfig::CoinHighestLane,
        GameConfig::CoinLowestLane + 1);
    coins.append(QRectF(
        x,
        laneCenterY(laneIndex) - GameConfig::CoinSize / 2.0,
        GameConfig::CoinSize,
        GameConfig::CoinSize));
}

void GameController::appendRandomPowerup(double x)
{
    Powerup powerup;
    powerup.bounds = QRectF(
        x,
        GameConfig::GroundLineY - GameConfig::BigHaqiPowerupSize,
        GameConfig::BigHaqiPowerupSize,
        GameConfig::BigHaqiPowerupSize);
    powerups.append(powerup);
    emit powerupsChanged(powerupBounds());
}

void GameController::appendRandomMagnetPowerup(double x)
{
    Powerup powerup;
    powerup.bounds = QRectF(
        x,
        GameConfig::GroundLineY - GameConfig::MagnetPowerupSize,
        GameConfig::MagnetPowerupSize,
        GameConfig::MagnetPowerupSize);
    magnetPowerups.append(powerup);
    emit magnetPowerupsChanged(magnetPowerupBounds());
}

QRectF GameController::playerCollisionBounds() const
{
    const QPixmap &frame = playerAnimator.currentFrame();
    if (frame.isNull()) {
        return QRectF(
            GameConfig::PlayerX,
            player.position().y(),
            GameConfig::PlayerWidth,
            GameConfig::PlayerHeight);
    }

    const QRectF physicsBox(
        GameConfig::PlayerX,
        player.position().y(),
        GameConfig::PlayerWidth,
        GameConfig::PlayerHeight);
    const QPointF feetAnchor(physicsBox.center().x(), physicsBox.bottom());
    const QRect visible = visibleBounds(frame);

    const double visibleCenterX =
        (visible.x() + visible.width() / 2.0)
        * GameConfig::PlayerVisualScale;
    const double visibleBottom =
        (visible.y() + visible.height())
        * GameConfig::PlayerVisualScale;

    const double imageLeft = feetAnchor.x() - visibleCenterX;
    const double imageTop = feetAnchor.y() - visibleBottom;
    return QRectF(
        imageLeft + visible.x() * GameConfig::PlayerVisualScale,
        imageTop + visible.y() * GameConfig::PlayerVisualScale,
        visible.width() * GameConfig::PlayerVisualScale,
        visible.height() * GameConfig::PlayerVisualScale);
}

QRect GameController::visibleBounds(const QPixmap &frame) const
{
    const qint64 key = frame.cacheKey();
    const auto cached = playerVisibleBoundsCache.constFind(key);
    if (cached != playerVisibleBoundsCache.cend())
        return cached.value();

    const QImage image = frame.toImage();
    int left = image.width();
    int top = image.height();
    int right = -1;
    int bottom = -1;

    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (qAlpha(image.pixel(x, y)) == 0)
                continue;

            left = qMin(left, x);
            top = qMin(top, y);
            right = qMax(right, x);
            bottom = qMax(bottom, y);
        }
    }

    const QRect bounds = right >= left && bottom >= top
        ? QRect(QPoint(left, top), QPoint(right, bottom))
        : frame.rect();
    playerVisibleBoundsCache.insert(key, bounds);
    return bounds;
}

QPointF GameController::playerMouthPosition() const
{
    return QPointF(
        GameConfig::PlayerX + GameConfig::PlayerMouthOffsetX,
        player.position().y() + GameConfig::PlayerMouthOffsetY);
}

void GameController::loadHaqiFrames()
{
    haqiFrames.clear();
    haqiFrames.reserve(GameConfig::HaqiFrameCount);

    for (int index = 1; index <= GameConfig::HaqiFrameCount; ++index) {
        QPixmap frame(QStringLiteral(":/images/haqi_white/haqi_white_%1.png")
                          .arg(index, 2, 10, QLatin1Char('0')));
        if (!frame.isNull())
            haqiFrames.append(frame);
    }
}

void GameController::loadBigHaqiFrames()
{
    bigHaqiFrames.clear();
    bigHaqiFrames.reserve(GameConfig::BigHaqiFrameCount);

    for (int index = 1; index <= GameConfig::BigHaqiFrameCount; ++index) {
        QPixmap frame(QStringLiteral(":/images/bighaqi_frames/bighaqi_%1.png")
                          .arg(index, 2, 10, QLatin1Char('0')));
        if (!frame.isNull())
            bigHaqiFrames.append(frame);
    }
}

void GameController::loadAtomicBreathFrames()
{
    atomicBreathFrames.clear();
    atomicBreathFrames.reserve(GameConfig::AtomicBreathFrameCount);

    for (int index = 1; index <= GameConfig::AtomicBreathFrameCount; ++index) {
        QPixmap frame(QStringLiteral(":/images/tuxi_frames/tuxi_%1.png")
                          .arg(index, 2, 10, QLatin1Char('0')));
        if (!frame.isNull())
            atomicBreathFrames.append(frame);
    }
}

void GameController::loadCoinFrames()
{
    coinFrames.clear();
    coinFrames.reserve(GameConfig::CoinFrameCount);

    for (int index = 1; index <= GameConfig::CoinFrameCount; ++index) {
        QPixmap frame(QStringLiteral(":/images/items/coins/coin_%1.png")
                          .arg(index, 2, 10, QLatin1Char('0')));
        if (!frame.isNull())
            coinFrames.append(frame);
    }
}

void GameController::loadMagnetFrames()
{
    magnetFrames.clear();
    magnetFrames.reserve(GameConfig::MagnetFrameCount);

    for (int index = 1; index <= GameConfig::MagnetFrameCount; ++index) {
        QPixmap frame(QStringLiteral(":/images/items/magnets/magnet_%1.png")
                          .arg(index, 2, 10, QLatin1Char('0')));
        if (!frame.isNull())
            magnetFrames.append(frame);
    }
}

void GameController::updateCoinAnimation()
{
    if (coinFrames.isEmpty())
        return;

    coinFrameElapsedMs += GameConfig::FrameIntervalMs;
    bool changed = false;
    while (coinFrameElapsedMs >= GameConfig::CoinFrameDurationMs) {
        coinFrameElapsedMs -= GameConfig::CoinFrameDurationMs;
        coinFrameIndex = (coinFrameIndex + 1) % coinFrames.size();
        changed = true;
    }

    if (changed)
        emit coinFrameChanged(coinFrames[coinFrameIndex]);
}

void GameController::updateMagnetAnimation()
{
    if (magnetFrames.isEmpty())
        return;

    magnetFrameElapsedMs += GameConfig::FrameIntervalMs;
    bool changed = false;
    while (magnetFrameElapsedMs >= GameConfig::MagnetFrameDurationMs) {
        magnetFrameElapsedMs -= GameConfig::MagnetFrameDurationMs;
        magnetFrameIndex = (magnetFrameIndex + 1) % magnetFrames.size();
        changed = true;
    }

    if (changed)
        emit magnetFrameChanged(magnetFrames[magnetFrameIndex]);
}

void GameController::startBigHaqiEffect()
{
    if (atomicBreathFrames.isEmpty())
        return;

    bigHaqiActive = true;
    bigHaqiFrameIndex = 0;
    bigHaqiFrameElapsedMs = 0;
    bigHaqiElapsedMs = 0;
    bigHaqiEntryStartY = player.position().y();
    bigHaqiFlightPeriodMs = randomBetween(
        GameConfig::BigHaqiFlightMinPeriodMs,
        GameConfig::BigHaqiFlightMaxPeriodMs);
    bigHaqiFlightPhase = randomBetween(0.0, 2.0 * M_PI);
    updateBigHaqiPlayerMotion();
    emit bigHaqiEffectChanged(
        orientedAtomicBreathFrame(atomicBreathFrames[bigHaqiFrameIndex]),
        bigHaqiTargetRect(),
        playerMouthPosition(),
        bigHaqiAngleDegrees(),
        true);
    applyBigHaqiAttack();
}

void GameController::updateBigHaqiEffect()
{
    if (!bigHaqiActive || atomicBreathFrames.isEmpty())
        return;

    bigHaqiElapsedMs += GameConfig::FrameIntervalMs;
    if (bigHaqiElapsedMs >= GameConfig::BigHaqiDurationMs) {
        bigHaqiActive = false;
        emit bigHaqiEffectChanged(QPixmap(), QRectF(), playerMouthPosition(),
                                  0.0, false);
        emit screenEffectsChanged(QPointF(), 0.0);
        return;
    }

    bigHaqiFrameElapsedMs += GameConfig::FrameIntervalMs;
    while (bigHaqiFrameElapsedMs >= GameConfig::BigHaqiFrameDurationMs) {
        bigHaqiFrameElapsedMs -= GameConfig::BigHaqiFrameDurationMs;
        bigHaqiFrameIndex =
            (bigHaqiFrameIndex + 1) % atomicBreathFrames.size();
    }

    emit bigHaqiEffectChanged(
        orientedAtomicBreathFrame(atomicBreathFrames[bigHaqiFrameIndex]),
        bigHaqiTargetRect(),
        playerMouthPosition(),
        bigHaqiAngleDegrees(),
        true);
    const double shakeAngle =
        bigHaqiElapsedMs * 2.0 * M_PI / GameConfig::BigHaqiRedFlashPeriodMs;
    const QPointF shakeOffset(
        qSin(shakeAngle) * GameConfig::BigHaqiScreenShakeAmplitude,
        qCos(shakeAngle * 1.37) * GameConfig::BigHaqiScreenShakeAmplitude);
    const double fadeRatio = qMin(
        1.0,
        bigHaqiElapsedMs / GameConfig::BigHaqiRedOverlayFadeInMs);
    const double flashRatio = (qSin(shakeAngle) + 1.0) / 2.0;
    const double redAlpha = fadeRatio
        * (GameConfig::BigHaqiRedOverlayBaseAlpha
           + GameConfig::BigHaqiRedOverlayFlashAlpha * flashRatio);
    emit screenEffectsChanged(shakeOffset, redAlpha);
    applyBigHaqiAttack();
}

void GameController::updateBigHaqiPlayerMotion()
{
    const double hoverElapsedMs = qMax(
        0,
        bigHaqiElapsedMs - GameConfig::BigHaqiEntryTransitionMs);
    const double angle =
        bigHaqiFlightPhase
        + hoverElapsedMs * 2.0 * M_PI / bigHaqiFlightPeriodMs;
    const double hoverY = GameConfig::BigHaqiFlightCenterY
        + qSin(angle) * GameConfig::BigHaqiFlightAmplitude
        - GameConfig::PlayerHeight / 2.0;
    double y = hoverY;
    if (bigHaqiElapsedMs < GameConfig::BigHaqiEntryTransitionMs) {
        const double progress = qBound(
            0.0,
            bigHaqiElapsedMs / double(GameConfig::BigHaqiEntryTransitionMs),
            1.0);
        const double easedProgress =
            progress * progress * (3.0 - 2.0 * progress);
        y = bigHaqiEntryStartY
            + (hoverY - bigHaqiEntryStartY) * easedProgress;
    }
    player.forceAirbornePosition(y);
}

void GameController::updatePowerups()
{
    bool changed = false;
    for (qsizetype index = powerups.size() - 1; index >= 0; --index) {
        powerups[index].elapsedMs += GameConfig::FrameIntervalMs;
        if (powerups[index].elapsedMs >= GameConfig::BigHaqiPowerupLifetimeMs) {
            powerups.removeAt(index);
            changed = true;
        }
    }

    if (changed)
        emit powerupsChanged(powerupBounds());

    bool magnetChanged = false;
    for (qsizetype index = magnetPowerups.size() - 1; index >= 0; --index) {
        magnetPowerups[index].elapsedMs += GameConfig::FrameIntervalMs;
        if (magnetPowerups[index].elapsedMs
            >= GameConfig::MagnetPowerupLifetimeMs) {
            magnetPowerups.removeAt(index);
            magnetChanged = true;
        }
    }

    if (magnetChanged)
        emit magnetPowerupsChanged(magnetPowerupBounds());
}

QVector<QRectF> GameController::powerupBounds() const
{
    QVector<QRectF> bounds;
    bounds.reserve(powerups.size());
    for (const Powerup &powerup : powerups)
        bounds.append(powerup.bounds);
    return bounds;
}

QVector<QRectF> GameController::magnetPowerupBounds() const
{
    QVector<QRectF> bounds;
    bounds.reserve(magnetPowerups.size());
    for (const Powerup &powerup : magnetPowerups)
        bounds.append(powerup.bounds);
    return bounds;
}

void GameController::updateMagnetEffect()
{
    if (magnetEffectRemainingMs <= 0)
        return;

    magnetEffectRemainingMs = qMax(
        0,
        magnetEffectRemainingMs - GameConfig::FrameIntervalMs);

    const QRectF playerBounds = playerCollisionBounds();
    const QPointF target = playerBounds.center();
    bool collected = false;
    for (qsizetype index = coins.size() - 1; index >= 0; --index) {
        if (coins[index].right() < 0
            || coins[index].left() > GameConfig::WindowWidth) {
            continue;
        }

        const QPointF center = coins[index].center();
        const QPointF delta = target - center;
        const double distance = qSqrt(
            delta.x() * delta.x() + delta.y() * delta.y());
        if (distance <= GameConfig::MagnetCoinAttractSpeed
            || playerBounds.intersects(coins[index])) {
            coins.removeAt(index);
            scoreSystem->addPoints(1);
            collected = true;
            continue;
        }

        coins[index].translate(
            delta.x() / distance * GameConfig::MagnetCoinAttractSpeed,
            delta.y() / distance * GameConfig::MagnetCoinAttractSpeed);
    }

    if (collected)
        emit coinsChanged(coins);
}

double GameController::currentSpeedMultiplier() const
{
    return bigHaqiActive ? GameConfig::BigHaqiSpeedMultiplier : 1.0;
}

void GameController::applyBigHaqiAttack()
{
    if (!bigHaqiActive)
        return;

    const QRectF attackArea = bigHaqiTargetRect();
    QPainterPath attackPath;
    attackPath.addPolygon(
        bigHaqiAttackPolygon(attackArea, bigHaqiAngleDegrees()));
    bool removedObstacle = false;
    for (qsizetype index = obstacles.size() - 1; index >= 0; --index) {
        if (!attackPath.intersects(obstacles[index].collisionBounds()))
            continue;

        const Obstacle destroyedObstacle = obstacles[index];
        obstacles.removeAt(index);
        scoreSystem->addPoints(obstacleDestroyScore(destroyedObstacle));
        spawnVehicleExplosion(destroyedObstacle);
        removedObstacle = true;
    }

    if (removedObstacle)
        publishWorldObjects();
}

QRectF GameController::bigHaqiTargetRect() const
{
    const QPointF mouth = playerMouthPosition();
    if (atomicBreathFrames.isEmpty())
        return QRectF(mouth.x(), 0, GameConfig::WindowWidth - mouth.x(),
                      GameConfig::WindowHeight);

    const double targetWidth = GameConfig::WindowWidth - mouth.x();
    const double targetHeight = GameConfig::BigHaqiMinVisualHeight;

    return QRectF(
        mouth.x(),
        mouth.y() - targetHeight / 2.0,
        targetWidth,
        targetHeight);
}

double GameController::bigHaqiAngleDegrees() const
{
    const double angle =
        bigHaqiElapsedMs * 2.0 * M_PI
        / GameConfig::BigHaqiBeamSwingPeriodMs;
    return qSin(angle) * GameConfig::BigHaqiBeamSwingAngleDegrees;
}

QPolygonF GameController::bigHaqiAttackPolygon(
    const QRectF &target,
    double angleDegrees) const
{
    const QPointF anchor = playerMouthPosition();
    QTransform transform;
    transform.translate(anchor.x(), anchor.y());
    transform.rotate(angleDegrees);
    transform.translate(-anchor.x(), -anchor.y());
    return transform.map(QPolygonF(target));
}

void GameController::startAtomicBreathEffect()
{
    if (atomicBreathFrames.isEmpty())
        return;

    atomicBreathActive = true;
    atomicBreathElapsedMs = 0;
    atomicBreathFrameIndex = 0;

    const QPixmap frame =
        orientedAtomicBreathFrame(atomicBreathFrames[atomicBreathFrameIndex]);
    const double angleDegrees = atomicBreathAngleDegrees();
    emit atomicBreathEffectChanged(
        frame,
        atomicBreathTargetRect(frame),
        playerMouthPosition(),
        angleDegrees,
        true);
    applyAtomicBreathAttack();
}

void GameController::updateAtomicBreathEffect()
{
    if (!atomicBreathActive || atomicBreathFrames.isEmpty())
        return;

    atomicBreathElapsedMs += GameConfig::FrameIntervalMs;
    if (atomicBreathElapsedMs >= GameConfig::AtomicBreathDurationMs) {
        atomicBreathActive = false;
        emit atomicBreathEffectChanged(QPixmap(), QRectF(),
                                       playerMouthPosition(), 0.0, false);
        return;
    }

    if (atomicBreathElapsedMs < GameConfig::AtomicBreathExpandDurationMs) {
        const int expandFrameMs =
            GameConfig::AtomicBreathExpandDurationMs
            / GameConfig::AtomicBreathExpandFrameCount;
        const int expandFrameCount = qMin(
            GameConfig::AtomicBreathExpandFrameCount,
            atomicBreathFrames.size());
        atomicBreathFrameIndex = qBound(
            0,
            atomicBreathElapsedMs / expandFrameMs,
            expandFrameCount - 1);
    } else {
        const int loopStartIndex = qMin(
            GameConfig::AtomicBreathLoopStartFrameIndex,
            atomicBreathFrames.size() - 1);
        const int loopFrameCount = atomicBreathFrames.size() - loopStartIndex;
        const int loopElapsed =
            atomicBreathElapsedMs - GameConfig::AtomicBreathExpandDurationMs;
        atomicBreathFrameIndex =
            loopStartIndex
            + (loopElapsed / GameConfig::AtomicBreathFrameDurationMs)
                % loopFrameCount;
    }

    const QPixmap frame =
        orientedAtomicBreathFrame(atomicBreathFrames[atomicBreathFrameIndex]);
    const double angleDegrees = atomicBreathAngleDegrees();
    emit atomicBreathEffectChanged(
        frame,
        atomicBreathTargetRect(frame),
        playerMouthPosition(),
        angleDegrees,
        true);
    applyAtomicBreathAttack();
}

void GameController::applyAtomicBreathAttack()
{
    if (!atomicBreathActive || atomicBreathFrames.isEmpty())
        return;

    const QPixmap frame =
        orientedAtomicBreathFrame(atomicBreathFrames[atomicBreathFrameIndex]);
    const QRectF attackArea = atomicBreathTargetRect(frame);
    QPainterPath attackPath;
    attackPath.addPolygon(
        atomicBreathAttackPolygon(attackArea, atomicBreathAngleDegrees()));
    bool removedObstacle = false;

    for (qsizetype index = obstacles.size() - 1; index >= 0; --index) {
        if (!attackPath.intersects(obstacles[index].collisionBounds()))
            continue;

        const Obstacle destroyedObstacle = obstacles[index];
        obstacles.removeAt(index);
        scoreSystem->addPoints(obstacleDestroyScore(destroyedObstacle));
        spawnVehicleExplosion(destroyedObstacle);
        removedObstacle = true;
    }

    if (removedObstacle)
        publishWorldObjects();
}

QRectF GameController::atomicBreathTargetRect(const QPixmap &frame) const
{
    const QPointF mouth = playerMouthPosition();
    const double targetWidth = playerFacingRight
        ? GameConfig::WindowWidth - mouth.x()
        : mouth.x();

    if (frame.isNull())
        return QRectF(
            playerFacingRight ? mouth.x() : 0.0,
            mouth.y() - GameConfig::AtomicBreathVisualHeight / 2.0,
            targetWidth,
            GameConfig::AtomicBreathVisualHeight);

    const double left = playerFacingRight
        ? mouth.x()
        : mouth.x() - targetWidth;

    return QRectF(
        left,
        mouth.y() - GameConfig::AtomicBreathVisualHeight / 2.0,
        targetWidth,
        GameConfig::AtomicBreathVisualHeight);
}

double GameController::atomicBreathAngleDegrees() const
{
    const double angle =
        atomicBreathElapsedMs * 2.0 * M_PI
        / GameConfig::AtomicBreathSwingPeriodMs;
    const double direction = playerFacingRight ? 1.0 : -1.0;
    return direction
        * qSin(angle)
        * GameConfig::AtomicBreathSwingAngleDegrees;
}

QPolygonF GameController::atomicBreathAttackPolygon(
    const QRectF &target,
    double angleDegrees) const
{
    const QPointF anchor = playerMouthPosition();
    QTransform transform;
    transform.translate(anchor.x(), anchor.y());
    transform.rotate(angleDegrees);
    transform.translate(-anchor.x(), -anchor.y());
    return transform.map(QPolygonF(target));
}

QPixmap GameController::orientedAtomicBreathFrame(const QPixmap &frame) const
{
    if (playerFacingRight || frame.isNull())
        return frame;

    return frame.transformed(QTransform().scale(-1, 1));
}

void GameController::startHaqiEffect()
{
    if (haqiFrames.isEmpty())
        return;

    haqiActive = true;
    haqiFrameIndex = 0;
    haqiFrameElapsedMs = 0;
    haqiHitCount = 0;
    emit haqiEffectChanged(
        haqiFrames[haqiFrameIndex],
        playerMouthPosition(),
        true);
    applyHaqiAttack();
}

void GameController::updateHaqiEffect()
{
    if (!haqiActive || haqiFrames.isEmpty())
        return;

    haqiFrameElapsedMs += GameConfig::FrameIntervalMs;
    while (haqiFrameElapsedMs >= GameConfig::HaqiFrameDurationMs) {
        haqiFrameElapsedMs -= GameConfig::HaqiFrameDurationMs;
        ++haqiFrameIndex;
        if (haqiFrameIndex >= haqiFrames.size()) {
            haqiActive = false;
            emit haqiEffectChanged(QPixmap(), playerMouthPosition(), false);
            return;
        }
    }

    emit haqiEffectChanged(
        haqiFrames[haqiFrameIndex],
        playerMouthPosition(),
        true);
    applyHaqiAttack();
}

void GameController::applyHaqiAttack()
{
    if (!haqiActive)
        return;

    const int currentFrameNumber = haqiFrameIndex + 1;
    if (currentFrameNumber < GameConfig::HaqiActiveStartFrame
        || currentFrameNumber > GameConfig::HaqiActiveEndFrame) {
        return;
    }

    if (haqiHitCount >= GameConfig::HaqiMaxHitsPerAttack)
        return;

    const QPointF mouth = playerMouthPosition();
    const QRectF attackArea = haqiTargetRect(haqiFrames[haqiFrameIndex]);

    qsizetype nearestIndex = -1;
    double nearestDistance = std::numeric_limits<double>::max();
    for (qsizetype index = obstacles.size() - 1; index >= 0; --index) {
        if (!obstacles[index].collidesWith(attackArea))
            continue;

        const double distance = qMax(
            0.0,
            obstacles[index].collisionBounds().left() - mouth.x());
        if (distance < nearestDistance) {
            nearestDistance = distance;
            nearestIndex = index;
        }
    }

    if (nearestIndex >= 0) {
        const Obstacle destroyedObstacle = obstacles[nearestIndex];
        obstacles.removeAt(nearestIndex);
        scoreSystem->addPoints(obstacleDestroyScore(destroyedObstacle));
        spawnVehicleExplosion(destroyedObstacle);
        ++haqiHitCount;
        publishWorldObjects();
    }
}

QRectF GameController::haqiTargetRect(const QPixmap &frame) const
{
    const QPointF mouth = playerMouthPosition();
    if (frame.isNull())
        return QRectF();

    return QRectF(
        mouth.x(),
        mouth.y() - frame.height() / 2.0,
        frame.width(),
        frame.height());
}

void GameController::loadExplosionClips()
{
    explosionClips.clear();

    const QStringList clipNames{
        QStringLiteral("fire_blast"),
        QStringLiteral("fire_smoke"),
        QStringLiteral("ground_blast"),
        QStringLiteral("dust_smoke"),
        QStringLiteral("energy_blast"),
        QStringLiteral("impact_blast")
    };

    for (const QString &clipName : clipNames) {
        QVector<QPixmap> frames;
        frames.reserve(GameConfig::ExplosionFrameCount);
        for (int index = 1; index <= GameConfig::ExplosionFrameCount; ++index) {
            QPixmap frame(
                QStringLiteral(":/images/explodes/%1/%1_%2.png")
                    .arg(clipName)
                    .arg(index, 2, 10, QLatin1Char('0')));
            if (!frame.isNull())
                frames.append(frame);
        }

        if (!frames.isEmpty())
            explosionClips.insert(clipName, frames);
    }
}

void GameController::spawnVehicleExplosion(const Obstacle &obstacle)
{
    const QRectF bounds = obstacle.bounds();
    QString preferredClip;
    QPointF anchor;
    bool bottomAnchor = false;

    if (obstacle.kind() == ObstacleKind::GroundVehicle) {
        preferredClip = QStringLiteral("ground_blast");
        anchor = QPointF(bounds.center().x(), bounds.bottom());
        bottomAnchor = true;
    } else {
        preferredClip = QRandomGenerator::global()->bounded(2) == 0
            ? QStringLiteral("fire_blast")
            : QStringLiteral("fire_smoke");
        anchor = bounds.center();
    }

    QVector<QPixmap> frames = explosionClip(preferredClip);
    if (frames.isEmpty())
        return;

    ActiveExplosion explosion;
    explosion.frames = frames;
    explosion.anchor = anchor;
    explosion.bottomAnchor = bottomAnchor;
    activeExplosions.append(explosion);
    publishExplosionEffects();
}

int GameController::obstacleDestroyScore(const Obstacle &obstacle) const
{
    return obstacle.kind() == ObstacleKind::Airborne
        ? GameConfig::AirborneObstacleDestroyScore
        : GameConfig::GroundVehicleDestroyScore;
}

void GameController::updateExplosionEffects()
{
    bool changed = false;
    for (qsizetype index = activeExplosions.size() - 1; index >= 0; --index) {
        ActiveExplosion &explosion = activeExplosions[index];
        explosion.frameElapsedMs += GameConfig::FrameIntervalMs;

        while (explosion.frameElapsedMs
               >= GameConfig::ExplosionFrameDurationMs) {
            explosion.frameElapsedMs -= GameConfig::ExplosionFrameDurationMs;
            ++explosion.frameIndex;
            changed = true;

            if (explosion.frameIndex >= explosion.frames.size()) {
                activeExplosions.removeAt(index);
                break;
            }
        }
    }

    if (changed)
        publishExplosionEffects();
}

void GameController::publishExplosionEffects()
{
    QVector<QPixmap> frames;
    QVector<QRectF> targets;
    frames.reserve(activeExplosions.size());
    targets.reserve(activeExplosions.size());

    for (const ActiveExplosion &explosion : activeExplosions) {
        if (explosion.frames.isEmpty()
            || explosion.frameIndex >= explosion.frames.size()) {
            continue;
        }

        const QPixmap &frame = explosion.frames[explosion.frameIndex];
        const QSizeF targetSize(
            frame.width() * GameConfig::ExplosionVisualScale,
            frame.height() * GameConfig::ExplosionVisualScale);
        const QPointF topLeft(
            explosion.anchor.x() - targetSize.width() / 2.0,
            explosion.bottomAnchor
                ? explosion.anchor.y() - targetSize.height()
                : explosion.anchor.y() - targetSize.height() / 2.0);

        frames.append(frame);
        targets.append(QRectF(topLeft, targetSize));
    }

    emit explosionEffectsChanged(frames, targets);
}

QVector<QPixmap> GameController::explosionClip(
    const QString &preferredName) const
{
    const QStringList fallbacks{
        preferredName,
        QStringLiteral("fire_blast"),
        QStringLiteral("fire_smoke"),
        QStringLiteral("impact_blast"),
        QStringLiteral("ground_blast"),
        QStringLiteral("dust_smoke"),
        QStringLiteral("energy_blast")
    };

    for (const QString &clipName : fallbacks) {
        const auto iter = explosionClips.constFind(clipName);
        if (iter != explosionClips.cend())
            return iter.value();
    }

    return {};
}

double GameController::laneCenterY(int laneIndex) const
{
    if (GameConfig::LaneCount <= 1)
        return GameConfig::LaneTopY;

    const int clampedLane = qBound(
        0,
        laneIndex,
        GameConfig::LaneCount - 1);
    const double laneSpacing =
        (GameConfig::LaneBottomY - GameConfig::LaneTopY)
        / double(GameConfig::LaneCount - 1);
    return GameConfig::LaneTopY + clampedLane * laneSpacing;
}

double GameController::rightmostObstacleX() const
{
    double rightmost = 0.0;
    for (const Obstacle &obstacle : obstacles)
        rightmost = qMax(rightmost, obstacle.bounds().right());
    return rightmost;
}

double GameController::rightmostCoinX() const
{
    double rightmost = 0.0;
    for (const QRectF &coin : coins)
        rightmost = qMax(rightmost, coin.right());
    return rightmost;
}

void GameController::loadObstaclePixmaps()
{
    carPixmaps.clear();
    planePixmaps.clear();

    const QDir carDir(QStringLiteral(":/images/obstacles/cars"));
    const QStringList carFiles = carDir.entryList(
        QStringList{QStringLiteral("*.png")},
        QDir::Files,
        QDir::Name);
    for (const QString &file : carFiles) {
        QPixmap pixmap(carDir.filePath(file));
        if (!pixmap.isNull())
            carPixmaps.append(pixmap);
    }

    const QDir planeDir(QStringLiteral(":/images/obstacles/planes"));
    const QStringList planeFiles = planeDir.entryList(
        QStringList{QStringLiteral("*.png")},
        QDir::Files,
        QDir::Name);
    for (const QString &file : planeFiles) {
        QPixmap pixmap(planeDir.filePath(file));
        if (!pixmap.isNull())
            planePixmaps.append(pixmap);
    }
}

void GameController::publishWorldObjects()
{
    emit obstaclesChanged(obstacles);
    emit coinsChanged(coins);
}
