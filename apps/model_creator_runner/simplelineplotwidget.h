#ifndef SIMPLELINEPLOTWIDGET_H
#define SIMPLELINEPLOTWIDGET_H

#include <QVector>
#include <QWidget>

class SimpleLinePlotWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SimpleLinePlotWidget(const QString &title, QWidget *parent = nullptr);

    void setSeries(const QVector<QPointF> &points);
    void setStatusMessage(const QString &message);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QVector<QPointF> series;
    QString title;
    QString statusMessage;
};

#endif // SIMPLELINEPLOTWIDGET_H
