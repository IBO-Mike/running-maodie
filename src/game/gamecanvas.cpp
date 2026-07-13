#include "gamecanvas.h"
#include "config/gameconfig.h"

#include <QPainter>
#include <QImage>
#include <QtMath>

GameCanvas::GameCanvas(QWidget *parent)
    : QWidget(parent)
    , background(QStringLiteral(":/images/backgrounds/background.png"))
    , playerPosition(GameConfig::PlayerX, GameConfig::GroundY)
{
    setAttribute(Qt::WA_OpaquePaintEvent);
}

void GameCanvas::reset(double initialBackgroundOffset)
{
    backgroundOffset = initialBackgroundOffset;
    playerPosition = QPointF(GameConfig::PlayerX, GameConfig::GroundY);
    coinBounds.clear();
    powerupBounds.clear();
    obstacles.clear();
    haqiVisible = false;
    haqiFrame = QPixmap();
    bigHaqiVisible = false;
    bigHaqiFrame = QPixmap();
    atomicBreathVisible = false;
    atomicBreathFrame = QPixmap();
    atomicBreathAnchor = QPointF();
    atomicBreathAngleDegrees = 0.0;
    screenShakeOffset = QPointF();
    screenRedOverlayAlpha = 0.0;
    explosionFrames.clear();
    explosionTargets.clear();
    update();
}

void GameCanvas::advanceWorld(double distance)
{
    if (background.isNull() || height() <= 0)
        return;

    const double displayedWidth =
        background.width() * (height() / double(background.height()));

    backgroundOffset -= distance;
    if (backgroundOffset <= -displayedWidth)
        backgroundOffset += displayedWidth;

    update();
}

void GameCanvas::setPlayerPosition(const QPointF &position)
{
    playerPosition = position;
    update();
}

void GameCanvas::setPlayerFrame(const QPixmap &frame)
{
    if (playerFrame.cacheKey() == frame.cacheKey())
        return;

    playerFrame = frame;

    const QImage image = playerFrame.toImage();
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

    playerVisibleBounds = right >= left && bottom >= top
        ? QRect(QPoint(left, top), QPoint(right, bottom))
        : playerFrame.rect();
    update();
}

void GameCanvas::setCoins(const QVector<QRectF> &coins)
{
    coinBounds = coins;
    update();
}

void GameCanvas::setCoinFrame(const QPixmap &frame)
{
    coinFrame = frame;
    update();
}

void GameCanvas::setPowerups(const QVector<QRectF> &powerups)
{
    powerupBounds = powerups;
    update();
}

void GameCanvas::setObstacles(const QVector<Obstacle> &newObstacles)
{
    obstacles = newObstacles;
    update();
}

void GameCanvas::setHaqiEffect(const QPixmap &frame, const QPointF &anchor,
                               bool visible)
{
    haqiFrame = frame;
    haqiAnchor = anchor;
    haqiVisible = visible;
    update();
}

void GameCanvas::setBigHaqiEffect(const QPixmap &frame, const QRectF &target,
                                  bool visible)
{
    bigHaqiFrame = frame;
    bigHaqiTarget = target;
    bigHaqiVisible = visible;
    update();
}

void GameCanvas::setAtomicBreathEffect(const QPixmap &frame,
                                       const QRectF &target,
                                       const QPointF &anchor,
                                       double angleDegrees,
                                       bool visible)
{
    atomicBreathFrame = frame;
    atomicBreathTarget = target;
    atomicBreathAnchor = anchor;
    atomicBreathAngleDegrees = angleDegrees;
    atomicBreathVisible = visible;
    update();
}

void GameCanvas::setExplosionEffects(const QVector<QPixmap> &frames,
                                     const QVector<QRectF> &targets)
{
    explosionFrames = frames;
    explosionTargets = targets;
    update();
}

void GameCanvas::setScreenEffects(const QPointF &shakeOffset,
                                  double redOverlayAlpha)
{
    screenShakeOffset = shakeOffset;
    screenRedOverlayAlpha = qBound(0.0, redOverlayAlpha, 255.0);
    update();
}

