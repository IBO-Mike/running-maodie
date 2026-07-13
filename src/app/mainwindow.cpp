#include "mainwindow.h"
#include "pages/gamepage.h"
#include "pages/homepage.h"

#include <QStackedWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , pageStack(new QStackedWidget(this))
    , homePage(new HomePage(pageStack))
    , gamePage(new GamePage(pageStack))
{
    setWindowTitle(QStringLiteral("Running Maodie"));
    resize(1100, 700);
    setMinimumSize(800, 520);
    setCentralWidget(pageStack);

    pageStack->addWidget(homePage);
    pageStack->addWidget(gamePage);
    pageStack->setCurrentWidget(homePage);

    connect(homePage, &HomePage::startGameRequested,
            this, [this](double backgroundOffset) {
        pageStack->setCurrentWidget(gamePage);
        gamePage->startGame(backgroundOffset);
    });

    connect(gamePage, &GamePage::backToHomeRequested, this, [this] {
        homePage->resetHome();
        pageStack->setCurrentWidget(homePage);
    });
}
