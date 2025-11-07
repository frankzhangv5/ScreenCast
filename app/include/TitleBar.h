#pragma once

#include <QLabel>
#include <QToolBar>
#include <QWidget>

class TitleBar : public QWidget
{
    Q_OBJECT

public:
    explicit TitleBar(QWidget* parent = nullptr, const QString& title = "");

    void setTitle(const QString& title);
    void setTitle(const QPixmap& pixmap);
    QString title() const;

    // Get toolbar for external icon settings, etc.
    QToolBar* getToolBar() const;

signals:
    void closeClicked();
    void minimizeClicked();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void setupUI();

    QLabel* m_titleLabel;
    QToolBar* m_toolBar; // Toolbar
    QLabel* m_minimizeButton;
    QLabel* m_closeButton;
    QPoint m_dragPosition;
    QWidget* m_parent;
};