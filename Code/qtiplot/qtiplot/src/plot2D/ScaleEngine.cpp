/***************************************************************************
   	File                 : ScaleEngine.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2007 by Ion Vasilief
    Email (use @ for *)  : ion_vasilief*yahoo.fr
    Description          : Extensions to QwtScaleEngine and QwtScaleTransformation

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
#include "qwt_compat.h"
#include "ScaleEngine.h"

QwtScaleTransformation* ScaleEngine::transformation() const
{
	return new ScaleTransformation(this);
}

double ScaleTransformation::invXForm(double p, double p1, double p2, double s1, double s2) const
{
    if (!d_engine->hasBreak()){
        QwtScaleTransformation *tr = new QwtScaleTransformation (d_engine->type());
        double res = tr->invXForm(p, p1, p2, s1, s2);
        delete tr;
        return res;
    }
	
    const int d_break_space = d_engine->breakWidth();
	const double lb = d_engine->axisBreakLeft();
    const double rb = d_engine->axisBreakRight();
	const double pm = p1 + (p2 - p1)*(double)d_engine->breakPosition()/100.0;
	double pml, pmr;
	if (p2 > p1){
		pml = pm - d_break_space;
		pmr = pm + d_break_space;
	} else {
		pml = pm + d_break_space;
		pmr = pm - d_break_space;
	}

	if (p > pml && p < pmr)
		return pm;
	
	bool invertedScale = d_engine->testAttribute(QwtScaleEngine::Inverted);
	QwtScaleTransformation::Type d_type = d_engine->type();

	if (invertedScale){
		if ((p2 > p1 && p <= pml) || (p2 < p1 && p >= pml)){
			if (d_engine->log10ScaleAfterBreak())
				return s1*exp((p - p1)/(pml - p1)*log(rb/s1));
            else 
				return s1 + (rb - s1)/(pml - p1)*(p - p1);
		}
		
		if ((p2 > p1 && p >= pmr) || (p2 < p1 && p <= pmr)){
			if (d_type == QwtScaleTransformation::Log10)
				return lb * exp((p - pmr)/(p2 - pmr)*log(s2/lb));
			else if (d_type == QwtScaleTransformation::Linear)
				return lb + (p - pmr)/(p2 - pmr)*(s2 - lb);
		}

	}

    if ((p2 > p1 && p <= pml) || (p2 < p1 && p >= pml)){
        if (d_type == QwtScaleTransformation::Linear)
            return s1 + (lb - s1)*(p - p1)/(pml - p1);
        else if (d_type == QwtScaleTransformation::Log10)
            return s1 * exp((p - p1)/(pml - p1)*log(lb/s1));
    }

	if ((p2 > p1 && p >= pmr) || (p2 < p1 && p <= pmr)){
	    if (d_engine->log10ScaleAfterBreak())
            return rb * exp((p - pmr)/(p2 - pmr)*log(s2/rb));
	    else
            return rb + (p - pmr)*(s2 - rb)/(p2 - pmr);
	}

	return DBL_MAX; // something invalid
}

double ScaleTransformation::xForm(double s, double s1, double s2, double p1, double p2) const
{
	if (!d_engine->hasBreak()){
		QwtScaleTransformation *tr = new QwtScaleTransformation (d_engine->type());
		double res = tr->xForm(s, s1, s2, p1, p2);
		delete tr;
		return res;
	}

    const int d_break_space = d_engine->breakWidth();
	const double lb = d_engine->axisBreakLeft();
    const double rb = d_engine->axisBreakRight();
	const double pm = p1 + (p2 - p1)*(double)d_engine->breakPosition()/100.0;
	double pml, pmr;
	if (p2 > p1){
		pml = pm - d_break_space;
		pmr = pm + d_break_space;
	} else {
		pml = pm + d_break_space;
		pmr = pm - d_break_space;
	}

	if (s > lb && s < rb)
		return pm;

	bool invertedScale = d_engine->testAttribute(QwtScaleEngine::Inverted);
	QwtScaleTransformation::Type d_type = d_engine->type();

	if (invertedScale){
		if (s <= lb){
			if (d_type == QwtScaleTransformation::Linear)
				return pmr + (lb - s)/(lb - s2)*(p2 - pmr);
			else if (d_type == QwtScaleTransformation::Log10){
				return pmr + log(lb/s)/log(lb/s2)*(p2 - pmr);
			}
		}
		
		if (s >= rb){
			if (d_engine->log10ScaleAfterBreak())
				return p1 + log(s1/s)/log(s1/rb)*(pml - p1);
			else
				return p1 + (s1 - s)/(s1 - rb)*(pml - p1);
		}

	}

    if (s <= lb){
        if (d_type == QwtScaleTransformation::Linear)
            return p1 + (s - s1)/(lb - s1)*(pml - p1);
        else if (d_type == QwtScaleTransformation::Log10)
            return p1 + log(s/s1)/log(lb/s1)*(pml - p1);
    }

	if (s >= rb){
	    if (d_engine->log10ScaleAfterBreak())
            return pmr + log(s/rb)/log(s2/rb)*(p2 - pmr);
	    else
            return pmr + (s - rb)/(s2 - rb)*(p2 - pmr);
	}

	return DBL_MAX; // something invalid
}

QwtScaleTransformation *ScaleTransformation::copy() const
{
    return new ScaleTransformation(d_engine);
}

/*****************************************************************************
 *
 * Class ScaleEngine
 *
 *****************************************************************************/

