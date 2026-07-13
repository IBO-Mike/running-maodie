#include "homepage.h"
#include "config/gameconfig.h"

#include <QImage>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QResizeEvent>
#include <QSettings>
#include <QTimer>

namespace {
constexpr int CatSleepDelayMs = 6000;
constexpr int LogoSpaceWidth = 430;
constexpr int LogoSpaceHeight = 120;
constexpr int HighScoreLabelWidth = 260;
constexpr int HighScoreLabelHeight = 38;
constexpr int HomeButtonWidth = 220;
constexpr int HomeButtonHeight = 56;
constexpr int HomeStartButtonY = 390;
constexpr int CatSeatOffsetFromStartButtonTop = 0;
constexpr double HomeEntrySpeed = 14.0;
}

HomePage::HomePage(QWidget *parent)
    : QWidget(parent)
    , background(QStringLiteral(":/images/backgrounds/background.png"))
    , sitFrames(loadFrames(QStringLiteral("sit"), 3))
    , sleepFrames(loadFrames(QStringLiteral("sleep"), 4))
    , fallFrames(loadFrames(QStringLiteral("fall"), 4))
    , runFrames(loadFrames(QStringLiteral("run"), 7))
    , logoSpace(new QWidget(this))
    , highScoreLabel(new QLabel(this))
    , startButton(new QPushButton(QStringLiteral("开始游戏"), this))
    , settingsButton(new QPushButton(QStringLiteral("设置"), this))
    , timer(new QTimer(this))
{
    setObjectName(QStringLiteral("homePage"));
    setMouseTracking(true);
    setStyleSheet(R"(
        QWidget#homePage { background-color: #9ddce3; }
        QPushButton {
            border: 0;
            border-radius: 16px;
            color: white;
            background-color: rgba(239, 139, 97, 235);
            font-size: 20px;
            font-weight: 700;
        }
        QPushButton:hover { background-color: #f29d79; }
        QPushButton:pressed { background-color: #da7049; }
        QPushButton#settingsButton {
            color: #455a64;
            background-color: rgba(255, 255, 255, 215);
            border: 2px solid rgba(84, 139, 137, 190);
        }
        QLabel#highScoreLabel {
            color: #fff3e0;
            background-color: rgba(25, 35, 40, 125);
            border-radius: 12px;
            font-size: 20px;
            font-weight: 800;
        }
    )");

    // 此区域刻意保持透明，后续直接放置 Logo 图片。
    logoSpace->setObjectName(QStringLiteral("logoSpace"));
    logoSpace->setAttribute(Qt::WA_TransparentForMouseEvents);
    highScoreLabel->setObjectName(QStringLiteral("highScoreLabel"));
    highScoreLabel->setAlignment(Qt::AlignCenter);
    settingsButton->setObjectName(QStringLiteral("settingsButton"));
    settingsButton->setToolTip(QStringLiteral("设置功能将在后续版本中加入"));

    connect(startButton, &QPushButton::clicked,
            this, &HomePage::beginGameTransition);
    connect(timer, &QTimer::timeout,
            this, &HomePage::updateFrame);
    timer->start(GameConfig::FrameIntervalMs);

    resetHome();
}

void HomePage::resetHome()
{
    state = HomeState::Entering;
    frameIndex = 0;
    frameElapsedMs = 0;
    inactiveElapsedMs = 0;
    fallVelocity = 0.0;
    homeGroupOffsetX = 0.0;
    logoSpace->show();
    highScoreLabel->show();
    updateHighScoreLabel();
    startButton->show();
    settingsButton->show();
    startButton->setEnabled(false);
    settingsButton->setEnabled(false);
    positionControls();
    const int centeredButtonX = (width() - HomeButtonWidth) / 2;
    homeGroupOffsetX = -centeredButtonX - HomeButtonWidth - 40;
    positionControls();
    update();
}

void HomePage::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    drawBackground(painter);
    drawCat(painter);
}

void HomePage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    positionControls();
}

