#include "musicplayerwidget.h"

#include <QAbstractItemView>
#include <QAudioOutput>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMediaPlayer>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QRandomGenerator>
#include <QSlider>
#include <QStyle>
#include <QVBoxLayout>

namespace {
constexpr int MiniPlayerWidth = 360;
constexpr int MiniPlayerHeight = 158;
constexpr int HandleWidth = 46;
constexpr int PlaylistPanelWidth = 390;

QStringList supportedAudioFilters()
{
    return {
        QStringLiteral("*.mp3"),
        QStringLiteral("*.wav"),
        QStringLiteral("*.ogg"),
        QStringLiteral("*.flac"),
        QStringLiteral("*.m4a"),
        QStringLiteral("*.aac")
    };
}
}

MusicPlayerWidget::MusicPlayerWidget(QWidget *parent)
    : QWidget(parent)
    , player(new QMediaPlayer(this))
    , audioOutput(new QAudioOutput(this))
    , songNameLabel(new QLabel(this))
    , artistNameLabel(new QLabel(this))
    , timeLabel(new QLabel(this))
    , volumeLabel(new QLabel(QStringLiteral("音量"), this))
    , progressSlider(new QSlider(Qt::Horizontal, this))
    , volumeSlider(new QSlider(Qt::Horizontal, this))
    , previousButton(new QPushButton(QStringLiteral("上一首"), this))
    , playPauseButton(new QPushButton(QStringLiteral("播放"), this))
    , nextButton(new QPushButton(QStringLiteral("下一首"), this))
    , shuffleButton(new QPushButton(QStringLiteral("顺序"), this))
    , playlistButton(new QPushButton(QStringLiteral("列表"), this))
    , handleButton(new QPushButton(QStringLiteral("♫"), this))
    , playlistPanel(new QWidget(parent))
    , playlistWidget(new QListWidget(playlistPanel))
    , playlistHintLabel(new QLabel(playlistPanel))
    , playlistAnimation(new QPropertyAnimation(playlistPanel, "geometry", this))
    , miniPlayerAnimation(new QPropertyAnimation(this, "geometry", this))
{
    setObjectName(QStringLiteral("musicMiniPlayer"));
    setFixedSize(MiniPlayerWidth, MiniPlayerHeight);
    setFocusPolicy(Qt::NoFocus);

    audioOutput->setVolume(0.45);
    player->setAudioOutput(audioOutput);

    buildMiniPlayer();
    buildPlaylistPanel();
    loadSongs();
    rebuildPlaylist();
    refreshCurrentSongLabels();
    updatePlaybackButtons();

    connect(player, &QMediaPlayer::positionChanged,
            this, [this](qint64 position) {
                if (!userSeeking)
                    progressSlider->setValue(int(position));
                timeLabel->setText(
                    QStringLiteral("%1 / %2")
                        .arg(formattedDuration(position),
                             formattedDuration(player->duration())));
            });
    connect(player, &QMediaPlayer::durationChanged,
            this, [this](qint64 duration) {
                progressSlider->setRange(0, int(qMax<qint64>(0, duration)));
                if (currentSongIndex >= 0
                    && currentSongIndex < songs.size()
                    && duration > 0) {
                    songs[currentSongIndex].durationMs = duration;
                    updatePlaylistItem(currentSongIndex);
                }
                timeLabel->setText(
                    QStringLiteral("%1 / %2")
                        .arg(formattedDuration(player->position()),
                             formattedDuration(duration)));
            });
    connect(player, &QMediaPlayer::playbackStateChanged,
            this, [this] { updatePlaybackButtons(); });
    connect(player, &QMediaPlayer::mediaStatusChanged,
            this, [this](QMediaPlayer::MediaStatus status) {
                if (status == QMediaPlayer::EndOfMedia)
                    playAdjacent(1);
            });

    connect(progressSlider, &QSlider::sliderPressed,
            this, [this] { userSeeking = true; });
    connect(progressSlider, &QSlider::sliderReleased, this, [this] {
        userSeeking = false;
        player->setPosition(progressSlider->value());
    });
    connect(progressSlider, &QSlider::sliderMoved,
            this, [this](int value) {
                timeLabel->setText(
                    QStringLiteral("%1 / %2")
                        .arg(formattedDuration(value),
                             formattedDuration(player->duration())));
            });
    connect(volumeSlider, &QSlider::valueChanged,
            this, [this](int value) {
                audioOutput->setVolume(value / 100.0);
            });

    connect(previousButton, &QPushButton::clicked,
            this, [this] { playAdjacent(-1); });
    connect(nextButton, &QPushButton::clicked,
            this, [this] { playAdjacent(1); });
    connect(playPauseButton, &QPushButton::clicked, this, [this] {
        if (songs.isEmpty())
            return;

        if (currentSongIndex < 0) {
            playRow(0);
            return;
        }

        if (player->playbackState() == QMediaPlayer::PlayingState)
            player->pause();
        else
            player->play();
    });
    connect(shuffleButton, &QPushButton::clicked, this, [this] {
        shuffleEnabled = !shuffleEnabled;
        updatePlaybackButtons();
    });
    connect(playlistButton, &QPushButton::clicked,
            this, [this] { setPlaylistOpen(!playlistOpen); });
    connect(handleButton, &QPushButton::clicked, this, [this] {
        if (collapsedToHandle) {
            setCollapsedToHandle(false, true);
            setPlaylistOpen(true);
        } else {
            setPlaylistOpen(false);
            setCollapsedToHandle(true, true);
        }
    });
    connect(playlistWidget, &QListWidget::itemClicked,
            this, [this](QListWidgetItem *item) {
                playRow(playlistWidget->row(item));
            });
    connect(playlistAnimation, &QPropertyAnimation::finished, this, [this] {
        if (!playlistOpen)
            playlistPanel->hide();
    });
    connect(miniPlayerAnimation, &QPropertyAnimation::finished,
            this, [this] { updateCollapsedUi(); });
}