ScaleEngine::ScaleEngine(QwtScaleTransformation::Type type,double left_break, double right_break): QwtScaleEngine(),
d_type(type),
d_break_left(left_break),
d_break_right(right_break),
d_break_pos(50),
d_step_before(0.0),
d_step_after(0.0),
d_minor_ticks_before(1),
d_minor_ticks_after(1),
d_log10_scale_after(false),
d_break_width(4),
d_break_decoration(true)
{}

bool ScaleEngine::hasBreak() const
{
return (d_break_left == d_break_right || (d_break_left == -DBL_MAX && d_break_right == DBL_MAX))?false:true;
}

double ScaleEngine::axisBreakLeft() const
{
    return d_break_left;
}

double ScaleEngine::axisBreakRight() const
{
    return d_break_right;
}

int ScaleEngine::breakWidth() const
{
    return d_break_width;
}

int ScaleEngine::breakPosition() const
{
    return d_break_pos;
}

double ScaleEngine::stepBeforeBreak() const
{
    return d_step_before;
}

double ScaleEngine::stepAfterBreak() const
{
    return d_step_after;
}

QwtScaleTransformation::Type ScaleEngine::type() const
{
    return d_type;
}

int ScaleEngine::minTicksBeforeBreak() const
{
    return d_minor_ticks_before;
}

int ScaleEngine::minTicksAfterBreak() const
{
    return d_minor_ticks_after;
}

bool ScaleEngine::log10ScaleAfterBreak() const
{
    return d_log10_scale_after;
}

bool ScaleEngine::hasBreakDecoration() const
{
	return d_break_decoration;
}

void ScaleEngine::clone(const ScaleEngine *engine)
{
    d_type = engine->type();
	d_break_left = engine->axisBreakLeft();
	d_break_right = engine->axisBreakRight();
    d_break_pos = engine->breakPosition();
	d_step_before = engine->stepBeforeBreak();
	d_step_after = engine->stepAfterBreak();
	d_minor_ticks_before = engine->minTicksBeforeBreak();
	d_minor_ticks_after = engine->minTicksAfterBreak();
    d_log10_scale_after = engine->log10ScaleAfterBreak();
    d_break_width = engine->breakWidth();
	d_break_decoration = engine->hasBreakDecoration();
	setAttributes(engine->attributes());
    setMargins(engine->loMargin(), engine->hiMargin());
}

