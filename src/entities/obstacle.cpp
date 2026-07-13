#include "obstacle.h"
#include "config/gameconfig.h"

#include <QImage>
#include <QtMath>

Obstacle::Obstacle(const QRectF &bounds, ObstacleKind kind, double speed)
    : renderBounds(bounds)
    , obstacleKind(kind)
    , moveSpeed(speed)
{
}

Obstacle::Obstacle(const QPixmap &pixmap, double x, double speed)
    : obstaclePixmap(pixmap)
    , renderBounds(
          x,
          GameConfig::GroundLineY
              - pixmap.height() * GameConfig::ObstacleVisualScale,
          pixmap.width() * GameConfig::ObstacleVisualScale,
          pixmap.height() * GameConfig::ObstacleVisualScale)
    , obstacleKind(ObstacleKind::GroundVehicle)
    , moveSpeed(speed)
{
}

Obstacle::Obstacle(const QPixmap &pixmap, const QRectF &bounds,
                   ObstacleKind kind, double speed)
    : obstaclePixmap(pixmap)
    , renderBounds(bounds)
    , obstacleKind(kind)
    , moveSpeed(speed)
{
}

void Obstacle::moveLeft(double distance)
{
    renderBounds.translate(-distance, 0);
}

void Obstacle::moveByOwnSpeed(double speedMultiplier)
{
    moveLeft(moveSpeed * speedMultiplier);
    if (!hasVerticalMotion)
        return;

    verticalElapsedMs += GameConfig::FrameIntervalMs;
    const double nextTop =
        verticalCenterY
        + qSin(verticalPhase + verticalElapsedMs * verticalAngularSpeed)
            * verticalAmplitude
        - renderBounds.height() / 2.0;
    renderBounds.moveTop(qBound(
        verticalMinTopY,
        nextTop,
        verticalMaxTopY));
}

void Obstacle::configureVerticalMotion(double centerY, double amplitude,
                                       double angularSpeed, double phase,
                                       double minTopY, double maxTopY)
{
    hasVerticalMotion = true;
    verticalCenterY = centerY;
    verticalAmplitude = amplitude;
    verticalAngularSpeed = angularSpeed;
    verticalPhase = phase;
    verticalElapsedMs = 0.0;
    verticalMinTopY = minTopY;
    verticalMaxTopY = maxTopY;
}

QRectF Obstacle::bounds() const
{
    return renderBounds;
}

QRectF Obstacle::collisionBounds() const
{
    if (obstaclePixmap.isNull())
        return renderBounds;

    const QRect visible = visibleBounds();
    const double scaleX = renderBounds.width() / obstaclePixmap.width();
    const double scaleY = renderBounds.height() / obstaclePixmap.height();
    return QRectF(
        renderBounds.left() + visible.left() * scaleX,
        renderBounds.top() + visible.top() * scaleY,
        visible.width() * scaleX,
        visible.height() * scaleY);
}

const QPixmap &Obstacle::pixmap() const
{
    return obstaclePixmap;
}

ObstacleKind Obstacle::kind() const
{
    return obstacleKind;
}

double Obstacle::speed() const
{
    return moveSpeed;
}

bool Obstacle::collidesWith(const QRectF &playerBounds) const
{
    if (obstaclePixmap.isNull())
        return playerBounds.intersects(renderBounds);

    const QRectF overlap = playerBounds.intersected(renderBounds);
    if (overlap.isEmpty())
        return false;

    const QImage image = obstaclePixmap.toImage();
    const double scaleX = image.width() / renderBounds.width();
    const double scaleY = image.height() / renderBounds.height();
    const int left = qMax(
        0,
        int(qFloor((overlap.left() - renderBounds.left()) * scaleX)));
    const int top = qMax(
        0,
        int(qFloor((overlap.top() - renderBounds.top()) * scaleY)));
    const int right = qMin(image.width() - 1,
                           int(qCeil((overlap.right() - renderBounds.left())
                                     * scaleX)));
    const int bottom = qMin(image.height() - 1,
                            int(qCeil((overlap.bottom() - renderBounds.top())
                                      * scaleY)));

    for (int y = top; y <= bottom; ++y) {
        for (int x = left; x <= right; ++x) {
            if (qAlpha(image.pixel(x, y))
                >= GameConfig::ObstacleAlphaThreshold) {
                return true;
            }
        }
    }

    return false;
}

bool Obstacle::isOutsideScreen() const
{
    return renderBounds.right() < 0;
}

QRect Obstacle::visibleBounds() const
{
    const QImage image = obstaclePixmap.toImage();
    int left = image.width();
    int top = image.height();
    int right = -1;
    int bottom = -1;

    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (qAlpha(image.pixel(x, y))
                < GameConfig::ObstacleAlphaThreshold) {
                continue;
            }

            left = qMin(left, x);
            top = qMin(top, y);
            right = qMax(right, x);
            bottom = qMax(bottom, y);
        }
    }

    return right >= left && bottom >= top
        ? QRect(QPoint(left, top), QPoint(right, bottom))
        : obstaclePixmap.rect();
}