void MusicPlayerWidget::startPlaybackIfAvailable()
{
    if (!songs.isEmpty() && currentSongIndex < 0)
        playRow(0);
}

void MusicPlayerWidget::setGameMode(bool enabled)
{
    if (gameMode == enabled) {
        if (gameMode)
            setPlaylistOpen(false);
        return;
    }

    gameMode = enabled;
    if (gameMode) {
        setPlaylistOpen(false);
        setCollapsedToHandle(!gameModeWasExpanded, true);
    } else {
        gameModeWasExpanded = !collapsedToHandle;
        setPlaylistOpen(false);
        setCollapsedToHandle(false, false);
    }
}

bool MusicPlayerWidget::isCollapsedToHandle() const
{
    return collapsedToHandle;
}

bool MusicPlayerWidget::isGameMode() const
{
    return gameMode;
}

int MusicPlayerWidget::collapsedHandleWidth() const
{
    return HandleWidth;
}

void MusicPlayerWidget::setPlaylistPanelGeometry(int top, int height)
{
    const int parentWidth = parentWidget() ? parentWidget()->width() : width();
    const QRect target = playlistOpen
        ? QRect(parentWidth - PlaylistPanelWidth, top,
                PlaylistPanelWidth, height)
        : QRect(parentWidth, top, PlaylistPanelWidth, height);
    if (playlistAnimation->state() == QAbstractAnimation::Running)
        playlistAnimation->stop();
    playlistPanel->setGeometry(target);
}

