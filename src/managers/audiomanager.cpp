#include "audiomanager.h"

AudioManager::AudioManager(QObject *parent)
    : QObject(parent)
{
}

void AudioManager::playMusic(const QString &)
{
    // 加入 Qt Multimedia 后在这里播放循环背景音乐。
}

void AudioManager::playEffect(const QString &)
{
    // 加入 Qt Multimedia 后在这里播放短音效。
}

void AudioManager::stopMusic()
{
}

void AudioManager::setMuted(bool value)
{
    muted = value;
}

bool AudioManager::isMuted() const
{
    return muted;
}
