#include "WindowHook.h"

#include "WindowAnimation.h"

#include <QApplication>
#include <QDebug>
#include <QDialog>
#include <QMainWindow>
#include <QWidget>

WindowHook* WindowHook::m_instance = nullptr;

WindowHook::WindowHook(QObject* parent) : QObject(parent) {}

void WindowHook::installHook()
{
    if (!m_instance)
    {
        m_instance = new WindowHook(qApp);
        qApp->installEventFilter(m_instance);
    }
}

bool WindowHook::eventFilter(QObject* obj, QEvent* event)
{
    // Check if object is QWidget or its subclass
    QWidget* widget = qobject_cast<QWidget*>(obj);
    if (!widget)
    {
        return QObject::eventFilter(obj, event);
    }

    // Apply animation effects only to window classes, exclude child controls
    // Check if it's a window (has window flags and is not a child window)
    if (!widget->isWindow() || widget->parentWidget())
    {
        return QObject::eventFilter(obj, event);
    }

    // Handle different event types
    switch (event->type())
    {
        case QEvent::Show:
            // Apply show animation
            WindowAnimation::addShowAnimation(widget);
            widget->setFocusPolicy(Qt::StrongFocus);
            break;

        case QEvent::WindowStateChange:
            // Check if it's minimize event
            if (widget->isMinimized())
            {
                // Apply minimize animation
                qDebug() << "Minimize event";
                WindowAnimation::addMinimizeAnimation(widget);
                // Prevent event propagation to avoid immediate window minimization
                return true;
            }
            break;

        case QEvent::Close:
            // Not apply close animation to avoid bug
            break;

        default:
            break;
    }

    return QObject::eventFilter(obj, event);
}