void GameCanvas::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.fillRect(rect(), QColor("#9ddce3"));
    painter.save();
    painter.translate(screenShakeOffset);

    if (!background.isNull()) {
        const double displayedWidth =
            background.width() * (height() / double(background.height()));
        const QRectF sourceRect(background.rect());

        for (double x = backgroundOffset; x < width(); x += displayedWidth) {
            painter.drawPixmap(
                QRectF(x, 0, displayedWidth + 1.0, height()),
                background,
                sourceRect);
        }
    }

    // 障碍物：地面汽车用图片显示，空中障碍物先用占位碰撞箱显示。
    for (const Obstacle &obstacle : obstacles) {
        if (!obstacle.pixmap().isNull()) {
            painter.drawPixmap(obstacle.bounds(),
                               obstacle.pixmap(),
                               QRectF(obstacle.pixmap().rect()));
        } else if (obstacle.kind() == ObstacleKind::Airborne) {
            painter.setBrush(QColor(97, 154, 214, 145));
            painter.setPen(QPen(QColor("#1e5f99"), 2, Qt::SolidLine));
            painter.drawRoundedRect(obstacle.bounds(), 12, 12);
            painter.setFont(QFont(QStringLiteral("Sans Serif"), 11,
                                  QFont::DemiBold));
            painter.drawText(obstacle.bounds(), Qt::AlignCenter,
                             QStringLiteral("空中障碍"));
        } else {
            painter.setBrush(QColor(98, 82, 72, 190));
            painter.setPen(QPen(QColor("#4d4038"), 2, Qt::SolidLine));
            painter.drawRoundedRect(obstacle.bounds(), 5, 5);
        }

        // 调试用：显示 PNG 非透明像素的外接碰撞范围。
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(QColor(33, 27, 23, 150), 2, Qt::DashLine));
        painter.drawRoundedRect(obstacle.collisionBounds(), 4, 4);
    }

    // 金币动画：金币矩形中心作为贴图锚点，碰撞仍使用 coinBounds。
    for (const QRectF &coin : coinBounds) {
        if (!coinFrame.isNull()) {
            const QRectF target(
                coin.center().x() - GameConfig::CoinVisualSize / 2.0,
                coin.center().y() - GameConfig::CoinVisualSize / 2.0,
                GameConfig::CoinVisualSize,
                GameConfig::CoinVisualSize);
            painter.drawPixmap(target, coinFrame, QRectF(coinFrame.rect()));
        } else {
            painter.setBrush(QColor(255, 202, 40, 150));
            painter.setPen(QPen(QColor("#b77900"), 3, Qt::DashLine));
            painter.drawRoundedRect(coin, 8, 8);
        }
    }

    // 临时强化哈气道具占位：使用明显色块，后续可替换为正式道具贴图。
    painter.setBrush(QColor("#e53935"));
    painter.setPen(QPen(QColor("#8e0000"), 3, Qt::SolidLine));
    painter.setFont(QFont(QStringLiteral("Sans Serif"), 10,
                          QFont::DemiBold));
    for (const QRectF &powerup : powerupBounds) {
        painter.drawRect(powerup);
        painter.drawText(powerup, Qt::AlignCenter, QStringLiteral("哈"));
    }

    // 图片与物理坐标解耦：所有动作都锚定在碰撞箱脚底中心。
    const QRectF playerBox(
        GameConfig::PlayerX,
        playerPosition.y(),
        GameConfig::PlayerWidth,
        GameConfig::PlayerHeight);
    QRectF playerCollisionBox = playerBox;
    if (!playerFrame.isNull()) {
        const QSizeF displaySize(
            playerFrame.width() * GameConfig::PlayerVisualScale,
            playerFrame.height() * GameConfig::PlayerVisualScale);
        const QPointF feetAnchor(
            playerBox.center().x(),
            playerBox.bottom());

        const double visibleCenterX =
            (playerVisibleBounds.x()
             + playerVisibleBounds.width() / 2.0)
            * GameConfig::PlayerVisualScale;
        const double visibleBottom =
            (playerVisibleBounds.y()
             + playerVisibleBounds.height())
            * GameConfig::PlayerVisualScale;
        const QRectF imageTarget(
            feetAnchor.x() - visibleCenterX,
            feetAnchor.y() - visibleBottom,
            displaySize.width(),
            displaySize.height());
        playerCollisionBox = QRectF(
            imageTarget.left()
                + playerVisibleBounds.x() * GameConfig::PlayerVisualScale,
            imageTarget.top()
                + playerVisibleBounds.y() * GameConfig::PlayerVisualScale,
            playerVisibleBounds.width() * GameConfig::PlayerVisualScale,
            playerVisibleBounds.height() * GameConfig::PlayerVisualScale);
        painter.drawPixmap(imageTarget, playerFrame,
                           QRectF(playerFrame.rect()));
    }

    if (haqiVisible && !haqiFrame.isNull()) {
        const QRectF haqiTarget(
            haqiAnchor.x(),
            haqiAnchor.y() - haqiFrame.height() / 2.0,
            haqiFrame.width(),
            haqiFrame.height());
        painter.drawPixmap(haqiTarget, haqiFrame, QRectF(haqiFrame.rect()));
    }

    if (bigHaqiVisible && !bigHaqiFrame.isNull()) {
        painter.drawPixmap(
            bigHaqiTarget,
            bigHaqiFrame,
            QRectF(bigHaqiFrame.rect()));
    }

    if (atomicBreathVisible && !atomicBreathFrame.isNull()) {
        painter.save();
        painter.translate(atomicBreathAnchor);
        painter.rotate(atomicBreathAngleDegrees);
        painter.translate(-atomicBreathAnchor);
        painter.drawPixmap(
            atomicBreathTarget,
            atomicBreathFrame,
            QRectF(atomicBreathFrame.rect()));
        painter.restore();
    }

    const qsizetype explosionCount =
        qMin(explosionFrames.size(), explosionTargets.size());
    for (qsizetype index = 0; index < explosionCount; ++index) {
        if (explosionFrames[index].isNull())
            continue;

        painter.drawPixmap(
            explosionTargets[index],
            explosionFrames[index],
            QRectF(explosionFrames[index].rect()));
    }

    // 调试用真实碰撞箱：当前猫咪 PNG 非透明像素的外接范围。
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor(195, 79, 53, 150), 2, Qt::DashLine));
    painter.drawRoundedRect(playerCollisionBox, 5, 5);
    painter.restore();

    if (screenRedOverlayAlpha > 0.0)
        painter.fillRect(rect(), QColor(210, 0, 0,
                                        int(screenRedOverlayAlpha)));
}
