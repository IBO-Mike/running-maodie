#ifndef COLLISIONSYSTEM_H
#define COLLISIONSYSTEM_H

#include <QObject>
#include <QRectF>

class CollisionSystem : public QObject
{
    Q_OBJECT

public:
    explicit CollisionSystem(QObject *parent = nullptr);
    void update();
    bool intersects(const QRectF &first, const QRectF &second) const;
};

#endif // COLLISIONSYSTEM_H
