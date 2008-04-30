#include <qpen.h>
#include <qwt_data.h>
#include <qwt_symbol.h>
#include <qwt_legend.h>
#include <qwt_polar_grid.h>
#include <qwt_polar_curve.h>
#include <qwt_polar_panner.h>
#include <qwt_polar_magnifier.h>
#include <qwt_scale_engine.h>
#include "plot.h"

const QwtDoubleInterval radialInterval(0.0, 10.0);
const QwtDoubleInterval azimuthInterval(0.0, 360.0);

class Data: public QwtData
{
public:
    Data(const QwtDoubleInterval &radialInterval, 
            const QwtDoubleInterval &azimuthInterval, size_t size):
        d_radialInterval(radialInterval),
        d_azimuthInterval(azimuthInterval),
        d_size(size)
    {
    }

    virtual size_t size() const
    {
        return d_size;
    }

protected:
    QwtDoubleInterval d_radialInterval;
    QwtDoubleInterval d_azimuthInterval;
    size_t d_size;
};

class SpiralData: public Data
{
public:
    SpiralData(const QwtDoubleInterval &radialInterval, 
            const QwtDoubleInterval &azimuthInterval, size_t size):
        Data(radialInterval, azimuthInterval, size)
    {
    }

    virtual QwtData *copy() const
    {
        return new SpiralData(d_radialInterval, d_azimuthInterval, d_size);
    }

    virtual double x(size_t i) const
    {
        const double step = 4 * d_azimuthInterval.width() / d_size;
        return d_azimuthInterval.minValue() + i * step;
    }

    virtual double y(size_t i) const
    {
        const double step = d_radialInterval.width() / d_size;
        return d_radialInterval.minValue() + i * step;
    }
};

class RoseData: public Data
{
public:
    RoseData(const QwtDoubleInterval &radialInterval, 
            const QwtDoubleInterval &azimuthInterval, size_t size):
        Data(radialInterval, azimuthInterval, size)
    {
    }

    virtual QwtData *copy() const
    {
        return new RoseData(d_radialInterval, d_azimuthInterval, d_size);
    }

    virtual double x(size_t i) const
    {
        const double step = d_azimuthInterval.width() / d_size;
        return d_azimuthInterval.minValue() + i * step;
    }

    virtual double y(size_t i) const
    {
        const double a = x(i) / 360.0 * M_PI;
        return d_radialInterval.maxValue() * qwtAbs(::sin(4 * a));
    }
};


Plot::Plot(QWidget *parent):
    QwtPolarPlot(QwtText("Polar Plot Demo"), parent)
{
    setAutoReplot(false);
    setCanvasBackground(Qt::darkBlue);

    // scales 
    setScale(QwtPolar::Azimuth, 
        azimuthInterval.minValue(), azimuthInterval.maxValue(), 
        azimuthInterval.width() / 12 );

    setScaleMaxMinor(QwtPolar::Azimuth, 2);
    setScale(QwtPolar::Radius, 
        radialInterval.minValue(), radialInterval.maxValue());

    QwtPolarPanner *panner = new QwtPolarPanner(canvas());
    panner->setScaleEnabled(QwtPolar::Radius, true);
    panner->setScaleEnabled(QwtPolar::Azimuth, true);

    (void) new QwtPolarMagnifier(canvas());

    // grids, axes 

    d_grid = new QwtPolarGrid();
    d_grid->setPen(QPen(Qt::white));
    for ( int scaleId = 0; scaleId < QwtPolar::ScaleCount; scaleId++ )
    {
        d_grid->showGrid(scaleId);
        d_grid->showMinorGrid(scaleId);

        QPen minorPen(Qt::gray);
#if 0
        minorPen.setStyle(Qt::DotLine);
#endif
        d_grid->setMinorGridPen(scaleId, minorPen);
    }
    d_grid->setAxisPen(QwtPolar::AxisAzimuth, QPen(Qt::black));

    d_grid->showAxis(QwtPolar::AxisAzimuth, true);
    d_grid->showAxis(QwtPolar::AxisLeft, false);
    d_grid->showAxis(QwtPolar::AxisRight, true);
    d_grid->showAxis(QwtPolar::AxisTop, true);
    d_grid->showAxis(QwtPolar::AxisBottom, false);
    d_grid->showGrid(QwtPolar::Azimuth, true);
    d_grid->showGrid(QwtPolar::Radius, true);
    d_grid->attach(this);

    // curves

    for ( int curveId = 0; curveId < PlotSettings::NumCurves; curveId++ )
    {
        d_curve[curveId] = createCurve(curveId);
        d_curve[curveId]->attach(this);
    }

    QwtLegend *legend = new QwtLegend;
    insertLegend(legend,  QwtPolarPlot::BottomLegend);
}

