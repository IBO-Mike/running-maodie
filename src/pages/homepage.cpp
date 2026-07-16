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
constexpr int TitleTopY = 42;
constexpr int TitleMaxWidth = 400;
constexpr int TitleMaxHeight = 154;
constexpr int TitleHorizontalMargin = 80;
constexpr int TitlePlayerGap = 16;
constexpr int HomeMusicPlayerWidth = 310;
constexpr int HomeMusicPlayerHeight = 132;
constexpr int HomeMusicPlayerTop = 18;
constexpr int HomeMusicPlayerRight = 18;
constexpr int HighScoreLabelWidth = 260;
constexpr int HighScoreLabelHeight = 38;
constexpr int HomeButtonWidth = 220;
constexpr int HomeButtonHeight = 56;
constexpr int HomeStartButtonY = 476;
constexpr int CatSeatOffsetFromStartButtonTop = 0;
constexpr double HomeEntrySpeed = 14.0;
}

HomePage::HomePage(QWidget *parent)
    : QWidget(parent)
    , background(QStringLiteral(":/images/backgrounds/background.png"))
    , titlePixmap(QStringLiteral(":/images/ui/title_cutout.png"))
    , sitFrames(loadFrames(QStringLiteral("sit"), 3))
    , sleepFrames(loadFrames(QStringLiteral("sleep"), 4))
    , fallFrames(loadFrames(QStringLiteral("fall"), 4))
    , runFrames(loadFrames(QStringLiteral("run"), 7))
    , titleLabel(new QLabel(this))
    , highScoreLabel(new QLabel(this))
    , startButton(new QPushButton(QStringLiteral("开始游戏"), this))
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
        QLabel#titleLabel { background: transparent; }
        QLabel#highScoreLabel {
            color: #fff3e0;
            background-color: rgba(25, 35, 40, 125);
            border-radius: 12px;
            font-size: 20px;
            font-weight: 800;
        }
    )");

    titleLabel->setObjectName(QStringLiteral("titleLabel"));
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    highScoreLabel->setObjectName(QStringLiteral("highScoreLabel"));
    highScoreLabel->setAlignment(Qt::AlignCenter);

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
    titleLabel->show();
    highScoreLabel->show();
    updateHighScoreLabel();
    startButton->show();
    startButton->setEnabled(false);
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
    updateTitlePixmap();

    const int highScoreY = titleLabel->geometry().bottom() + 14;
    highScoreLabel->setGeometry(
        (width() - HighScoreLabelWidth) / 2,
        highScoreY,
        HighScoreLabelWidth,
        HighScoreLabelHeight);

    const int buttonX =
        (width() - HomeButtonWidth) / 2 + int(homeGroupOffsetX);
    const int startY = qMin(
        qMax(HomeStartButtonY, highScoreY + HighScoreLabelHeight + 96),
        qMax(0, height() - HomeButtonHeight - 24));
    startButton->setGeometry(
        buttonX, startY, HomeButtonWidth, HomeButtonHeight);

    if (state == HomeState::Entering
        || state == HomeState::Sit
        || state == HomeState::Sleep) {
        catAnchor = QPointF(
            buttonX + HomeButtonWidth / 2.0,
            startY + CatSeatOffsetFromStartButtonTop);
    }
}

void HomePage::updateTitlePixmap()
{
    const int availableTitleAreaWidth =
        qMax(260, width() - TitleHorizontalMargin * 2);
    if (titlePixmap.isNull()) {
        const int fallbackWidth = qMin(TitleMaxWidth, availableTitleAreaWidth);
        QRect titleRect((width() - fallbackWidth) / 2,
                        TitleTopY,
                        fallbackWidth,
                        TitleMaxHeight);
        const QRect playerSafeRect(
            width() - HomeMusicPlayerWidth - HomeMusicPlayerRight
                - TitlePlayerGap,
            HomeMusicPlayerTop,
            HomeMusicPlayerWidth + TitlePlayerGap,
            HomeMusicPlayerHeight + TitlePlayerGap);
        if (titleRect.intersects(playerSafeRect))
            titleRect.moveTop(playerSafeRect.bottom() + TitlePlayerGap);
        titleLabel->setGeometry(titleRect);
        titleLabel->setText(QStringLiteral("Running Maodie"));
        return;
    }

    int titleWidth = qMin(TitleMaxWidth, availableTitleAreaWidth - 48);
    titleWidth = qMax(260, titleWidth);
    int titleHeight = int(titleWidth * titlePixmap.height()
                          / double(titlePixmap.width()));
    if (titleHeight > TitleMaxHeight) {
        titleHeight = TitleMaxHeight;
        titleWidth = int(titleHeight * titlePixmap.width()
                         / double(titlePixmap.height()));
    }

    QRect titleRect((width() - titleWidth) / 2,
                    TitleTopY,
                    titleWidth,
                    titleHeight);
    const QRect playerSafeRect(
        width() - HomeMusicPlayerWidth - HomeMusicPlayerRight - TitlePlayerGap,
        HomeMusicPlayerTop,
        HomeMusicPlayerWidth + TitlePlayerGap,
        HomeMusicPlayerHeight + TitlePlayerGap);
    if (titleRect.intersects(playerSafeRect))
        titleRect.moveTop(playerSafeRect.bottom() + TitlePlayerGap);

    titleLabel->setGeometry(titleRect);
    titleLabel->setPixmap(titlePixmap.scaled(titleLabel->size(),
                                             Qt::KeepAspectRatio,
                                             Qt::SmoothTransformation));
}

void HomePage::updateHighScoreLabel()
{
    const int highScore =
        QSettings().value(QStringLiteral("scores/highScore"), 0).toInt();
    highScoreLabel->setText(QStringLiteral("最高得分：%1").arg(highScore));
}
