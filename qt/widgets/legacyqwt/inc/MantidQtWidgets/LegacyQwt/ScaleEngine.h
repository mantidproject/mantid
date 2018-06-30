/***************************************************************************
    File                 : ScaleEngine.h
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2007 by Ion Vasilief
    Email (use @ for *)  : ion_vasilief*yahoo.fr
    Description          : Extensions to QwtScaleEngine and
 QwtScaleTransformation

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
#ifndef SCALE_ENGINE_H
#define SCALE_ENGINE_H

#include "MantidQtWidgets/LegacyQwt/DllOption.h"
#include <float.h>
#include <qwt_scale_engine.h>
#include <qwt_scale_map.h>

class ScaleEngine;

class EXPORT_OPT_MANTIDQT_LEGACYQWT ScaleTransformation
    : public QwtScaleTransformation {
public:
  enum Type { Linear, Log10, Power };

  ScaleTransformation(const ScaleEngine *engine)
      : QwtScaleTransformation(Other), d_engine(engine){};
  double xForm(double x, double, double, double p1, double p2) const override;
  double invXForm(double x, double s1, double s2, double p1,
                  double p2) const override;
  QwtScaleTransformation *copy() const override;
  ~ScaleTransformation() override;

protected:
  QwtScaleTransformation *newScaleTransformation() const;
  //! The scale engine that generates the transformation
  const ScaleEngine *d_engine;
};

class EXPORT_OPT_MANTIDQT_LEGACYQWT ScaleEngine : public QwtScaleEngine {
public:
  ScaleEngine(ScaleTransformation::Type type = ScaleTransformation::Linear,
              double left_break = -DBL_MAX, double right_break = DBL_MAX);

  ~ScaleEngine() override;

  QwtScaleTransformation *transformation() const override;
  QwtScaleDiv divideScale(double x1, double x2, int maxMajSteps,
                          int maxMinSteps,
                          double stepSize = 0.0) const override;
  void autoScale(int maxNumSteps, double &x1, double &x2,
                 double &stepSize) const override;

  double axisBreakLeft() const;
  double axisBreakRight() const;
  void setBreakRegion(double from, double to) {
    d_break_left = from;
    d_break_right = to;
  };

  int breakWidth() const;
  void setBreakWidth(int width) { d_break_width = width; };

  int breakPosition() const;
  void setBreakPosition(int pos) { d_break_pos = pos; };

  double stepBeforeBreak() const;
  void setStepBeforeBreak(double step) { d_step_before = step; };

  double stepAfterBreak() const;
  void setStepAfterBreak(double step) { d_step_after = step; };

  int minTicksBeforeBreak() const;
  void setMinTicksBeforeBreak(int ticks) { d_minor_ticks_before = ticks; };

  int minTicksAfterBreak() const;
  void setMinTicksAfterBreak(int ticks) { d_minor_ticks_after = ticks; };

  bool log10ScaleAfterBreak() const;
  void setLog10ScaleAfterBreak(bool on) { d_log10_scale_after = on; };

  double nthPower() const;
  void setNthPower(double nth_power) { d_nth_power = nth_power; };

  ScaleTransformation::Type type() const;
  void setType(ScaleTransformation::Type type) { d_type = type; };

  bool hasBreak() const;
  void clone(const ScaleEngine *engine);

  bool hasBreakDecoration() const;
  void drawBreakDecoration(bool draw) { d_break_decoration = draw; };

private:
  QwtScaleEngine *newScaleEngine() const;

  ScaleTransformation::Type d_type;
  double d_break_left, d_break_right;
  //! Position of axis break (% of axis length)
  int d_break_pos;
  //! Scale increment before and after break
  double d_step_before, d_step_after;
  //! Minor ticks before and after break
  int d_minor_ticks_before, d_minor_ticks_after;
  //! Log10 scale after break
  bool d_log10_scale_after;
  //! Width of the axis break in pixels
  int d_break_width;
  //! If true draw the break decoration
  bool d_break_decoration;
  //! Nth Power for a power scale
  double d_nth_power;
};

#endif
