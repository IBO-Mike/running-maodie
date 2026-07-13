#include "playeranimator.h"
#include "config/gameconfig.h"

#include <QString>

PlayerAnimator::PlayerAnimator()
{
    addClip(PlayerAction::Idle,   "idle",   4, 140, true);
    addClip(PlayerAction::Walk,   "walk",   7, 100, true);
    addClip(PlayerAction::Run,    "run",    7, 75,  true);
    addClip(PlayerAction::Jump,   "jump",   7, 90,  false);
    addClip(PlayerAction::Sit,    "sit",    3, 180, false);
    addClip(PlayerAction::Sleep,  "sleep",  4, 220, true);
    addClip(PlayerAction::Attack, "attack", 3, 110, false);
    addClip(PlayerAction::Hurt,   "hurt",   4, 100, false);
    addClip(PlayerAction::Fall,   "fall",   4, 120, false);
    reset();
}

void PlayerAnimator::reset()
{
    currentAction = PlayerAction::Idle;
    frameIndex = 0;
    frameElapsedMs = 0;
    inactiveElapsedMs = 0;
    landingRemainingMs = 0;
    attackActivationPending = false;
    currentActionFinished = false;
    terminalFall = false;
}

void PlayerAnimator::update(int deltaMs, bool grounded,
                            double verticalSpeed, double horizontalSpeed,
                            double airborneHeight)
{
    if (currentAction == PlayerAction::Fall) {
        if (terminalFall) {
            advanceTimedClip(deltaMs);
            return;
        }

        if (grounded) {
            chooseLocomotion(true, horizontalSpeed);
            return;
        }

        if (verticalSpeed < 0.0) {
            transitionTo(PlayerAction::Jump);
            updateJumpFrame(verticalSpeed);
            return;
        }

        if (advanceTimedClip(deltaMs)) {
            frameIndex = 0;
            frameElapsedMs = 0;
            currentActionFinished = false;
        }
        return;
    }

    if (currentAction == PlayerAction::Hurt) {
        if (advanceTimedClip(deltaMs))
            chooseLocomotion(grounded, horizontalSpeed);
        return;
    }

    if (currentAction == PlayerAction::Attack) {
        const int previousFrame = frameIndex;
        if (advanceTimedClip(deltaMs)) {
            chooseLocomotion(grounded, horizontalSpeed);
        } else if (previousFrame != 1 && frameIndex == 1) {
            // attack_02 是唯一生效帧。
            attackActivationPending = true;
        }
        return;
    }

    if (!grounded
        && verticalSpeed > 0.0
        && airborneHeight > GameConfig::HighFallThreshold) {
        transitionTo(PlayerAction::Fall);
        terminalFall = false;
        return;
    }

    if (!grounded) {
        if (currentAction != PlayerAction::Jump)
            transitionTo(PlayerAction::Jump);
        updateJumpFrame(verticalSpeed);
        inactiveElapsedMs = 0;
        return;
    }

    if (currentAction == PlayerAction::Jump) {
        const int lastJumpFrame =
            clips[PlayerAction::Jump].frames.size() - 1;
        if (landingRemainingMs == 0) {
            frameIndex = lastJumpFrame;
            landingRemainingMs = 70;
            return;
        }

        landingRemainingMs -= deltaMs;
        if (landingRemainingMs > 0)
            return;

        chooseLocomotion(true, horizontalSpeed);
    }

    if (horizontalSpeed >= 4.0) {
        if (currentAction != PlayerAction::Run)
            transitionTo(PlayerAction::Run);
        advanceTimedClip(deltaMs);
        inactiveElapsedMs = 0;
        return;
    }

    if (horizontalSpeed > 0.1) {
        if (currentAction != PlayerAction::Walk)
            transitionTo(PlayerAction::Walk);
        advanceTimedClip(deltaMs);
        inactiveElapsedMs = 0;
        return;
    }

    inactiveElapsedMs += deltaMs;
    if (inactiveElapsedMs >= 9000) {
        if (currentAction != PlayerAction::Sleep)
            transitionTo(PlayerAction::Sleep);
        advanceTimedClip(deltaMs);
    } else if (inactiveElapsedMs >= 4000) {
        if (currentAction != PlayerAction::Sit)
            transitionTo(PlayerAction::Sit);
        advanceTimedClip(deltaMs);
    } else {
        if (currentAction != PlayerAction::Idle)
            transitionTo(PlayerAction::Idle);
        advanceTimedClip(deltaMs);
    }
}

