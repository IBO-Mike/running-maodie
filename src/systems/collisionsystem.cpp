#include "collisionsystem.h"

CollisionSystem::CollisionSystem(QObject *parent)
    : QObject(parent)
{
}

void CollisionSystem::update()
{
    // 后续在这里检测玩家、障碍物和收集物之间的碰撞。
}

bool CollisionSystem::intersects(const QRectF &first,
                                 const QRectF &second) const
{
    return first.intersects(second);
}
