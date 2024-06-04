#ifndef GAMEWINDOW_HPP
#define GAMEWINDOW_HPP

#include <QWidget>
#include <QPaintEvent>

class GameWindow : public QWidget{
    Q_OBJECT

public:
    GameWindow(QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;
};

#endif // GAMEWINDOW_HPP