QwtScaleDiv ScaleEngine::divideScale(double x1, double x2, int maxMajSteps,
		int maxMinSteps, double stepSize) const
{
	QwtScaleEngine *engine;
	if (!hasBreak()){
        if (d_type == QwtScaleTransformation::Log10)
            engine = new QwtLog10ScaleEngine();
        else
            engine = new QwtLinearScaleEngine();
		QwtScaleDiv div = engine->divideScale(x1, x2, maxMajSteps, maxMinSteps, stepSize);
		delete engine;
		return div;
	}

    double lb = d_break_left;
    double rb = d_break_right;
	double step1 = d_step_before;
	double step2 = d_step_after;
    if (x1 > x2){
        lb = d_break_right;
        rb = d_break_left;
		step1 = d_step_after;
		step2 = d_step_before;
        if (d_log10_scale_after)
            engine = new QwtLog10ScaleEngine();
        else
            engine = new QwtLinearScaleEngine();
    } else if (d_type == QwtScaleTransformation::Log10)
        engine = new QwtLog10ScaleEngine();
      else
        engine = new QwtLinearScaleEngine();

    int max_min_intervals = d_minor_ticks_before;
	if (d_minor_ticks_before == 1)
		max_min_intervals = 3;
	if (d_minor_ticks_before > 1)
		max_min_intervals = d_minor_ticks_before + 1;

	QwtScaleDiv div1 = engine->divideScale(x1, lb, maxMajSteps/2, max_min_intervals, step1);

    max_min_intervals = d_minor_ticks_after;
	if (d_minor_ticks_after == 1)
		max_min_intervals = 3;
	if (d_minor_ticks_after > 1)
		max_min_intervals = d_minor_ticks_after + 1;

    delete engine;
    if (testAttribute(QwtScaleEngine::Inverted)){
        if (d_type == QwtScaleTransformation::Log10)
            engine = new QwtLog10ScaleEngine();
        else
            engine = new QwtLinearScaleEngine();
    } else if (d_log10_scale_after)
            engine = new QwtLog10ScaleEngine();
      else
            engine = new QwtLinearScaleEngine();

    QwtScaleDiv div2 = engine->divideScale(rb, x2, maxMajSteps/2, max_min_intervals, step2);

    QwtValueList ticks[QwtScaleDiv::NTickTypes];
    ticks[QwtScaleDiv::MinorTick] = div1.ticks(QwtScaleDiv::MinorTick) + div2.ticks(QwtScaleDiv::MinorTick);
    ticks[QwtScaleDiv::MediumTick] = div1.ticks(QwtScaleDiv::MediumTick) + div2.ticks(QwtScaleDiv::MediumTick);
    ticks[QwtScaleDiv::MajorTick] = div1.ticks(QwtScaleDiv::MajorTick) + div2.ticks(QwtScaleDiv::MajorTick);

	delete engine;
	return QwtScaleDiv(x1, x2, ticks);
}

void ScaleEngine::autoScale (int maxNumSteps, double &x1, double &x2, double &stepSize) const
{
	if (!hasBreak() || testAttribute(QwtScaleEngine::Inverted)){
		QwtScaleEngine *engine;
		if (d_type == QwtScaleTransformation::Log10)
			engine = new QwtLog10ScaleEngine();
		else
			engine = new QwtLinearScaleEngine();
		
		engine->setAttributes(attributes());
    	engine->setReference(reference());
    	engine->setMargins(loMargin(), hiMargin());
		engine->autoScale(maxNumSteps, x1, x2, stepSize);
		delete engine;
	} else {
		QwtScaleEngine *engine;
		if (d_type == QwtScaleTransformation::Log10)
			engine = new QwtLog10ScaleEngine();
		else
			engine = new QwtLinearScaleEngine();
		
		engine->setAttributes(attributes());
		double breakLeft = d_break_left;
		engine->autoScale(maxNumSteps, x1, breakLeft, stepSize);
		delete engine;
		
		engine = new QwtLinearScaleEngine();
		engine->setAttributes(attributes());
		double breakRight = d_break_right;
		engine->autoScale(maxNumSteps, breakRight, x2, stepSize);
		delete engine;
	}
}
