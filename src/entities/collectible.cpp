#include "collectible.h"

Collectible::Collectible(const QRectF &bounds, int value)
    : collectibleBounds(bounds)
    , scoreValue(value)
{
}

QRectF Collectible::bounds() const
{
    return collectibleBounds;
}

int Collectible::value() const
{
    return scoreValue;
}
