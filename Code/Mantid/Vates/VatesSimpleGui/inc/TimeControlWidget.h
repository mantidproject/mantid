/* 
 * File:   TimeControlWidget.h
 * Author: 2zr
 *
 * Created on March 9, 2011, 2:15 PM
 */

#ifndef _TIMECONTROLWIDGET_H
#define	_TIMECONTROLWIDGET_H

#include "ui_timecontrolwidget.h"

class TimeControlWidget : public QWidget, public Ui::TimeControlWidget
{
    Q_OBJECT

public:
    TimeControlWidget(QWidget *parent = 0);
    virtual ~TimeControlWidget();
};

#endif	/* _TIMECONTROLWIDGET_H */
