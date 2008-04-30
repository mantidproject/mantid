/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * QwtPolar Widget Library
 * Copyright (C) 2008   Uwe Rathmann
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GPL License, Version 2.0
 *****************************************************************************/

#include <cmath>
#include <qglobal.h>
#include "qwt_math.h"
#include "qwt_polar_point.h"

QwtPolarPoint::QwtPolarPoint(const QwtDoublePoint &p)
{
    d_radius = ::sqrt(qwtSqr(p.x()) + qwtSqr(p.y()) );
    d_azimuth = ::atan2(p.y(), p.x());
}

void QwtPolarPoint::setPoint(const QwtDoublePoint &p)
{
    d_radius = ::sqrt(qwtSqr(p.x()) + qwtSqr(p.y()) );
    d_azimuth = ::atan2(p.y(), p.x());
}

QwtDoublePoint QwtPolarPoint::toPoint() const
{
    if ( d_radius <= 0.0 )
        return QwtDoublePoint(0.0, 0.0);

    const double x = d_radius * ::cos(d_azimuth);
    const double y = d_radius * ::sin(d_azimuth);

    return QwtDoublePoint(x, y);
}

bool QwtPolarPoint::operator==(const QwtPolarPoint &other) const
{
    return d_radius == other.d_radius && d_azimuth == other.d_azimuth;
}

bool QwtPolarPoint::operator!=(const QwtPolarPoint &other) const
{
    return d_radius != other.d_radius || d_azimuth != other.d_azimuth;
}

QwtPolarPoint QwtPolarPoint::normalized() const
{
    const double radius = qwtMax(d_radius, 0.0);
    const double azimuth = ::fmod(d_azimuth, 2 * M_PI);

    return QwtPolarPoint(radius, azimuth);
}

