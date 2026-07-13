#ifndef SCORESYSTEM_H
#define SCORESYSTEM_H

#include <QObject>

class ScoreSystem : public QObject
{
    Q_OBJECT

public:
    explicit ScoreSystem(QObject *parent = nullptr);
    void reset();
    void update();
    void addPoints(int points);
    int score() const;

signals:
    void scoreChanged(int score);

private:
    int currentScore = 0;
    int elapsedFrames = 0;
};

#endif // SCORESYSTEM_H