void MusicPlayerWidget::buildMiniPlayer()
{
    setStyleSheet(R"(
        QWidget#musicMiniPlayer {
            background-color: rgba(25, 35, 40, 232);
            border: 2px solid rgba(255, 243, 224, 115);
            border-radius: 16px;
        }
        QWidget#musicMiniPlayer QLabel {
            color: white;
            background: transparent;
        }
        QWidget#musicMiniPlayer QPushButton {
            min-width: 0;
            min-height: 0;
            border: 0;
            border-radius: 10px;
            color: white;
            background-color: rgba(239, 139, 97, 220);
            padding: 5px 7px;
            font-size: 12px;
            font-weight: 700;
        }
        QWidget#musicMiniPlayer QPushButton:hover {
            background-color: #f29d79;
        }
        QWidget#musicMiniPlayer QPushButton:pressed {
            background-color: #da7049;
        }
        QWidget#musicMiniPlayer QPushButton:checked {
            background-color: rgba(84, 139, 137, 225);
        }
        QWidget#musicMiniPlayer QPushButton#playerHandleButton {
            border-top-left-radius: 16px;
            border-bottom-left-radius: 16px;
            border-top-right-radius: 0;
            border-bottom-right-radius: 0;
            background-color: rgba(239, 139, 97, 235);
            font-size: 20px;
        }
        QWidget#musicMiniPlayer QSlider::groove:horizontal {
            height: 5px;
            border-radius: 2px;
            background: rgba(255, 243, 224, 90);
        }
        QWidget#musicMiniPlayer QSlider::sub-page:horizontal {
            border-radius: 2px;
            background: #ef8b61;
        }
        QWidget#musicMiniPlayer QSlider::handle:horizontal {
            width: 12px;
            margin: -4px 0;
            border-radius: 6px;
            background: #fff3e0;
        }
    )");

    songNameLabel->setText(QStringLiteral("暂无歌曲"));
    songNameLabel->setFont(QFont(QStringLiteral("Sans Serif"), 13,
                                 QFont::DemiBold));
    artistNameLabel->setText(QStringLiteral("请将音乐放入 resources/audio/bgm"));
    artistNameLabel->setStyleSheet(QStringLiteral("color: #fff3e0;"));
    timeLabel->setText(QStringLiteral("00:00 / 00:00"));
    timeLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    timeLabel->setStyleSheet(QStringLiteral("color: #fff3e0; font-size: 11px;"));
    volumeLabel->setStyleSheet(QStringLiteral("color: #fff3e0; font-size: 11px;"));

    progressSlider->setRange(0, 0);
    volumeSlider->setRange(0, 100);
    volumeSlider->setValue(45);
    progressSlider->setFocusPolicy(Qt::NoFocus);
    volumeSlider->setFocusPolicy(Qt::NoFocus);
    handleButton->setObjectName(QStringLiteral("playerHandleButton"));
    handleButton->setFocusPolicy(Qt::NoFocus);
    previousButton->setFocusPolicy(Qt::NoFocus);
    playPauseButton->setFocusPolicy(Qt::NoFocus);
    nextButton->setFocusPolicy(Qt::NoFocus);
    shuffleButton->setFocusPolicy(Qt::NoFocus);
    playlistButton->setFocusPolicy(Qt::NoFocus);
    shuffleButton->setCheckable(true);

    auto *titleRow = new QHBoxLayout;
    titleRow->setContentsMargins(0, 0, 0, 0);
    titleRow->addWidget(songNameLabel, 1);
    titleRow->addWidget(timeLabel);

    auto *buttonRow = new QHBoxLayout;
    buttonRow->setContentsMargins(0, 0, 0, 0);
    buttonRow->setSpacing(6);
    buttonRow->addWidget(previousButton);
    buttonRow->addWidget(playPauseButton);
    buttonRow->addWidget(nextButton);
    buttonRow->addWidget(shuffleButton);
    buttonRow->addWidget(playlistButton);

    auto *volumeRow = new QHBoxLayout;
    volumeRow->setContentsMargins(0, 0, 0, 0);
    volumeRow->setSpacing(8);
    volumeRow->addWidget(volumeLabel);
    volumeRow->addWidget(volumeSlider, 1);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 9, 12, 10);
    layout->setSpacing(5);
    layout->addLayout(titleRow);
    layout->addWidget(artistNameLabel);
    layout->addWidget(progressSlider);
    layout->addLayout(volumeRow);
    layout->addLayout(buttonRow);

    handleButton->setGeometry(0, 0, HandleWidth, MiniPlayerHeight);
    handleButton->hide();
}

