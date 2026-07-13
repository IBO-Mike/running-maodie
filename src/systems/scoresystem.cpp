#include "scoresystem.h"

ScoreSystem::ScoreSystem(QObject *parent)
    : QObject(parent)
{
}

void ScoreSystem::reset()
{
    currentScore = 0;
    elapsedFrames = 0;
    emit scoreChanged(currentScore);
}

void ScoreSystem::update()
{
    // 分数只在收集金币等明确事件发生时增加。
}

void ScoreSystem::addPoints(int points)
{
    currentScore += points;
    emit scoreChanged(currentScore);
}

int ScoreSystem::score() const
{
    return currentScore;
}