bool PlayerAnimator::requestAttack()
{
    if (currentAction == PlayerAction::Fall
        || currentAction == PlayerAction::Hurt
        || currentAction == PlayerAction::Attack) {
        return false;
    }

    transitionTo(PlayerAction::Attack);
    return true;
}

void PlayerAnimator::requestHurt(int remainingHealth, bool strongKnockback)
{
    if (remainingHealth <= 0 || strongKnockback) {
        transitionTo(PlayerAction::Fall);
        terminalFall = true;
        return;
    }

    if (currentAction != PlayerAction::Fall)
        transitionTo(PlayerAction::Hurt);
}

PlayerAction PlayerAnimator::action() const
{
    return currentAction;
}

const QPixmap &PlayerAnimator::currentFrame() const
{
    return clips[currentAction].frames[frameIndex];
}

bool PlayerAnimator::isCurrentActionFinished() const
{
    return currentActionFinished;
}

bool PlayerAnimator::takeAttackActivation()
{
    const bool pending = attackActivationPending;
    attackActivationPending = false;
    return pending;
}

void PlayerAnimator::addClip(PlayerAction action, const QString &name,
                             int frameCount, int frameDurationMs, bool loops)
{
    Clip clip;
    clip.frameDurationMs = frameDurationMs;
    clip.loops = loops;

    for (int frame = 1; frame <= frameCount; ++frame) {
        clip.frames.append(QPixmap(
            QStringLiteral(":/images/player/%1_%2.png")
                .arg(name)
                .arg(frame, 2, 10, QLatin1Char('0'))));
    }

    clips.insert(action, clip);
}

void PlayerAnimator::transitionTo(PlayerAction nextAction)
{
    if (currentAction == nextAction)
        return;

    currentAction = nextAction;
    frameIndex = 0;
    frameElapsedMs = 0;
    landingRemainingMs = 0;
    attackActivationPending = false;
    currentActionFinished = false;
    if (nextAction != PlayerAction::Fall)
        terminalFall = false;
}

void PlayerAnimator::updateJumpFrame(double verticalSpeed)
{
    const int frameCount = clips[PlayerAction::Jump].frames.size();
    if (verticalSpeed < -12.0)
        frameIndex = 0;                 // 起跳
    else if (verticalSpeed < -7.0)
        frameIndex = 1;                 // 快速上升
    else if (verticalSpeed < -2.0)
        frameIndex = 2;                 // 缓慢上升
    else if (verticalSpeed <= 2.0)
        frameIndex = 3;                 // 最高点
    else if (verticalSpeed < 7.0)
        frameIndex = 4;                 // 下降
    else
        frameIndex = frameCount - 2;    // 快速下降，末帧留给落地
}

void PlayerAnimator::chooseLocomotion(bool grounded, double horizontalSpeed)
{
    if (!grounded)
        transitionTo(PlayerAction::Jump);
    else if (horizontalSpeed >= 4.0)
        transitionTo(PlayerAction::Run);
    else if (horizontalSpeed > 0.1)
        transitionTo(PlayerAction::Walk);
    else
        transitionTo(PlayerAction::Idle);
}

bool PlayerAnimator::advanceTimedClip(int deltaMs)
{
    Clip &clip = clips[currentAction];
    frameElapsedMs += deltaMs;

    while (frameElapsedMs >= clip.frameDurationMs) {
        frameElapsedMs -= clip.frameDurationMs;
        ++frameIndex;

        if (frameIndex < clip.frames.size())
            continue;

        if (clip.loops) {
            frameIndex = 0;
            return false;
        }

        frameIndex = clip.frames.size() - 1;
        currentActionFinished = true;
        return true;
    }

    return false;
}