void HomePage::mousePressEvent(QMouseEvent *event)
{
    if ((state == HomeState::Sit || state == HomeState::Sleep)
        && catClickArea.contains(event->position())) {
        state = HomeState::Sit;
        frameIndex = 0;
        frameElapsedMs = 0;
        inactiveElapsedMs = 0;
        update();
        event->accept();
        return;
    }

    QWidget::mousePressEvent(event);
}

void HomePage::updateFrame()
{
    if (!background.isNull() && height() > 0) {
        const double displayedWidth =
            background.width() * (height() / double(background.height()));
        backgroundOffset -= GameConfig::BackgroundSpeed;
        if (backgroundOffset <= -displayedWidth)
            backgroundOffset += displayedWidth;
    }

    switch (state) {
    case HomeState::Entering:
        homeGroupOffsetX += HomeEntrySpeed;
        if (homeGroupOffsetX >= 0.0) {
            homeGroupOffsetX = 0.0;
            state = HomeState::Sit;
            frameIndex = 0;
            frameElapsedMs = 0;
            inactiveElapsedMs = 0;
            startButton->setEnabled(true);
            settingsButton->setEnabled(true);
        }
        positionControls();
        advanceAnimation(180, false);
        break;

    case HomeState::Sit:
        inactiveElapsedMs += GameConfig::FrameIntervalMs;
        advanceAnimation(180, false);
        if (inactiveElapsedMs >= CatSleepDelayMs) {
            state = HomeState::Sleep;
            frameIndex = 0;
            frameElapsedMs = 0;
        }
        break;

    case HomeState::Sleep:
        advanceAnimation(220, true);
        break;

    case HomeState::Falling:
        advanceAnimation(120, true);
        fallVelocity += GameConfig::Gravity;
        catAnchor.ry() += fallVelocity;
        if (catAnchor.y() >= GameConfig::GroundLineY) {
            catAnchor.setY(GameConfig::GroundLineY);
            state = HomeState::MovingToGame;
            frameIndex = 0;
            frameElapsedMs = 0;
        }
        break;

    case HomeState::MovingToGame: {
        advanceAnimation(75, true);
        const double gameAnchorX =
            GameConfig::PlayerX + GameConfig::PlayerWidth / 2.0;
        catAnchor.rx() -= GameConfig::BackgroundSpeed;
        if (catAnchor.x() <= gameAnchorX) {
            catAnchor.setX(gameAnchorX);
            state = HomeState::Finished;
            emit startGameRequested(backgroundOffset);
        }
        break;
    }

    case HomeState::Finished:
        break;
    }

    update();
}

void HomePage::beginGameTransition()
{
    if (state == HomeState::Entering
        || state == HomeState::Falling
        || state == HomeState::MovingToGame
        || state == HomeState::Finished) {
        return;
    }

    startButton->hide();
    settingsButton->hide();
    state = HomeState::Falling;
    frameIndex = 0;
    frameElapsedMs = 0;
    fallVelocity = 0.0;
}

QVector<QPixmap> HomePage::loadFrames(const QString &action, int count) const
{
    QVector<QPixmap> frames;
    frames.reserve(count);
    for (int index = 1; index <= count; ++index) {
        frames.append(QPixmap(
            QStringLiteral(":/images/player/%1_%2.png")
                .arg(action)
                .arg(index, 2, 10, QLatin1Char('0'))));
    }
    return frames;
}

const QPixmap &HomePage::currentCatFrame() const
{
    switch (state) {
    case HomeState::Entering:
    case HomeState::Sit:
        return sitFrames[qMin(frameIndex, sitFrames.size() - 1)];
    case HomeState::Sleep:
        return sleepFrames[qMin(frameIndex, sleepFrames.size() - 1)];
    case HomeState::Falling:
        return fallFrames[qMin(frameIndex, fallFrames.size() - 1)];
    case HomeState::MovingToGame:
    case HomeState::Finished:
        return runFrames[qMin(frameIndex, runFrames.size() - 1)];
    }
    return sitFrames.first();
}

