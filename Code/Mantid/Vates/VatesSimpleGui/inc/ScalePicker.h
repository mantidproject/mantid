/*
 * scalepicker.h
 *
 *  Created on: Apr 12, 2011
 *      Author: 2zr
 */

#ifndef SCALEPICKER_H_
#define SCALEPICKER_H_

#include <qobject.h>

class QEvent;
class QPoint;
class QRect;
class QwtScaleWidget;

class ScalePicker: public QObject {
	Q_OBJECT

public:
	ScalePicker(QwtScaleWidget *scale = 0);
	virtual ~ScalePicker() {}
    virtual bool eventFilter(QObject *, QEvent *);
    QPoint *getLocation(double axisval);

signals:
    void clicked(double value);
    void makeIndicator(const QPoint &point);

private:
    void mouseClicked(const QwtScaleWidget *, const QPoint &);
    QRect scaleRect(const QwtScaleWidget *) const;
};

#endif /* SCALEPICKER_H_ */
