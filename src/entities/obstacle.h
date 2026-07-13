#ifndef OBSTACLE_H
#define OBSTACLE_H

#include <QPixmap>
#include <QRectF>

enum class ObstacleKind {
    GroundVehicle,
    Airborne
};

class Obstacle
{
public:
    explicit Obstacle(
        const QRectF &bounds = {},
        ObstacleKind kind = ObstacleKind::GroundVehicle,
        double speed = 0.0);
    Obstacle(const QPixmap &pixmap, double x, double speed = 0.0);
    Obstacle(const QPixmap &pixmap, const QRectF &bounds, ObstacleKind kind,
             double speed = 0.0);

    void moveLeft(double distance);
    void moveByOwnSpeed(double speedMultiplier = 1.0);
    void configureVerticalMotion(double centerY, double amplitude,
                                 double angularSpeed, double phase,
                                 double minTopY, double maxTopY);
    QRectF bounds() const;
    QRectF collisionBounds() const;
    const QPixmap &pixmap() const;
    ObstacleKind kind() const;
    double speed() const;
    bool collidesWith(const QRectF &playerBounds) const;
    bool isOutsideScreen() const;

private:
    QRect visibleBounds() const;

    QPixmap obstaclePixmap;
    QRectF renderBounds;
    ObstacleKind obstacleKind = ObstacleKind::GroundVehicle;
    double moveSpeed = 0.0;
    bool hasVerticalMotion = false;
    double verticalCenterY = 0.0;
    double verticalAmplitude = 0.0;
    double verticalAngularSpeed = 0.0;
    double verticalPhase = 0.0;
    double verticalElapsedMs = 0.0;
    double verticalMinTopY = 0.0;
    double verticalMaxTopY = 0.0;
};

#endif // OBSTACLE_H
