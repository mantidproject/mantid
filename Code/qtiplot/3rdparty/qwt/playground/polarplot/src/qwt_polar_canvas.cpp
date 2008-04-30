/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * QwtPolar Widget Library
 * Copyright (C) 2008   Uwe Rathmann
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GPL License, Version 2.0
 *****************************************************************************/

#include <qpainter.h>
#include <qevent.h>
#include <qpixmap.h>
#if QT_VERSION >= 0x040000
#ifdef Q_WS_X11
#include <qx11info_x11.h>
#endif
#endif
#include "qwt_polar_canvas.h"
#include "qwt_polar_plot.h"

class QwtPolarCanvas::PrivateData
{
public:
    PrivateData():
        paintAttributes(0),
        cache(NULL)
    {
    }

    ~PrivateData()
    {
        delete cache;
    }

    int paintAttributes;
    QPixmap *cache;
};

QwtPolarCanvas::QwtPolarCanvas(QwtPolarPlot *plot):
    QFrame(plot)
{
    d_data = new PrivateData;

#if QT_VERSION >= 0x040100
    setAutoFillBackground(true);
#endif

#if QT_VERSION < 0x040000
#ifndef QT_NO_CURSOR
    setWFlags(Qt::WNoAutoErase);
    setCursor(Qt::crossCursor);
#endif
#else
#ifndef QT_NO_CURSOR
    setCursor(Qt::CrossCursor);
#endif
#endif // >= 0x040000

    setPaintAttribute(PaintCached, true);
}

QwtPolarCanvas::~QwtPolarCanvas()
{
    delete d_data;
}

QwtPolarPlot *QwtPolarCanvas::plot()
{
    QWidget *w = parentWidget();
    if ( w && w->inherits("QwtPolarPlot") )
        return (QwtPolarPlot *)w;

    return NULL;
}

const QwtPolarPlot *QwtPolarCanvas::plot() const
{
    const QWidget *w = parentWidget();
    if ( w && w->inherits("QwtPolarPlot") )
        return (QwtPolarPlot *)w;

    return NULL;
}

void QwtPolarCanvas::setPaintAttribute(PaintAttribute attribute, bool on)
{
    if ( bool(d_data->paintAttributes & attribute) == on )
        return;

    if ( on )
        d_data->paintAttributes |= attribute;
    else
        d_data->paintAttributes &= ~attribute;

    switch(attribute)
    {
        case PaintCached:
        {
            if ( on )
            {
                if ( d_data->cache == NULL )
                    d_data->cache = new QPixmap();

                if ( isVisible() )
                {
                    const QRect cr = contentsRect();
                    *d_data->cache = QPixmap::grabWidget(this,
                        cr.x(), cr.y(), cr.width(), cr.height() );
                }
            }
            else
            {
                delete d_data->cache;
                d_data->cache = NULL;
            }
            break;
        }
    }
}

bool QwtPolarCanvas::testPaintAttribute(PaintAttribute attribute) const
{
    return (d_data->paintAttributes & attribute) != 0;
}

//! Return the paint cache, might be null
QPixmap *QwtPolarCanvas::paintCache()
{
    return d_data->cache;
}

//! Return the paint cache, might be null
const QPixmap *QwtPolarCanvas::paintCache() const
{
    return d_data->cache;
}

//! Invalidate the internal paint cache
void QwtPolarCanvas::invalidatePaintCache()
{
    if ( d_data->cache )
        *d_data->cache = QPixmap();
}

void QwtPolarCanvas::paintEvent(QPaintEvent *event)
{
#if QT_VERSION >= 0x040000
    QPainter painter(this);

    if ( !contentsRect().contains( event->rect() ) )
    {
        painter.save();
        painter.setClipRegion( event->region() & frameRect() );
        drawFrame( &painter );
        painter.restore();
    }

    painter.setClipRegion(event->region() & contentsRect());

    drawContents( &painter );
#else // QT_VERSION < 0x040000
    QFrame::paintEvent(event);
#endif
}

void QwtPolarCanvas::drawContents(QPainter *painter)
{
    if ( d_data->paintAttributes & PaintCached && d_data->cache
        && d_data->cache->size() == contentsRect().size() )
    {
        painter->drawPixmap(contentsRect().topLeft(), *d_data->cache);
    }
    else
    {
        QwtPolarPlot *plt = plot();
        if ( plt )
        {
            const bool doAutoReplot = plt->autoReplot();
            plt->setAutoReplot(false);
            drawCanvas(painter, QwtDoubleRect(contentsRect()) );
            plt->setAutoReplot(doAutoReplot);
        }
    }
}

void QwtPolarCanvas::drawCanvas(QPainter *painter, 
    const QwtDoubleRect& canvasRect)
{
    if ( !canvasRect.isValid() )
        return;

    if ( d_data->paintAttributes & PaintCached && d_data->cache )
    {
        *d_data->cache = QPixmap(contentsRect().size());

#ifdef Q_WS_X11
#if QT_VERSION >= 0x040000
        if ( d_data->cache->x11Info().screen() != x11Info().screen() )
            d_data->cache->x11SetScreen(x11Info().screen());
#else
        if ( d_data->cache->x11Screen() != x11Screen() )
            d_data->cache->x11SetScreen(x11Screen());
#endif
#endif

        d_data->cache->fill(this, d_data->cache->rect().topLeft());
        QPainter cachePainter(d_data->cache);
        cachePainter.translate(-contentsRect().x(),
            -contentsRect().y());

        plot()->drawCanvas(&cachePainter, canvasRect);

        cachePainter.end();

        painter->drawPixmap(canvasRect.toRect(), *d_data->cache);
    }
    else
        plot()->drawCanvas(painter, canvasRect);
}
