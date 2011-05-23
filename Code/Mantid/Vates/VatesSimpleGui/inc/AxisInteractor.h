/*
 * axisinteractor.h
 *
 *  Created on: Apr 7, 2011
 *      Author: 2zr
 */

#ifndef AXISINTERACTOR_H_
#define AXISINTERACTOR_H_

#include <QtGui/QWidget>
#include "ui_axisinteractor.h"

class QGraphicsScene;
class QMouseEvent;
class QString;
class QwtScaleEngine;
class QwtScaleTransformation;
class ScalePicker;

class AxisInteractor : public QWidget
{
	Q_OBJECT

public:
	AxisInteractor(QWidget *parent = 0);
	virtual ~AxisInteractor() {}
	ScalePicker *getScalePicker() { return this->scalePicker; }
	void clearSelections();
	bool hasIndicator();
	void setInformation(QString title, double min, double max);
	void selectIndicator(const QString &name);
	void updateIndicator(double value);

protected slots:
	void createIndicator(const QPoint &point);
	void setIndicatorName(const QString &name);

protected:
	bool eventFilter(QObject *obj, QEvent *event);

private:
	QwtScaleEngine *engine;
	bool isSceneGeomInit;
	ScalePicker *scalePicker;
	QGraphicsScene *scene;
	QwtScaleTransformation *transform;
	Ui::AxisInteractor ui;
};

#endif /* AXISINTERACTOR_H_ */