QRect HomePage::visibleBounds(const QPixmap &frame)
{
    const qint64 key = frame.cacheKey();
    const auto cached = alphaBoundsCache.constFind(key);
    if (cached != alphaBoundsCache.cend())
        return cached.value();

    const QImage image = frame.toImage();
    int left = image.width();
    int top = image.height();
    int right = -1;
    int bottom = -1;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (qAlpha(image.pixel(x, y)) == 0)
                continue;
            left = qMin(left, x);
            top = qMin(top, y);
            right = qMax(right, x);
            bottom = qMax(bottom, y);
        }
    }

    const QRect bounds = right >= left && bottom >= top
        ? QRect(QPoint(left, top), QPoint(right, bottom))
        : frame.rect();
    alphaBoundsCache.insert(key, bounds);
    return bounds;
}

void HomePage::advanceAnimation(int frameDurationMs, bool loops)
{
    frameElapsedMs += GameConfig::FrameIntervalMs;
    if (frameElapsedMs < frameDurationMs)
        return;
    frameElapsedMs -= frameDurationMs;

    int frameCount = 1;
    switch (state) {
    case HomeState::Entering: frameCount = sitFrames.size(); break;
    case HomeState::Sit: frameCount = sitFrames.size(); break;
    case HomeState::Sleep: frameCount = sleepFrames.size(); break;
    case HomeState::Falling: frameCount = fallFrames.size(); break;
    case HomeState::MovingToGame:
    case HomeState::Finished: frameCount = runFrames.size(); break;
    }

    ++frameIndex;
    if (frameIndex >= frameCount)
        frameIndex = loops ? 0 : frameCount - 1;
}

void HomePage::drawBackground(QPainter &painter)
{
    painter.fillRect(rect(), QColor("#9ddce3"));
    if (background.isNull() || height() <= 0)
        return;

    const double displayedWidth =
        background.width() * (height() / double(background.height()));
    for (double x = backgroundOffset; x < width(); x += displayedWidth) {
        painter.drawPixmap(
            QRectF(x, 0, displayedWidth + 1.0, height()),
            background,
            QRectF(background.rect()));
    }
}

void HomePage::drawCat(QPainter &painter)
{
    const QPixmap &frame = currentCatFrame();
    if (frame.isNull())
        return;

    const QRect alphaBounds = visibleBounds(frame);
    const QSizeF displaySize(
        frame.width() * GameConfig::PlayerVisualScale,
        frame.height() * GameConfig::PlayerVisualScale);
    const double visibleCenterX =
        (alphaBounds.x() + alphaBounds.width() / 2.0)
        * GameConfig::PlayerVisualScale;
    const double visibleBottom =
        (alphaBounds.y() + alphaBounds.height())
        * GameConfig::PlayerVisualScale;
    const QRectF target(
        catAnchor.x() - visibleCenterX,
        catAnchor.y() - visibleBottom,
        displaySize.width(),
        displaySize.height());

    painter.drawPixmap(target, frame, QRectF(frame.rect()));
    catClickArea = target;
}

void HomePage::positionControls()
{
    logoSpace->setGeometry(
        (width() - LogoSpaceWidth) / 2,
        24,
        LogoSpaceWidth,
        LogoSpaceHeight);
    highScoreLabel->setGeometry(
        (width() - HighScoreLabelWidth) / 2,
        154,
        HighScoreLabelWidth,
        HighScoreLabelHeight);

    const int buttonX =
        (width() - HomeButtonWidth) / 2 + int(homeGroupOffsetX);
    const int startY = HomeStartButtonY;
    startButton->setGeometry(
        buttonX, startY, HomeButtonWidth, HomeButtonHeight);
    settingsButton->setGeometry(
        buttonX, startY + 72, HomeButtonWidth, HomeButtonHeight);

    if (state == HomeState::Entering
        || state == HomeState::Sit
        || state == HomeState::Sleep) {
        catAnchor = QPointF(
            buttonX + HomeButtonWidth / 2.0,
            startY + CatSeatOffsetFromStartButtonTop);
    }
}

void HomePage::updateHighScoreLabel()
{
    const int highScore =
        QSettings().value(QStringLiteral("scores/highScore"), 0).toInt();
    highScoreLabel->setText(QStringLiteral("最高得分：%1").arg(highScore));
}