PlotSettings Plot::settings() const
{
    PlotSettings s;
    for ( int scaleId = 0; scaleId < QwtPolar::ScaleCount; scaleId++ )
    {
        s.flags[PlotSettings::MajorGridBegin + scaleId] =
            d_grid->isGridVisible(scaleId);
        s.flags[PlotSettings::MinorGridBegin + scaleId] =
            d_grid->isMinorGridVisible(scaleId);
    }
    for ( int axisId = 0; axisId < QwtPolar::AxesCount; axisId++ )
    {
        s.flags[PlotSettings::AxisBegin + axisId] = 
            d_grid->isAxisVisible(axisId);
    }

    s.flags[PlotSettings::AutoScaling] = d_grid->hasAxisAutoScaling();

    const QwtScaleTransformation *transform =
        scaleEngine(QwtPolar::Radius)->transformation();
    s.flags[PlotSettings::Logarithmic] = 
        (transform->type() == QwtScaleTransformation::Log10);
    delete transform;

    const QwtScaleDiv *sd = scaleDiv(QwtPolar::Radius);
    s.flags[PlotSettings::Inverted] = sd->lBound() > sd->hBound();

#if QT_VERSION >= 0x040000
    s.flags[PlotSettings::Antialiasing] = d_grid->testRenderHint(
        QwtPolarItem::RenderAntialiased );
#endif

    for ( int curveId = 0; curveId < PlotSettings::NumCurves; curveId++ )
    {
        s.flags[PlotSettings::CurveBegin + curveId] =
            d_curve[curveId]->isVisible();
    }
    
    return s;
}

void Plot::applySettings(const PlotSettings& s)
{
    for ( int scaleId = 0; scaleId < QwtPolar::ScaleCount; scaleId++ )
    {
        d_grid->showGrid(scaleId, 
            s.flags[PlotSettings::MajorGridBegin + scaleId]);
        d_grid->showMinorGrid(scaleId, 
            s.flags[PlotSettings::MinorGridBegin + scaleId]);
    }

    for ( int axisId = 0; axisId < QwtPolar::AxesCount; axisId++ )
    {
        d_grid->showAxis(axisId, 
            s.flags[PlotSettings::AxisBegin + axisId]);
    }

    d_grid->setAxisAutoScaling(s.flags[PlotSettings::AutoScaling]);

    const QwtDoubleInterval interval = 
        scaleDiv(QwtPolar::Radius)->interval().normalized();
    if ( s.flags[PlotSettings::Inverted] )
    {
        setScale(QwtPolar::Radius,
            interval.maxValue(), interval.minValue());
    }
    else
    {
        setScale(QwtPolar::Radius,
            interval.minValue(), interval.maxValue());
    }

    const QwtScaleTransformation *transform = 
        scaleEngine(QwtPolar::Radius)->transformation();
    if ( s.flags[PlotSettings::Logarithmic] )
    {
        if ( transform->type() != QwtScaleTransformation::Log10 )
            setScaleEngine(QwtPolar::Radius, new QwtLog10ScaleEngine());
    }
    else
    {
        if ( transform->type() != QwtScaleTransformation::Linear )
            setScaleEngine(QwtPolar::Radius, new QwtLinearScaleEngine());
    }
    delete transform;

#if QT_VERSION >= 0x040000
    d_grid->setRenderHint( QwtPolarItem::RenderAntialiased, 
        s.flags[PlotSettings::Antialiasing] );
#endif

    for ( int curveId = 0; curveId < PlotSettings::NumCurves; curveId++ )
    {
#if QT_VERSION >= 0x040000
        d_curve[curveId]->setRenderHint(QwtPolarItem::RenderAntialiased,
            s.flags[PlotSettings::Antialiasing] );
#endif
        d_curve[curveId]->setVisible(
            s.flags[PlotSettings::CurveBegin + curveId]);
    }

    replot();
}

QwtPolarCurve *Plot::createCurve(int id) const
{
    const int numPoints = 200;

    QwtPolarCurve *curve = new QwtPolarCurve();
    curve->setStyle(QwtPolarCurve::Lines);
    switch(id)
    {
        case PlotSettings::Spiral:
        {
            curve->setTitle("Spiral");
            curve->setPen(QPen(Qt::yellow, 2));
            curve->setSymbol( QwtSymbol(QwtSymbol::Rect, 
                QBrush(Qt::yellow), QPen(Qt::white), QSize(3, 3)) );
            curve->setData(
                SpiralData(radialInterval, azimuthInterval, numPoints));
            break;
        }
        case PlotSettings::Rose:
        {
            curve->setTitle("Rose");
            curve->setPen(QPen(Qt::red, 2)); 
            curve->setSymbol( QwtSymbol(QwtSymbol::Rect,
                QBrush(Qt::cyan), QPen(Qt::white), QSize(3, 3)) );
            curve->setData(
                RoseData(radialInterval, azimuthInterval, numPoints));
            break;
        }
    }
    return curve;
}
