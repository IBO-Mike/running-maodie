#ifndef HUDWIDGET_H
#define HUDWIDGET_H

#include <QWidget>

class QLabel;

class HudWidget : public QWidget
{
    Q_OBJECT

public:
    explicit HudWidget(QWidget *parent = nullptr);

public slots:
    void setScore(int score);

private:
    QLabel *scoreLabel;
};

#endif // HUDWIDGET_H
