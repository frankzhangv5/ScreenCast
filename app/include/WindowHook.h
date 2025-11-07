#pragma once

#include "WindowAnimation.h"

#include <QEvent>
#include <QObject>

class WindowHook : public QObject
{
    Q_OBJECT

public:
    explicit WindowHook(QObject* parent = nullptr);
    static void installHook();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    static WindowHook* m_instance;
};