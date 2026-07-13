#include "hudwidget.h"

#include <QHBoxLayout>
#include <QLabel>

HudWidget::HudWidget(QWidget *parent)
    : QWidget(parent)
    , scoreLabel(new QLabel(this))
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(16, 8, 16, 8);
    layout->addWidget(scoreLabel);
    layout->addStretch();
    setScore(0);
}

void HudWidget::setScore(int score)
{
    scoreLabel->setText(QStringLiteral("分数：%1").arg(score));
}
