/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * QwtPolar Widget Library
 * Copyright (C) 2008   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GPL License, Version 2.0
 *****************************************************************************/

#ifndef QWT_POLAR_CURVE_H
#define QWT_POLAR_CURVE_H

#include "qwt_global.h"
#include "qwt_data.h"
#include "qwt_polar_item.h"

class QPainter;
class QwtSymbol;

class QWT_EXPORT QwtPolarCurve: public QwtPolarItem
{
public:
    enum CurveStyle
    {
        NoCurve,
        Lines,
        UserCurve = 100
    };

    explicit QwtPolarCurve();
    explicit QwtPolarCurve(const QwtText &title);
    explicit QwtPolarCurve(const QString &title);

    virtual ~QwtPolarCurve();

    virtual int rtti() const;

    void setData(const QwtData &data);
    QwtData &data();
    const QwtData &data() const;

    int dataSize() const;
    inline double radius(int i) const;
    inline double azimuth(int i) const;

    void setPen(const QPen &);
    const QPen &pen() const;

    void setStyle(CurveStyle style);
    CurveStyle style() const;

    void setSymbol(const QwtSymbol &s);
    const QwtSymbol& symbol() const;

    virtual void draw(QPainter *p, 
        const QwtScaleMap &azimuthMap, const QwtScaleMap &radialMap,
        const QwtDoublePoint &pole, double radius,
        const QwtDoubleRect &canvasRect) const;

    virtual void draw(QPainter *p,
        const QwtScaleMap &azimuthMap, const QwtScaleMap &radialMap,
        const QwtDoublePoint &pole,
        int from, int to) const;

    virtual void updateLegend(QwtLegend *) const;

protected:

    void init();

    virtual void drawCurve(QPainter *, int style,
        const QwtScaleMap &azimuthMap, const QwtScaleMap &radialMap,
        const QwtDoublePoint &pole,
        int from, int to) const;

    virtual void drawSymbols(QPainter *, const QwtSymbol &,
        const QwtScaleMap &azimuthMap, const QwtScaleMap &radialMap,
        const QwtDoublePoint &pole,
        int from, int to) const;

    void drawLines(QPainter *, 
        const QwtScaleMap &azimuthMap, const QwtScaleMap &radialMap,
        const QwtDoublePoint &pole,
        int from, int to) const;

private:
    QwtData *d_points;

    class PrivateData;
    PrivateData *d_data;
};

//! \return the the curve data
inline QwtData &QwtPolarCurve::data()
{
    return *d_points;
}

//! \return the the curve data
inline const QwtData &QwtPolarCurve::data() const
{
    return *d_points;
}

/*!
    \param i index
    \return azimuth at position i
*/
inline double QwtPolarCurve::azimuth(int i) const 
{ 
    return d_points->x(i); 
}

/*!
    \param i index
    \return radius at position i
*/
inline double QwtPolarCurve::radius(int i) const 
{ 
    return d_points->y(i); 
}

#endif
