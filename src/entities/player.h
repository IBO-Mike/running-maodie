#ifndef PLAYER_H
#define PLAYER_H

#include <QPointF>

class Player
{
public:
    void reset();
    void jump();
    void update();
    void landOnSurface(double surfaceY);
    void leaveSurface();
    void forceAirbornePosition(double y);

    QPointF position() const;
    bool isOnGround() const;
    double verticalSpeed() const;
    double airborneHeight() const;
    int jumpsUsed() const;

private:
    QPointF currentPosition;
    double verticalVelocity = 0.0;
    bool onGround = true;
    int jumpCount = 0;
    double takeoffY = 0.0;
};

#endif // PLAYER_H