void MusicPlayerWidget::buildPlaylistPanel()
{
    playlistPanel->setObjectName(QStringLiteral("musicPlaylistPanel"));
    playlistPanel->setFocusPolicy(Qt::NoFocus);
    playlistPanel->setStyleSheet(R"(
        QWidget#musicPlaylistPanel {
            background-color: rgba(25, 35, 40, 232);
            border-left: 2px solid rgba(255, 243, 224, 115);
        }
        QWidget#musicPlaylistPanel QLabel {
            color: white;
            background: transparent;
        }
        QWidget#musicPlaylistPanel QListWidget {
            border: 0;
            outline: 0;
            color: white;
            background: transparent;
            font-size: 13px;
        }
        QWidget#musicPlaylistPanel QListWidget::item {
            padding: 8px 10px;
            border-bottom: 1px solid rgba(255, 243, 224, 32);
        }
        QWidget#musicPlaylistPanel QListWidget::item:selected {
            background-color: rgba(239, 139, 97, 145);
            border-radius: 8px;
        }
    )");

    auto *titleLabel = new QLabel(QStringLiteral("播放列表"), playlistPanel);
    titleLabel->setFont(QFont(QStringLiteral("Sans Serif"), 18,
                              QFont::DemiBold));
    playlistHintLabel->setText(QStringLiteral("拖拽歌曲可调整播放顺序"));
    playlistHintLabel->setStyleSheet(QStringLiteral("color: #fff3e0;"));

    playlistWidget->setDragDropMode(QAbstractItemView::InternalMove);
    playlistWidget->setDefaultDropAction(Qt::MoveAction);
    playlistWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    playlistWidget->setFocusPolicy(Qt::NoFocus);
    playlistWidget->setSpacing(4);

    auto *layout = new QVBoxLayout(playlistPanel);
    layout->setContentsMargins(18, 22, 18, 18);
    layout->setSpacing(10);
    layout->addWidget(titleLabel);
    layout->addWidget(playlistHintLabel);
    layout->addWidget(playlistWidget, 1);

    playlistAnimation->setDuration(220);
    playlistAnimation->setEasingCurve(QEasingCurve::OutCubic);
    playlistPanel->hide();
}

void MusicPlayerWidget::loadSongs()
{
    songs.clear();

    const QDir bgmDir(bgmDirectoryPath());
    const QFileInfoList files = bgmDir.entryInfoList(
        supportedAudioFilters(),
        QDir::Files,
        QDir::Name | QDir::IgnoreCase);
    for (const QFileInfo &file : files)
        songs.append(songInfoFromFile(file.absoluteFilePath()));
}

void MusicPlayerWidget::rebuildPlaylist()
{
    playlistWidget->clear();

    if (songs.isEmpty()) {
        auto *item = new QListWidgetItem(
            QStringLiteral("暂无音乐\n请将音频文件放入 resources/audio/bgm"));
        item->setFlags(Qt::NoItemFlags);
        playlistWidget->addItem(item);
        return;
    }

    for (int index = 0; index < songs.size(); ++index) {
        auto *item = new QListWidgetItem;
        item->setData(Qt::UserRole, index);
        item->setSizeHint(QSize(0, 58));
        playlistWidget->addItem(item);
        updatePlaylistItem(index);
    }
}

void MusicPlayerWidget::updatePlaylistItem(int songIndex)
{
    const int row = rowForSong(songIndex);
    if (row < 0)
        return;

    const SongInfo &song = songs[songIndex];
    QListWidgetItem *item = playlistWidget->item(row);
    item->setText(
        QStringLiteral("%1\n%2 · %3")
            .arg(song.title,
                 song.artist,
                 formattedDuration(song.durationMs)));
}

void MusicPlayerWidget::refreshCurrentSongLabels()
{
    if (currentSongIndex < 0 || currentSongIndex >= songs.size()) {
        songNameLabel->setText(QStringLiteral("暂无歌曲"));
        artistNameLabel->setText(QStringLiteral("请将音乐放入 resources/audio/bgm"));
        timeLabel->setText(QStringLiteral("00:00 / 00:00"));
        progressSlider->setRange(0, 0);
        return;
    }

    const SongInfo &song = songs[currentSongIndex];
    songNameLabel->setText(song.title);
    artistNameLabel->setText(song.artist);
}

