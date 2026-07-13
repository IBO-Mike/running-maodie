#include "spawnsystem.h"

SpawnSystem::SpawnSystem(QObject *parent)
    : QObject(parent)
{
}

void SpawnSystem::reset()
{
    elapsedFrames = 0;
}

void SpawnSystem::update()
{
    ++elapsedFrames;
    // 后续根据时间、随机数和难度发出生成信号。
}
