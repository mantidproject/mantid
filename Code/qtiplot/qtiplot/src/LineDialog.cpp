/***************************************************************************
    File                 : LineDialog.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief
    Email (use @ for *)  : ion_vasilief*yahoo.fr
    Description          : Line options dialog

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
#include "LineDialog.h"
#include "ColorButton.h"
#include "ArrowMarker.h"
#include "Graph.h"
#include "Plot.h"
#include "ApplicationWindow.h"
#include "DoubleSpinBox.h"

#include <qwt_plot.h>

#include <QMessageBox>
#include <QLayout>
#include <QGroupBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QTabWidget>

LineDialog::LineDialog( ArrowMarker *line, QWidget* parent,  Qt::WFlags fl )
    : QDialog( parent, fl )
{
    setWindowTitle( tr( "MantidPlot - Line options" ) );
	setAttribute(Qt::WA_DeleteOnClose);
	
	lm = line;

	QGroupBox *gb1 = new QGroupBox();
    QGridLayout *gl1 = new QGridLayout();

    gl1->addWidget(new QLabel(tr("Color")), 0, 0);
	colorBox = new ColorButton();
	colorBox->setColor(lm->color());
	gl1->addWidget(colorBox, 0, 1);

	gl1->addWidget(new QLabel(tr("Line type")), 1, 0);
    styleBox = new QComboBox( FALSE);
	styleBox->insertItem("_____");
	styleBox->insertItem("- - -");
	styleBox->insertItem(".....");
	styleBox->insertItem("_._._");
	styleBox->insertItem("_.._..");
	gl1->addWidget(styleBox, 1, 1);

	setLineStyle(lm->style());

	gl1->addWidget(new QLabel(tr("Line width")), 2, 0);
    widthBox = new DoubleSpinBox('f');
	widthBox->setLocale(((ApplicationWindow *)parent)->locale());
	widthBox->setSingleStep(0.1);
    widthBox->setRange(0, 100);
	widthBox->setValue(lm->width());
	gl1->addWidget(widthBox, 2, 1);

	startBox = new QCheckBox();
    startBox->setText( tr( "Arrow at &start" ) );
	startBox->setChecked(lm->hasStartArrow());
	gl1->addWidget(startBox, 3, 0);

	endBox = new QCheckBox();
    endBox->setText( tr( "Arrow at &end" ) );
	endBox->setChecked(lm->hasEndArrow());
	gl1->addWidget(endBox, 3, 1);
	gl1->setRowStretch(4, 1);

	gb1->setLayout(gl1);

	QHBoxLayout* hl1 = new QHBoxLayout();
    hl1->addWidget(gb1);

	options = new QWidget();
    options->setLayout(hl1);

	tw = new QTabWidget();
	tw->addTab(options, tr( "Opti&ons" ) );

    QGroupBox *gb2 = new QGroupBox();
    QGridLayout *gl2 = new QGridLayout();

    gl2->addWidget(new QLabel(tr("Length")), 0, 0);
	boxHeadLength = new QSpinBox();
	boxHeadLength->setValue(lm->headLength());
	gl2->addWidget(boxHeadLength, 0, 1);

	gl2->addWidget(new QLabel(tr( "Angle" )), 1, 0 );
	boxHeadAngle = new QSpinBox();
	boxHeadAngle->setRange(0, 85);
	boxHeadAngle->setSingleStep(5);
	boxHeadAngle->setValue(lm->headAngle());
	gl2->addWidget(boxHeadAngle, 1, 1);

	filledBox = new QCheckBox();
    filledBox->setText( tr( "&Filled" ) );
	filledBox->setChecked(lm->filledArrowHead());
	gl2->addWidget(filledBox, 2, 1);
	gl2->setRowStretch(3, 1);

	gb2->setLayout(gl2);

	QHBoxLayout* hl2 = new QHBoxLayout();
    hl2->addWidget(gb2);

    head = new QWidget();
    head->setLayout(hl2);
	tw->addTab(head, tr("Arrow &Head"));

	initGeometryTab();

	buttonDefault = new QPushButton( tr( "Set &Default" ) );
	btnApply = new QPushButton( tr( "&Apply" ) );
	btnOk = new QPushButton(tr( "&Ok" ) );
    btnOk->setDefault(true);

    QBoxLayout *bl1 = new QBoxLayout (QBoxLayout::LeftToRight);
    bl1->addStretch();
	bl1->addWidget(buttonDefault);
	bl1->addWidget(btnApply);
	bl1->addWidget(btnOk);

	QVBoxLayout* vl = new QVBoxLayout();
    vl->addWidget(tw);
	vl->addLayout(bl1);
	setLayout(vl);

	enableHeadTab();

	connect( btnOk, SIGNAL( clicked() ), this, SLOT(accept() ) );
	connect( btnApply, SIGNAL( clicked() ), this, SLOT(apply() ) );
	connect( tw, SIGNAL(currentChanged (QWidget *)), this, SLOT(enableButtonDefault(QWidget *)));
	connect( buttonDefault, SIGNAL(clicked()), this, SLOT(setDefaultValues()));
}

void LineDialog::initGeometryTab()
{
	unitBox = new QComboBox();
	unitBox->insertItem(tr("Scale Coordinates"));
	unitBox->insertItem(tr("Pixels"));

	QBoxLayout *bl1 = new QBoxLayout (QBoxLayout::LeftToRight);
	bl1->addWidget(new QLabel(tr( "Unit" )));
	bl1->addWidget(unitBox);

	ApplicationWindow *app = (ApplicationWindow *)parent();
	QLocale locale = QLocale();
	if (app)
		locale = app->locale();
	
    QGroupBox *gb1 = new QGroupBox(tr("Start Point"));
	xStartBox = new DoubleSpinBox();
	xStartBox->setLocale(locale);
	xStartBox->setDecimals(6);
	yStartBox = new DoubleSpinBox();
	yStartBox->setLocale(locale);
	yStartBox->setDecimals(6);
	
	xStartPixelBox = new QSpinBox();
	xStartPixelBox->setRange(-INT_MAX, INT_MAX);
	yStartPixelBox = new QSpinBox();
	yStartPixelBox->setRange(-INT_MAX, INT_MAX);

    QGridLayout *gl1 = new QGridLayout();
    gl1->addWidget(new QLabel( tr("X")), 0, 0);
    gl1->addWidget(xStartBox, 0, 1);
	gl1->addWidget(xStartPixelBox, 0, 1);
    gl1->addWidget(new QLabel(tr("To")), 1, 0);
    gl1->addWidget(yStartBox, 1, 1);
	gl1->addWidget(yStartPixelBox, 1, 1);
	gl1->setColumnStretch(1, 10);
	gl1->setRowStretch(2, 1);
    gb1->setLayout(gl1);

    QGroupBox *gb2 = new QGroupBox(tr("End Point"));
    xEndBox = new DoubleSpinBox();
	xEndBox->setLocale(locale);
	xEndBox->setDecimals(6);
	yEndBox = new DoubleSpinBox();
	yEndBox->setLocale(locale);
	yEndBox->setDecimals(6);
	
	xEndPixelBox = new QSpinBox();
	xEndPixelBox->setRange(-INT_MAX, INT_MAX);
	yEndPixelBox = new QSpinBox();
	yEndPixelBox->setRange(-INT_MAX, INT_MAX);
	
    QGridLayout *gl2 = new QGridLayout();
    gl2->addWidget(new QLabel( tr("X")), 0, 0);
    gl2->addWidget(xEndBox, 0, 1);
	gl2->addWidget(xEndPixelBox, 0, 1);
	
    gl2->addWidget(new QLabel(tr("To")), 1, 0);
    gl2->addWidget(yEndBox, 1, 1);
	gl2->addWidget(yEndPixelBox, 1, 1);
	gl2->setColumnStretch(1, 10);
	gl2->setRowStretch(2, 1);
    gb2->setLayout(gl2);

    QBoxLayout *bl2 = new QBoxLayout (QBoxLayout::LeftToRight);
	bl2->addWidget(gb1);
	bl2->addWidget(gb2);

	QVBoxLayout* vl = new QVBoxLayout();
    vl->addLayout(bl1);
    vl->addLayout(bl2);

    geometry = new QWidget();
    geometry->setLayout(vl);
	tw->addTab(geometry, tr( "&Geometry" ) );

	connect(unitBox, SIGNAL(activated(int)), this, SLOT(displayCoordinates(int)));
	displayCoordinates(0);
}

void LineDialog::displayCoordinates(int unit)
{
	if (unit == ScaleCoordinates){
		QwtDoublePoint sp = lm->startPointCoord();
		xStartBox->setValue(sp.x());
		xStartBox->show();
		xStartPixelBox->hide();
		yStartBox->setValue(sp.y());
		yStartBox->show();
		yStartPixelBox->hide();

		QwtDoublePoint ep = lm->endPointCoord();
		xEndBox->setValue(ep.x());
		xEndBox->show();
		xEndPixelBox->hide();
		yEndBox->setValue(ep.y());
		yEndBox->show();
		yEndPixelBox->hide();
	} else {
		QPoint startPoint = lm->startPoint();
		QPoint endPoint = lm->endPoint();

		xStartBox->hide();	
		xStartPixelBox->setValue(startPoint.x());
		xStartPixelBox->show();
		
		yStartBox->hide();
		yStartPixelBox->setValue(startPoint.y());
		yStartPixelBox->show();
		
		xEndBox->hide();
		xEndPixelBox->setValue(endPoint.x());
		xEndPixelBox->show();
		
		yEndBox->hide();
		yEndPixelBox->setValue(endPoint.y());
		yEndPixelBox->show();
	}
}

void LineDialog::setCoordinates(int unit)
{
	if (unit == ScaleCoordinates){
		lm->setStartPoint(xStartBox->value(), yStartBox->value());
		lm->setEndPoint(xEndBox->value(), yEndBox->value());
	} else {
		lm->setStartPoint(QPoint(xStartPixelBox->value(), yStartPixelBox->value()));
		lm->setEndPoint(QPoint(xEndPixelBox->value(), yEndPixelBox->value()));
	}
}

void LineDialog::apply()
{
    if (tw->currentPage()==(QWidget *)options){
        lm->setStyle(Graph::getPenStyle(styleBox->currentItem()));
        lm->setColor(colorBox->color());
        lm->setWidth(widthBox->value());
        lm->drawEndArrow(endBox->isChecked());
        lm->drawStartArrow(startBox->isChecked());
	} else if (tw->currentPage()==(QWidget *)head){
        if (lm->headLength() != boxHeadLength->value())
            lm->setHeadLength( boxHeadLength->value() );

        if (lm->headAngle() != boxHeadAngle->value())
            lm->setHeadAngle( boxHeadAngle->value() );

        if (lm->filledArrowHead() != filledBox->isChecked())
            lm->fillArrowHead( filledBox->isChecked() );
	} else if (tw->currentPage()==(QWidget *)geometry)
        setCoordinates(unitBox->currentItem());

	QwtPlot *plot = lm->plot();
	Graph *g = (Graph *)plot->parent();
	plot->replot();
	g->notifyChanges();

	enableHeadTab();
}

void LineDialog::accept()
{
	apply();
	close();
}

void LineDialog::setLineStyle(Qt::PenStyle style)
{
if (style==Qt::SolidLine)
	styleBox->setCurrentItem(0);
else if (style==Qt::DashLine)
	styleBox->setCurrentItem(1);
else if (style==Qt::DotLine)
	styleBox->setCurrentItem(2);
else if (style==Qt::DashDotLine)
	styleBox->setCurrentItem(3);
else if (style==Qt::DashDotDotLine)
	styleBox->setCurrentItem(4);
}

void LineDialog::enableHeadTab()
{
if (startBox->isChecked() || endBox->isChecked())
	tw->setTabEnabled (head, true);
else
	tw->setTabEnabled (head, false);
}

void LineDialog::setDefaultValues()
{
ApplicationWindow *app = (ApplicationWindow *)this->parent();
if (!app)
	return;

app->setArrowDefaultSettings(widthBox->value(), colorBox->color(),
							Graph::getPenStyle(styleBox->currentItem()),
							boxHeadLength->value(), boxHeadAngle->value(), filledBox->isChecked());
}

void LineDialog::enableButtonDefault(QWidget *w)
{
if (w == geometry)
	buttonDefault->setEnabled(false);
else
	buttonDefault->setEnabled(true);
}
