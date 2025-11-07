#pragma once

#include <QWidget>

class WindowAnimation
{
public:
    enum AnimationType
    {
        Fade,
        SlideLeft,
        SlideRight,
        SlideTop,
        SlideBottom,
        Scale,
        Flip
    };
    // Add show animation to window
    static void addShowAnimation(QWidget* window, AnimationType type = Scale);

    // Add minimize animation to window
    static void addMinimizeAnimation(QWidget* window, AnimationType type = Scale);

    // Add close animation to window
    static void addCloseAnimation(QWidget* window, AnimationType type = Fade);

private:
    // Set up animation effect
    static void setupAnimation(QWidget* window);

    // Set up opacity animation
    static void setupOpacityAnimation(QWidget* window, bool isShow);

    // Set up slide animation
    static void setupSlideAnimation(QWidget* window, bool isShow, AnimationType type);

    // Set up scale animation
    static void setupScaleAnimation(QWidget* window, bool isShow);
};