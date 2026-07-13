#include "player.h"
#include "config/gameconfig.h"

void Player::reset()
{
    currentPosition = QPointF(GameConfig::PlayerX, GameConfig::GroundY);
    verticalVelocity = 0.0;
    onGround = true;
    jumpCount = 0;
    takeoffY = currentPosition.y();
}

void Player::jump()
{
    if (onGround)
        takeoffY = currentPosition.y();

    verticalVelocity = GameConfig::JumpVelocity;
    onGround = false;
    ++jumpCount;
}

void Player::update()
{
    if (onGround)
        return;

    verticalVelocity += GameConfig::Gravity;
    currentPosition.ry() += verticalVelocity;

    if (currentPosition.y() < 0.0) {
        currentPosition.setY(0.0);
        if (verticalVelocity < 0.0)
            verticalVelocity = 0.0;
    }

    if (currentPosition.y() >= GameConfig::GroundY) {
        currentPosition.setY(GameConfig::GroundY);
        verticalVelocity = 0.0;
        onGround = true;
        jumpCount = 0;
        takeoffY = currentPosition.y();
    }
}

void Player::landOnSurface(double surfaceY)
{
    currentPosition.setY(surfaceY - GameConfig::PlayerHeight);
    verticalVelocity = 0.0;
    onGround = true;
    jumpCount = 0;
    takeoffY = currentPosition.y();
}

void Player::leaveSurface()
{
    if (currentPosition.y() >= GameConfig::GroundY)
        return;

    onGround = false;
    verticalVelocity = 0.0;
}

void Player::forceAirbornePosition(double y)
{
    currentPosition.setY(qBound(0.0, y, double(GameConfig::GroundY)));
    verticalVelocity = 0.0;
    onGround = false;
    takeoffY = qMax(takeoffY, currentPosition.y());
}

QPointF Player::position() const
{
    return currentPosition;
}

bool Player::isOnGround() const
{
    return onGround;
}

double Player::verticalSpeed() const
{
    return verticalVelocity;
}

double Player::airborneHeight() const
{
    return qMax(0.0, takeoffY - currentPosition.y());
}

int Player::jumpsUsed() const
{
    return jumpCount;
}
