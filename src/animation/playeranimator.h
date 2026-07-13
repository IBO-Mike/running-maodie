#ifndef PLAYERANIMATOR_H
#define PLAYERANIMATOR_H

#include <QHash>
#include <QPixmap>
#include <QVector>

enum class PlayerAction {
    Idle,
    Walk,
    Run,
    Jump,
    Sit,
    Sleep,
    Attack,
    Hurt,
    Fall
};

class PlayerAnimator
{
public:
    PlayerAnimator();

    void reset();
    void update(int deltaMs, bool grounded, double verticalSpeed,
                double horizontalSpeed, double airborneHeight);

    bool requestAttack();
    void requestHurt(int remainingHealth, bool strongKnockback);

    PlayerAction action() const;
    const QPixmap &currentFrame() const;
    bool isCurrentActionFinished() const;
    bool takeAttackActivation();

private:
    struct Clip {
        QVector<QPixmap> frames;
        int frameDurationMs = 100;
        bool loops = true;
    };

    void addClip(PlayerAction action, const QString &name, int frameCount,
                 int frameDurationMs, bool loops);
    void transitionTo(PlayerAction nextAction);
    void updateJumpFrame(double verticalSpeed);
    void chooseLocomotion(bool grounded, double horizontalSpeed);
    bool advanceTimedClip(int deltaMs);

    QHash<PlayerAction, Clip> clips;
    PlayerAction currentAction = PlayerAction::Idle;
    int frameIndex = 0;
    int frameElapsedMs = 0;
    int inactiveElapsedMs = 0;
    int landingRemainingMs = 0;
    bool attackActivationPending = false;
    bool currentActionFinished = false;
    bool terminalFall = false;
};

#endif // PLAYERANIMATOR_H
