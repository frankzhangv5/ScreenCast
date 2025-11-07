#include "WindowAnimation.h"

#include <QApplication>
#include <QGraphicsOpacityEffect>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QScreen>
#include <QTimer>

void WindowAnimation::addShowAnimation(QWidget* window, AnimationType type)
{
    switch (type)
    {
        case Fade:
            // Set window initial state to transparent
            {
                QGraphicsOpacityEffect* opacityEffect = new QGraphicsOpacityEffect(window);
                opacityEffect->setOpacity(0.0);
                window->setGraphicsEffect(opacityEffect);
            }
            // Show window
            window->show();
            // Create opacity animation
            setupOpacityAnimation(window, true);
            break;
        case SlideLeft:
        case SlideRight:
        case SlideTop:
        case SlideBottom:
            window->show();
            setupSlideAnimation(window, true, type);
            break;
        case Scale:
            window->show();
            setupScaleAnimation(window, true);
            break;
        default:
            // Set window initial state to transparent
            {
                QGraphicsOpacityEffect* opacityEffect = new QGraphicsOpacityEffect(window);
                opacityEffect->setOpacity(0.0);
                window->setGraphicsEffect(opacityEffect);
            }
            // Show window
            window->show();
            // Create opacity animation
            setupOpacityAnimation(window, true);
            break;
    }
}

void WindowAnimation::addMinimizeAnimation(QWidget* window, AnimationType type)
{
    switch (type)
    {
        case Fade:
            // Create opacity animation
            setupOpacityAnimation(window, false);
            // Minimize window after animation
            QTimer::singleShot(300, [window]() { window->showMinimized(); });
            break;
        case SlideLeft:
        case SlideRight:
        case SlideTop:
        case SlideBottom:
            setupSlideAnimation(window, false, type);
            // Minimize window after animation
            QTimer::singleShot(300, [window]() { window->showMinimized(); });
            break;
        case Scale:
            setupScaleAnimation(window, false);
            // Minimize window after animation
            QTimer::singleShot(300, [window]() { window->showMinimized(); });
            break;
        default:
            // Create opacity animation
            setupOpacityAnimation(window, false);
            // Minimize window after animation
            QTimer::singleShot(300, [window]() { window->showMinimized(); });
            break;
    }
}

void WindowAnimation::addCloseAnimation(QWidget* window, AnimationType type)
{
    switch (type)
    {
        case Fade:
            // Create opacity animation
            setupOpacityAnimation(window, false);
            // Close window after animation
            QTimer::singleShot(300, [window]() { window->close(); });
            break;
        case SlideLeft:
        case SlideRight:
        case SlideTop:
        case SlideBottom:
            setupSlideAnimation(window, false, type);
            // Close window after animation
            QTimer::singleShot(300, [window]() { window->close(); });
            break;
        case Scale:
            setupScaleAnimation(window, false);
            // Close window after animation
            QTimer::singleShot(300, [window]() { window->close(); });
            break;
        default:
            // Create opacity animation
            setupOpacityAnimation(window, false);
            // Close window after animation
            QTimer::singleShot(300, [window]() { window->close(); });
            break;
    }
}

void WindowAnimation::setupOpacityAnimation(QWidget* window, bool isShow)
{
    QGraphicsOpacityEffect* opacityEffect = qobject_cast<QGraphicsOpacityEffect*>(window->graphicsEffect());
    if (!opacityEffect)
    {
        opacityEffect = new QGraphicsOpacityEffect(window);
        window->setGraphicsEffect(opacityEffect);
    }

    QPropertyAnimation* animation = new QPropertyAnimation(opacityEffect, "opacity");
    animation->setDuration(300);
    animation->setStartValue(isShow ? 0.0 : 1.0);
    animation->setEndValue(isShow ? 1.0 : 0.0);
    animation->setEasingCurve(QEasingCurve::OutCubic);

    // Delete animation object after animation
    QObject::connect(animation, &QPropertyAnimation::finished, [animation]() { animation->deleteLater(); });

    animation->start();
}

void WindowAnimation::setupSlideAnimation(QWidget* window, bool isShow, AnimationType type)
{
    // Get screen size
    QScreen* screen = QApplication::primaryScreen();
    QRect screenGeometry = screen->availableGeometry();

    // Save original position
    QRect originalGeometry = window->geometry();

    // Set start position based on animation type
    QRect startGeometry = originalGeometry;
    QRect endGeometry = originalGeometry;

    switch (type)
    {
        case SlideLeft:
            if (isShow)
            {
                startGeometry.moveTo(-originalGeometry.width(), originalGeometry.y());
            }
            else
            {
                endGeometry.moveTo(-originalGeometry.width(), originalGeometry.y());
            }
            break;
        case SlideRight:
            if (isShow)
            {
                startGeometry.moveTo(screenGeometry.width(), originalGeometry.y());
            }
            else
            {
                endGeometry.moveTo(screenGeometry.width(), originalGeometry.y());
            }
            break;
        case SlideTop:
            if (isShow)
            {
                startGeometry.moveTo(originalGeometry.x(), -originalGeometry.height());
            }
            else
            {
                endGeometry.moveTo(originalGeometry.x(), -originalGeometry.height());
            }
            break;
        case SlideBottom:
            if (isShow)
            {
                startGeometry.moveTo(originalGeometry.x(), screenGeometry.height());
            }
            else
            {
                endGeometry.moveTo(originalGeometry.x(), screenGeometry.height());
            }
            break;
        default:
            break;
    }

    // Set start position
    if (isShow)
    {
        window->setGeometry(startGeometry);
    }

    // Create position animation
    QPropertyAnimation* animation = new QPropertyAnimation(window, "geometry");
    animation->setDuration(300);
    animation->setStartValue(isShow ? startGeometry : originalGeometry);
    animation->setEndValue(isShow ? originalGeometry : endGeometry);
    animation->setEasingCurve(QEasingCurve::OutCubic);

    // Delete animation object after animation
    QObject::connect(animation, &QPropertyAnimation::finished, [animation]() { animation->deleteLater(); });

    animation->start();
}

void WindowAnimation::setupScaleAnimation(QWidget* window, bool isShow)
{
    // Save original size
    QRect originalGeometry = window->geometry();

    // Set start size
    QRect startGeometry = originalGeometry;
    if (isShow)
    {
        startGeometry.setWidth(0);
        startGeometry.setHeight(0);
        // Set scaling center point
        startGeometry.moveCenter(originalGeometry.center());
    }

    // Create size animation
    QPropertyAnimation* animation = new QPropertyAnimation(window, "geometry");
    animation->setDuration(300);
    animation->setStartValue(isShow ? startGeometry : originalGeometry);
    animation->setEndValue(isShow ? originalGeometry : startGeometry);
    animation->setEasingCurve(QEasingCurve::OutCubic);

    // Delete animation object after animation
    QObject::connect(animation, &QPropertyAnimation::finished, [animation]() { animation->deleteLater(); });

    animation->start();
}