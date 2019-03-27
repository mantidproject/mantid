/***************************************************************************
    File                 : PowerScaleEngine.h
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2009 by Ion Vasilief
    Email (use @ for *)  : ion_vasilief*yahoo.fr
    Description          : Return a transformation for power (X^n) scales

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *  This program is free software; you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation; either version 2 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/

#ifndef POWER_SCALE_ENGINE_H
#define POWER_SCALE_ENGINE_H

#include "MantidQtWidgets/LegacyQwt/DllOption.h"
#include "ScaleEngine.h"
#include <qwt_scale_engine.h>
#include <qwt_scale_map.h>

class EXPORT_OPT_MANTIDQT_LEGACYQWT PowerScaleTransformation
    : public ScaleTransformation {
public:
  PowerScaleTransformation(const ScaleEngine *engine)
      : ScaleTransformation(engine), nth_power(engine->nthPower()){};
  double xForm(double x, double /*unused*/, double /*unused*/, double p1, double p2) const override;
  double invXForm(double x, double s1, double s2, double p1,
                  double p2) const override;
  QwtScaleTransformation *copy() const override;
  ~PowerScaleTransformation() override;

private:
  double nth_power;
};

/*!
  \brief A scale engine for power (X^n) scales
*/

class EXPORT_OPT_MANTIDQT_LEGACYQWT PowerScaleEngine : public QwtScaleEngine {
public:
  void autoScale(int maxSteps, double &x1, double &x2,
                 double &stepSize) const override;

  QwtScaleDiv divideScale(double x1, double x2, int numMajorSteps,
                          int numMinorSteps,
                          double stepSize = 0.0) const override;

  QwtScaleTransformation *transformation() const override;

  ~PowerScaleEngine() override;

protected:
  QwtDoubleInterval align(const QwtDoubleInterval & /*interval*/, double stepSize) const;

private:
  void buildTicks(const QwtDoubleInterval & /*interval*/, double stepSize, int maxMinSteps,
                  QwtValueList ticks[QwtScaleDiv::NTickTypes]) const;

  void buildMinorTicks(const QwtValueList &majorTicks, int maxMinMark,
                       double step, QwtValueList & /*minorTicks*/, QwtValueList & /*mediumTicks*/) const;

  QwtValueList buildMajorTicks(const QwtDoubleInterval &interval,
                               double stepSize) const;
};

#endif