void MusicPlayerWidget::playRow(int row)
{
    if (row < 0 || row >= playlistWidget->count())
        return;

    QListWidgetItem *item = playlistWidget->item(row);
    const int songIndex = item->data(Qt::UserRole).toInt();
    playSong(songIndex);
}

void MusicPlayerWidget::playSong(int songIndex)
{
    if (songIndex < 0 || songIndex >= songs.size())
        return;

    currentSongIndex = songIndex;
    player->setSource(songs[songIndex].url);
    player->play();
    refreshCurrentSongLabels();

    const int row = rowForSong(songIndex);
    if (row >= 0)
        playlistWidget->setCurrentRow(row);
}

void MusicPlayerWidget::playAdjacent(int offset)
{
    if (songs.isEmpty())
        return;

    if (shuffleEnabled && songs.size() > 1) {
        int row = currentVisualRow();
        int nextRow = row;
        while (nextRow == row)
            nextRow = QRandomGenerator::global()->bounded(playlistWidget->count());
        playRow(nextRow);
        return;
    }

    const int row = currentVisualRow();
    const int baseRow = row >= 0 ? row : 0;
    const int nextRow =
        (baseRow + offset + playlistWidget->count()) % playlistWidget->count();
    playRow(nextRow);
}

void MusicPlayerWidget::setPlaylistOpen(bool open)
{
    if (playlistOpen == open)
        return;

    playlistOpen = open;
    playlistButton->setChecked(open);
    if (gameMode && open)
        gameModeWasExpanded = true;

    const int parentWidth = parentWidget() ? parentWidget()->width() : width();
    const int panelTop = y() + height();
    const int parentHeight = parentWidget() ? parentWidget()->height() : height();
    const int panelHeight = qMax(120, parentHeight - panelTop);
    const QRect start = playlistPanel->geometry();
    const QRect end = open
        ? QRect(parentWidth - PlaylistPanelWidth, panelTop,
                PlaylistPanelWidth, panelHeight)
        : QRect(parentWidth, panelTop, PlaylistPanelWidth, panelHeight);

    if (open) {
        playlistPanel->setGeometry(
            QRect(parentWidth, panelTop, PlaylistPanelWidth, panelHeight));
        playlistPanel->show();
        playlistPanel->raise();
        raise();
    }

    playlistAnimation->stop();
    playlistAnimation->setStartValue(open ? playlistPanel->geometry() : start);
    playlistAnimation->setEndValue(end);
    playlistAnimation->start();
    emit playlistVisibilityChanged(open);
}

void MusicPlayerWidget::setCollapsedToHandle(bool collapsed, bool animate)
{
    if (gameMode && collapsed)
        gameModeWasExpanded = false;
    collapsedToHandle = collapsed;
    updateCollapsedUi();

    const QRect target = collapsed ? collapsedGeometry() : expandedGeometry();
    if (animate) {
        miniPlayerAnimation->stop();
        miniPlayerAnimation->setDuration(220);
        miniPlayerAnimation->setEasingCurve(QEasingCurve::OutCubic);
        miniPlayerAnimation->setStartValue(geometry());
        miniPlayerAnimation->setEndValue(target);
        miniPlayerAnimation->start();
    } else {
        setGeometry(target);
    }
}

void MusicPlayerWidget::updateCollapsedUi()
{
    const bool showHandle = gameMode;
    handleButton->setVisible(showHandle);
    handleButton->raise();

    handleButton->setText(collapsedToHandle
                              ? QStringLiteral("♫")
                              : QStringLiteral("›"));
    const QList<QWidget *> contentWidgets = {
        songNameLabel,
        artistNameLabel,
        timeLabel,
        volumeLabel,
        progressSlider,
        volumeSlider,
        previousButton,
        playPauseButton,
        nextButton,
        shuffleButton,
        playlistButton
    };
    for (QWidget *widget : contentWidgets)
        widget->setVisible(!collapsedToHandle);

    if (layout()) {
        const int leftMargin = gameMode && !collapsedToHandle
            ? HandleWidth + 12
            : 12;
        layout()->setContentsMargins(leftMargin, 9, 12, 10);
    }
}

