#ifndef AUDIOMANAGER_H
#define AUDIOMANAGER_H

#include <QObject>
#include <QString>

class AudioManager : public QObject
{
    Q_OBJECT

public:
    explicit AudioManager(QObject *parent = nullptr);
    void playMusic(const QString &resourcePath);
    void playEffect(const QString &resourcePath);
    void stopMusic();
    void setMuted(bool muted);
    bool isMuted() const;

private:
    bool muted = false;
};

#endif // AUDIOMANAGER_H
