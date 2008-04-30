/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * QwtPolar Widget Library
 * Copyright (C) 2008   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GPL License, Version 2.0
 *****************************************************************************/

#ifndef QWT_POLAR_PANNER_H
#define QWT_POLAR_PANNER_H 1

#include "qwt_global.h"
#include "qwt_panner.h"

class QwtPolarPlot;
class QwtPolarCanvas;

class QWT_EXPORT QwtPolarPanner: public QwtPanner
{
    Q_OBJECT

public:
    explicit QwtPolarPanner(QwtPolarCanvas *);
    virtual ~QwtPolarPanner();

    QwtPolarPlot *plot();
    const QwtPolarPlot *plot() const;

    QwtPolarCanvas *canvas();
    const QwtPolarCanvas *canvas() const;

    void setScaleEnabled(int scaleId, bool on = true);
    bool isScaleEnabled(int scaleId) const;

protected slots:
    virtual void movePlot(int dx, int dy);

private:
    class PrivateData;
    PrivateData *d_data;
};

#endif
