#ifndef MUSICPLAYERWIDGET_H
#define MUSICPLAYERWIDGET_H

#include <QUrl>
#include <QVector>
#include <QWidget>

class QLabel;
class QListWidget;
class QListWidgetItem;
class QMediaPlayer;
class QAudioOutput;
class QPropertyAnimation;
class QPushButton;
class QSlider;

class MusicPlayerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MusicPlayerWidget(QWidget *parent = nullptr);

    void setPlaylistPanelGeometry(int top, int height);
    void startPlaybackIfAvailable();
    void setGameMode(bool enabled);
    bool isGameMode() const;
    bool isCollapsedToHandle() const;
    int collapsedHandleWidth() const;

signals:
    void playlistVisibilityChanged(bool visible);

private:
    struct SongInfo {
        QString title;
        QString artist;
        QString filePath;
        qint64 durationMs = 0;
        QUrl url;
    };

    void buildMiniPlayer();
    void buildPlaylistPanel();
    void loadSongs();
    void rebuildPlaylist();
    void updatePlaylistItem(int songIndex);
    void refreshCurrentSongLabels();
    void playRow(int row);
    void playSong(int songIndex);
    void playAdjacent(int offset);
    void setPlaylistOpen(bool open);
    void setCollapsedToHandle(bool collapsed, bool animate);
    void updateCollapsedUi();
    QRect expandedGeometry() const;
    QRect collapsedGeometry() const;
    void updatePlaybackButtons();
    int currentVisualRow() const;
    int rowForSong(int songIndex) const;
    QString bgmDirectoryPath() const;
    QString formattedDuration(qint64 durationMs) const;
    SongInfo songInfoFromFile(const QString &filePath) const;

    QMediaPlayer *player;
    QAudioOutput *audioOutput;
    QLabel *songNameLabel;
    QLabel *artistNameLabel;
    QLabel *timeLabel;
    QLabel *volumeLabel;
    QSlider *progressSlider;
    QSlider *volumeSlider;
    QPushButton *previousButton;
    QPushButton *playPauseButton;
    QPushButton *nextButton;
    QPushButton *shuffleButton;
    QPushButton *playlistButton;
    QPushButton *handleButton;
    QWidget *playlistPanel;
    QListWidget *playlistWidget;
    QLabel *playlistHintLabel;
    QPropertyAnimation *playlistAnimation;
    QPropertyAnimation *miniPlayerAnimation;
    QVector<SongInfo> songs;
    int currentSongIndex = -1;
    bool shuffleEnabled = false;
    bool playlistOpen = false;
    bool gameModeWasExpanded = false;
    bool userSeeking = false;
    bool gameMode = false;
    bool collapsedToHandle = false;
};

#endif // MUSICPLAYERWIDGET_H
