/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * QwtPolar Widget Library
 * Copyright (C) 2008   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GPL License, Version 2.0
 *****************************************************************************/

#ifndef QWT_POLAR_ITEM_H
#define QWT_POLAR_ITEM_H

#include "qwt_global.h"
#include "qwt_text.h"
#include "qwt_legend_itemmanager.h"
#include "qwt_double_interval.h"
#include "qwt_double_rect.h"

class QString;
class QRect;
class QPainter;
class QwtPolarPlot;
class QwtScaleMap;
class QwtScaleDiv;

/*!
  \brief Base class for items on a polar plot
*/

class QWT_EXPORT QwtPolarItem: public QwtLegendItemManager
{
public:
    enum RttiValues
    { 
        Rtti_PolarItem = 0,

        Rtti_PolarGrid,
        Rtti_PolarScale,
        Rtti_PolarMarker,
        Rtti_PolarCurve,

        Rtti_PolarUserItem = 1000
    };

    enum ItemAttribute
    {
        Legend = 1,
        AutoScale = 2
    };

#if QT_VERSION >= 0x040000
    enum RenderHint
    {
        RenderAntialiased = 1
    };
#endif

    explicit QwtPolarItem(const QwtText &title = QwtText());
    virtual ~QwtPolarItem();

    void attach(QwtPolarPlot *plot);

    /*!
       \brief This method detaches a QwtPolarItem from any QwtPolarPlot it 
              has been associated with.

       detach() is equivalent to calling attach( NULL )
       \sa attach( QwtPolarPlot* plot )
    */
    void detach() { attach(NULL); }

    QwtPolarPlot *plot() const;
    
    void setTitle(const QString &title);
    void setTitle(const QwtText &title);
    const QwtText &title() const;

    virtual int rtti() const;

    void setItemAttribute(ItemAttribute, bool on = true);
    bool testItemAttribute(ItemAttribute) const;

#if QT_VERSION >= 0x040000
    void setRenderHint(RenderHint, bool on = true);
    bool testRenderHint(RenderHint) const;
#endif

    double z() const; 
    void setZ(double z);

    void show();
    void hide();
    virtual void setVisible(bool);
    bool isVisible () const;

    virtual void itemChanged();

    /*!
      \brief Draw the item

      \param painter Painter
      \param radialMap Maps distance values into painter coordinates.
      \param azimuthMap Maps angle values into painter coordinates.
      \param pole Position of the pole in painter coordinates
      \param canvasRect Contents rect of the canvas in painter coordinates
    */
    virtual void draw(QPainter *painter, 
        const QwtScaleMap &azimuthMap, const QwtScaleMap &radialMap,
        const QwtDoublePoint &pole, double radius,
        const QwtDoubleRect &canvasRect) const = 0;

    virtual QwtDoubleInterval interval(int scaleId) const;

    virtual QWidget *legendItem() const;

    virtual void updateLegend(QwtLegend *) const;
    virtual void updateScaleDiv(const QwtScaleDiv&,
        const QwtScaleDiv&);

    virtual int canvasMarginHint() const;

private:
    // Disabled copy constructor and operator=
    QwtPolarItem( const QwtPolarItem & );
    QwtPolarItem &operator=( const QwtPolarItem & );

    class PrivateData;
    PrivateData *d_data;
};
            
#endif
