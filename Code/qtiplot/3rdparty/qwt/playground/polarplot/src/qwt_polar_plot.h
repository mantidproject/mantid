/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * QwtPolar Widget Library
 * Copyright (C) 2008   Uwe Rathmann
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GPL License, Version 2.0
 *****************************************************************************/

// vim: expandtab

#ifndef QWT_POLAR_PLOT_H
#define QWT_POLAR_PLOT_H 1

#include <qframe.h>
#include "qwt_global.h"
#include "qwt_double_interval.h"
#include "qwt_scale_map.h"
#include "qwt_polar.h"
#include "qwt_polar_point.h"
#include "qwt_polar_itemdict.h"

class QwtRoundScaleDraw;
class QwtScaleEngine;
class QwtScaleDiv;
class QwtTextLabel;
class QwtPolarCanvas;

class QWT_EXPORT QwtPolarPlot: public QFrame, public QwtPolarItemDict
{
    Q_OBJECT

public:
    enum LegendPosition
    {
        LeftLegend,
        RightLegend,
        BottomLegend,
        TopLegend,

        ExternalLegend
    };

    explicit QwtPolarPlot( QWidget *parent = NULL);
    QwtPolarPlot(const QwtText &title, QWidget *parent = NULL);

    virtual ~QwtPolarPlot();

    void setTitle(const QString &);
    void setTitle(const QwtText &);

    QwtText title() const;

    QwtTextLabel *titleLabel();
    const QwtTextLabel *titleLabel() const;

    void setAutoReplot(bool tf = true); 
    bool autoReplot() const;

    void setAutoScale(int scaleId, bool enable);
    bool autoScale(int scaleId) const;

    void setScaleMaxMinor(int scaleId, int maxMinor);
    int scaleMaxMinor(int scaleId) const;

    int scaleMaxMajor(int scaleId) const;
    void setScaleMaxMajor(int scaleId, int maxMajor);

    QwtScaleEngine *scaleEngine(int scaleId);
    const QwtScaleEngine *scaleEngine(int scaleId) const;
    void setScaleEngine(int scaleId, QwtScaleEngine *);

    void setScale(int scaleId, double min, double max, double step = 0);

    void setScaleDiv(int scaleId, const QwtScaleDiv &);
    const QwtScaleDiv *scaleDiv(int scaleId) const;
    QwtScaleDiv *scaleDiv(int scaleId);

    QwtScaleMap scaleMap(int scaleId) const;

    void zoom(const QwtPolarPoint&, double factor);
    void unzoom();

    QwtPolarPoint zoomPos() const;
    double zoomFactor() const;

    virtual void polish();
    virtual QSize sizeHint() const;
    virtual QSize minimumSizeHint() const;

    // Canvas

    QwtPolarCanvas *canvas();
    const QwtPolarCanvas *canvas() const;

    void setCanvasBackground (const QBrush &c);
    const QBrush& canvasBackground() const;

    virtual void drawCanvas(QPainter *, const QwtDoubleRect &) const;

    // Legend

    void insertLegend(QwtLegend *, LegendPosition = QwtPolarPlot::RightLegend);

    QwtLegend *legend();
    const QwtLegend *legend() const;

    QwtDoubleInterval visibleInterval() const;
    QwtDoubleRect plotRect() const;

signals:
    void legendClicked(QwtPolarItem *plotItem);
    void legendChecked(QwtPolarItem *plotItem, bool on);

public slots:
    virtual void replot();
    void autoRefresh();

protected slots:
    virtual void legendItemClicked();
    virtual void legendItemChecked(bool);

protected:
    virtual bool event(QEvent *);

    virtual void updateLayout();

    virtual void drawItems(QPainter *painter, 
        const QwtScaleMap &radialMap, const QwtScaleMap &azimuthMap,
        const QwtDoublePoint &pole, double radius,
        const QwtDoubleRect &canvasRect) const;

    void updateScale(int scaleId);

    int canvasMarginHint() const;

private:
    void initPlot(const QwtText &);

    class ScaleData;
    class PrivateData;
    PrivateData *d_data;
};

#endif
