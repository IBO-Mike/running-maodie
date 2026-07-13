#ifndef COLLECTIBLE_H
#define COLLECTIBLE_H

#include <QRectF>

class Collectible
{
public:
    explicit Collectible(const QRectF &bounds = {}, int value = 1);
    QRectF bounds() const;
    int value() const;

private:
    QRectF collectibleBounds;
    int scoreValue;
};

#endif // COLLECTIBLE_H
