#ifndef SPAWNSYSTEM_H
#define SPAWNSYSTEM_H

#include <QObject>

class SpawnSystem : public QObject
{
    Q_OBJECT

public:
    explicit SpawnSystem(QObject *parent = nullptr);
    void reset();
    void update();

signals:
    void obstacleRequested();
    void collectibleRequested();

private:
    int elapsedFrames = 0;
};

#endif // SPAWNSYSTEM_H