QRect MusicPlayerWidget::expandedGeometry() const
{
    const int parentWidth = parentWidget() ? parentWidget()->width() : width();
    const int rightMargin = gameMode ? 0 : 18;
    return QRect(
        qMax(18, parentWidth - MiniPlayerWidth - rightMargin),
        y(),
        MiniPlayerWidth,
        MiniPlayerHeight);
}

QRect MusicPlayerWidget::collapsedGeometry() const
{
    const int parentWidth = parentWidget() ? parentWidget()->width() : width();
    return QRect(
        parentWidth - HandleWidth,
        y(),
        MiniPlayerWidth,
        MiniPlayerHeight);
}

void MusicPlayerWidget::updatePlaybackButtons()
{
    const bool hasSongs = !songs.isEmpty();
    previousButton->setEnabled(hasSongs);
    playPauseButton->setEnabled(hasSongs);
    nextButton->setEnabled(hasSongs);
    shuffleButton->setEnabled(hasSongs);

    const bool playing =
        player->playbackState() == QMediaPlayer::PlayingState;
    playPauseButton->setText(playing
                                 ? QStringLiteral("暂停")
                                 : QStringLiteral("播放"));
    shuffleButton->setChecked(shuffleEnabled);
    shuffleButton->setText(shuffleEnabled
                               ? QStringLiteral("随机")
                               : QStringLiteral("顺序"));
}

int MusicPlayerWidget::currentVisualRow() const
{
    return rowForSong(currentSongIndex);
}

int MusicPlayerWidget::rowForSong(int songIndex) const
{
    if (songIndex < 0)
        return -1;

    for (int row = 0; row < playlistWidget->count(); ++row) {
        QListWidgetItem *item = playlistWidget->item(row);
        if (item->data(Qt::UserRole).toInt() == songIndex)
            return row;
    }
    return -1;
}

QString MusicPlayerWidget::bgmDirectoryPath() const
{
#ifdef RUNNING_MAODIE_SOURCE_DIR
    return QDir(QString::fromUtf8(RUNNING_MAODIE_SOURCE_DIR))
        .filePath(QStringLiteral("resources/audio/bgm"));
#else
    return QDir(QCoreApplication::applicationDirPath())
        .filePath(QStringLiteral("resources/audio/bgm"));
#endif
}

QString MusicPlayerWidget::formattedDuration(qint64 durationMs) const
{
    if (durationMs <= 0)
        return QStringLiteral("--:--");

    const qint64 totalSeconds = durationMs / 1000;
    const qint64 minutes = totalSeconds / 60;
    const qint64 seconds = totalSeconds % 60;
    return QStringLiteral("%1:%2")
        .arg(minutes, 2, 10, QLatin1Char('0'))
        .arg(seconds, 2, 10, QLatin1Char('0'));
}

MusicPlayerWidget::SongInfo MusicPlayerWidget::songInfoFromFile(
    const QString &filePath) const
{
    const QFileInfo fileInfo(filePath);
    QString baseName = fileInfo.completeBaseName();
    baseName.replace(QLatin1Char('_'), QLatin1Char(' '));

    SongInfo song;
    song.filePath = fileInfo.absoluteFilePath();
    song.url = QUrl::fromLocalFile(song.filePath);
    song.artist = QStringLiteral("未知歌手");
    song.title = baseName.trimmed();

    const QString separator = QStringLiteral(" - ");
    const int separatorIndex = baseName.indexOf(separator);
    if (separatorIndex > 0) {
        song.artist = baseName.left(separatorIndex).trimmed();
        song.title = baseName.mid(separatorIndex + separator.size()).trimmed();
    }

    return song;
}
