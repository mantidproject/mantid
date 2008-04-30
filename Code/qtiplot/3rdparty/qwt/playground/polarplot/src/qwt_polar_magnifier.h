/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * QwtPolar Widget Library
 * Copyright (C) 2008   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GPL License, Version 2.0
 *****************************************************************************/

#ifndef QWT_POLAR_MAGNIFIER_H
#define QWT_POLAR_MAGNIFIER_H 1

#include "qwt_global.h"
#include "qwt_magnifier.h"

class QwtPolarPlot;
class QwtPolarCanvas;

class QWT_EXPORT QwtPolarMagnifier: public QwtMagnifier
{
    Q_OBJECT

public:
    explicit QwtPolarMagnifier(QwtPolarCanvas *);
    virtual ~QwtPolarMagnifier();

    QwtPolarPlot *plot();
    const QwtPolarPlot *plot() const;

    QwtPolarCanvas *canvas();
    const QwtPolarCanvas *canvas() const;

protected:
    virtual void rescale(double factor);
};

#endif
