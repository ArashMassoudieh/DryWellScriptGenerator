#include "simplelineplotwidget.h"

#include <QPainter>

namespace {
QRectF plotRect(const QRect &outer)
{
    return QRectF(outer.left() + 45, outer.top() + 24,
                  qMax(10, outer.width() - 60),
                  qMax(10, outer.height() - 48));
}
}

SimpleLinePlotWidget::SimpleLinePlotWidget(const QString &titleText, QWidget *parent)
    : QWidget(parent), title(titleText)
{
    setMinimumHeight(180);
}

void SimpleLinePlotWidget::setSeries(const QVector<QPointF> &points)
{
    series = points;
    update();
}

void SimpleLinePlotWidget::setStatusMessage(const QString &message)
{
    statusMessage = message;
    update();
}

void SimpleLinePlotWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.fillRect(rect(), QColor(250, 250, 250));
    p.setRenderHint(QPainter::Antialiasing, true);

    p.setPen(QPen(Qt::black, 1));
    p.drawText(QRect(10, 4, width() - 20, 18), Qt::AlignLeft | Qt::AlignVCenter, title);

    const QRectF r = plotRect(rect());
    p.setPen(QPen(QColor(200, 200, 200), 1));
    p.drawRect(r);

    if (series.isEmpty()) {
        p.setPen(QPen(Qt::darkGray, 1));
        const QString text = statusMessage.isEmpty() ? QStringLiteral("No data loaded.") : statusMessage;
        p.drawText(r.toRect(), Qt::AlignCenter, text);
        return;
    }

    qreal minX = series.first().x();
    qreal maxX = series.first().x();
    qreal minY = series.first().y();
    qreal maxY = series.first().y();
    for (const QPointF &pt : series) {
        minX = qMin(minX, pt.x());
        maxX = qMax(maxX, pt.x());
        minY = qMin(minY, pt.y());
        maxY = qMax(maxY, pt.y());
    }

    if (qFuzzyCompare(minX, maxX)) maxX = minX + 1.0;
    if (qFuzzyCompare(minY, maxY)) maxY = minY + 1.0;

    QPainterPath path;
    bool first = true;
    for (const QPointF &pt : series) {
        const qreal nx = (pt.x() - minX) / (maxX - minX);
        const qreal ny = (pt.y() - minY) / (maxY - minY);
        const QPointF mapped(r.left() + nx * r.width(),
                             r.bottom() - ny * r.height());
        if (first) {
            path.moveTo(mapped);
            first = false;
        } else {
            path.lineTo(mapped);
        }
    }

    p.setPen(QPen(QColor(25, 118, 210), 2));
    p.drawPath(path);

    p.setPen(QPen(Qt::darkGray, 1));
    p.drawText(QRectF(r.left(), r.bottom() + 2, r.width(), 16), Qt::AlignLeft | Qt::AlignVCenter,
               QStringLiteral("x: %1 .. %2").arg(minX).arg(maxX));
    p.drawText(QRectF(r.left(), r.top() - 16, r.width(), 16), Qt::AlignRight | Qt::AlignVCenter,
               QStringLiteral("y: %1 .. %2").arg(minY).arg(maxY));
}
