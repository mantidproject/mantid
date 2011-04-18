/***************************************************************************
    File                 : Plot3DDialog.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2004-2007 by Ion Vasilief
    Email (use @ for *)  : ion_vasilief*yahoo.fr
    Description          : Surface plot options dialog

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
#include "Plot3DDialog.h"
#include "TextDialog.h"
#include "MyParser.h"
#include "SymbolDialog.h"
#include "ApplicationWindow.h"
#include "ColorButton.h"
#include "TextFormatButtons.h"

#include <QListWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QLayout>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QStackedWidget>
#include <QWidget>
#include <QMessageBox>
#include <QComboBox>
#include <QWidgetList>
#include <QFileDialog>
#include <QGroupBox>
#include <QFontDialog>
#include <QApplication>

#include <qwt3d_color.h>

Plot3DDialog::Plot3DDialog( QWidget* parent,  Qt::WFlags fl )
    : QDialog( parent, fl )
{
    setName( "Plot3DDialog" );
	setWindowTitle( tr( "MantidPlot - Surface Plot Options" ) );

	bars=0; points=0;

    QHBoxLayout *hbox = new QHBoxLayout();
    hbox->addStretch();
	btnTable = new QPushButton();
    hbox->addWidget(btnTable);
	buttonApply = new QPushButton(tr( "&Apply" ));
    hbox->addWidget(buttonApply);
	buttonOk = new QPushButton(tr( "&OK" ));
	buttonOk->setDefault( true );
    hbox->addWidget(buttonOk);
	buttonCancel = new QPushButton(tr( "&Cancel" ));
    hbox->addWidget(buttonCancel);

    generalDialog = new QTabWidget();

    initScalesPage();
	initAxesPage();
	initTitlePage();
	initColorsPage();
	initGeneralPage();

	QVBoxLayout* vl = new QVBoxLayout(this);
	vl->addWidget(generalDialog);
	vl->addLayout(hbox);

	connect( buttonOk, SIGNAL( clicked() ), this, SLOT( accept() ) );
	connect( buttonCancel, SIGNAL( clicked() ), this, SLOT( reject() ) );
	connect( buttonApply, SIGNAL( clicked() ), this, SLOT(updatePlot() ) );
	connect( btnTable, SIGNAL( clicked() ), this, SLOT(worksheet() ) );
}

void Plot3DDialog::initScalesPage()
{
	axesList = new QListWidget();
	axesList->addItem(tr( "X" ) );
	axesList->addItem(tr( "Y" ) );
	axesList->addItem(tr( "Z" ) );
	axesList->setFixedWidth(50);
	axesList->setCurrentRow(0);

    QGridLayout *gl1 = new QGridLayout();
    gl1->addWidget(new QLabel(tr("From")), 0, 0);
	boxFrom = new QLineEdit();
	boxFrom->setMaximumWidth(150);
    gl1->addWidget(boxFrom, 0, 1);
    gl1->addWidget(new QLabel(tr("To")), 1, 0);
	boxTo = new QLineEdit();
	boxTo->setMaximumWidth(150);
    gl1->addWidget(boxTo, 1, 1);
    gl1->addWidget(new QLabel(tr("Type")), 2, 0);
	boxType=new QComboBox();
	boxType->addItem(tr("linear"));
	boxType->addItem(tr("logarithmic"));
	boxType->setMaximumWidth(150);
    gl1->addWidget(boxType, 2, 1);
    gl1->setRowStretch(3, 1);

    QGroupBox *gb1 = new QGroupBox();
    gb1->setLayout(gl1);

    QGridLayout *gl2 = new QGridLayout();
    gl2->addWidget(new QLabel(tr("Major Ticks")), 0, 0);
	boxMajors = new QSpinBox();
    gl2->addWidget(boxMajors, 0, 1);
    gl2->addWidget(new QLabel(tr("Minor Ticks")), 1, 0);
	boxMinors = new QSpinBox();
    gl2->addWidget(boxMinors, 1, 1);
    gl2->setRowStretch(2, 1);

    TicksGroupBox = new QGroupBox();
    TicksGroupBox->setLayout(gl2);

	QHBoxLayout* hb = new QHBoxLayout();
	hb->addWidget(axesList);
	hb->addWidget(gb1);
    hb->addWidget(TicksGroupBox);

    scale = new QWidget();
    scale->setLayout(hb);
	generalDialog->insertTab(scale, tr( "&Scale" ) );
}

void Plot3DDialog::initAxesPage()
{
	axesList2 = new QListWidget();
	axesList2->addItem(tr( "X" ) );
	axesList2->addItem(tr( "Y" ) );
	axesList2->addItem(tr( "Z" ) );
	axesList2->setFixedWidth(50);
	axesList2->setCurrentRow(0);

    QGridLayout *gl1 = new QGridLayout();
    gl1->addWidget(new QLabel(tr("Title")), 0, 0);
	boxLabel = new QTextEdit();
	boxLabel->setMaximumHeight(60);
    gl1->addWidget(boxLabel, 0, 1);
    gl1->addWidget(new QLabel(tr("Axis Font")), 1, 0);

    QHBoxLayout* hb1 = new QHBoxLayout();
	btnLabelFont = new QPushButton(tr( "&Choose font" ));
    hb1->addWidget(btnLabelFont);
	
	axisTitleFormatButtons = new TextFormatButtons(boxLabel);
	axisTitleFormatButtons->toggleCurveButton(false);
	axisTitleFormatButtons->toggleFontButtons(false);
	hb1->addWidget(axisTitleFormatButtons);

    hb1->addStretch();
    gl1->addLayout(hb1, 1, 1);

    gl1->addWidget(new QLabel(tr("Major Ticks Length")), 2, 0);
	boxMajorLength = new QLineEdit();
    gl1->addWidget(boxMajorLength, 2, 1);
    gl1->addWidget(new QLabel(tr("Minor Ticks Length")), 3, 0);
	boxMinorLength = new QLineEdit();
    gl1->addWidget(boxMinorLength, 3, 1);
    gl1->setRowStretch(4, 1);

    QGroupBox *gb1 = new QGroupBox();
    gb1->setLayout(gl1);

	QHBoxLayout* hb2 = new QHBoxLayout();
	hb2->addWidget(axesList2);
	hb2->addWidget(gb1);

    axes = new QWidget();
    axes->setLayout(hb2);
	generalDialog->insertTab(axes, tr( "&Axis" ) );

	connect( axesList2, SIGNAL(currentRowChanged(int)), this, SLOT(viewAxisOptions(int)));
	connect( axesList, SIGNAL(currentRowChanged(int)), this, SLOT(viewScaleLimits(int)));
	connect( btnLabelFont, SIGNAL(clicked()), this, SLOT(pickAxisLabelFont()));
}

void Plot3DDialog::initTitlePage()
{
    QHBoxLayout* hb1 = new QHBoxLayout();
    hb1->addStretch();
    QLabel *colorLabel = new QLabel(tr( "Co&lor" ));
    hb1->addWidget(colorLabel);
    btnTitleColor = new ColorButton();
    hb1->addWidget(btnTitleColor);
    colorLabel->setBuddy(btnTitleColor);

    btnTitleFont = new QPushButton(tr( "&Font" ));
    hb1->addWidget(btnTitleFont);
	
	QVBoxLayout* vl = new QVBoxLayout();
	boxTitle = new QTextEdit();
	boxTitle->setMaximumHeight(80);
	vl->addWidget(boxTitle);
	
	titleFormatButtons = new TextFormatButtons(boxTitle);
	titleFormatButtons->toggleCurveButton(false);
	titleFormatButtons->toggleFontButtons(false);
	hb1->addWidget(titleFormatButtons);
    hb1->addStretch();

    vl->addLayout(hb1);
    vl->addStretch();

    title = new QWidget();
    title->setLayout(vl);
	generalDialog->insertTab(title, tr( "&Title" ) );

	connect( btnTitleFont, SIGNAL(clicked()), this, SLOT(pickTitleFont() ) );
}

void Plot3DDialog::initColorsPage()
{
    QGridLayout* vl1 = new QGridLayout();
    btnFromColor = new ColorButton();
    QLabel *maxLabel = new QLabel(tr( "&Max" ));
    maxLabel->setBuddy(btnFromColor);
    vl1->addWidget(maxLabel, 0, 0);
    vl1->addWidget(btnFromColor, 0, 1);

	btnToColor = new ColorButton();
	QLabel *minLabel = new QLabel(tr( "M&in" ));
	minLabel->setBuddy(btnToColor);
    vl1->addWidget(minLabel, 1, 0);
    vl1->addWidget(btnToColor, 1, 1);

	btnColorMap = new QPushButton(tr( "Color Ma&p" ));
    vl1->addWidget(btnColorMap, 2, 1);
    vl1->setRowStretch(3, 1);

    QGroupBox *gb1 = new QGroupBox(tr( "Data" ));
    gb1->setLayout(vl1);

    QGridLayout* vl2 = new QGridLayout();
    btnMesh = new ColorButton();
    QLabel *meshLabel = new QLabel(tr( "&Line" ));
    meshLabel->setBuddy(btnMesh);
    vl2->addWidget(meshLabel, 0, 0);
    vl2->addWidget(btnMesh, 0, 1);

    btnBackground = new ColorButton();
    QLabel *backgroundLabel = new QLabel(tr( "&Background" ));
    backgroundLabel->setBuddy(btnBackground);
    vl2->addWidget(backgroundLabel, 1, 0);
    vl2->addWidget(btnBackground, 1, 1);
    vl2->setRowStretch(2, 1);

    QGroupBox *gb2 = new QGroupBox(tr( "General" ));
    gb2->setLayout(vl2);

    QGridLayout *gl1 = new QGridLayout();
    btnAxes = new ColorButton();
    QLabel *axesLabel = new QLabel(tr( "A&xes" ));
    axesLabel->setBuddy(btnAxes);
    gl1->addWidget(axesLabel, 0, 0);
    gl1->addWidget(btnAxes, 0, 1);

    btnLabels = new ColorButton();
    QLabel *labLabels = new QLabel(tr( "Lab&els" ));
    labLabels->setBuddy(btnLabels);
    gl1->addWidget(labLabels, 1, 0);
    gl1->addWidget(btnLabels, 1, 1);

	btnNumbers = new ColorButton();
	QLabel *numbersLabel = new QLabel(tr( "&Numbers" ));
    numbersLabel->setBuddy(btnNumbers);
	gl1->addWidget(numbersLabel, 2, 0);
    gl1->addWidget(btnNumbers, 2, 1);

	btnGrid = new ColorButton();
	QLabel *gridLabel = new QLabel(tr( "&Grid" ));
	gridLabel->setBuddy(btnGrid);
	gl1->addWidget(gridLabel, 3, 0);
    gl1->addWidget(btnGrid, 3, 1);
    gl1->setRowStretch(4, 1);

    AxesColorGroupBox = new QGroupBox(tr( "Coordinate System" ));
    AxesColorGroupBox->setLayout(gl1);

    QHBoxLayout* hb1 = new QHBoxLayout();
	hb1->addWidget(gb1);
    hb1->addWidget(gb2);
    hb1->addWidget(AxesColorGroupBox);

    QHBoxLayout* hb2 = new QHBoxLayout();
    hb2->addStretch();
	hb2->addWidget(new QLabel( tr( "Opacity" )));
	boxTransparency = new QSpinBox();
    boxTransparency->setRange(0, 100);
    boxTransparency->setSingleStep(5);
    hb2->addWidget(boxTransparency);

	QVBoxLayout* vl = new QVBoxLayout();
	vl->addLayout(hb2);
	vl->addLayout(hb1);

    colors = new QWidget();
    colors->setLayout(vl);
	generalDialog->insertTab(colors, tr( "&Colors" ) );

	connect( btnColorMap, SIGNAL( clicked() ), this, SLOT(pickDataColorMap() ) );
	connect( boxTransparency, SIGNAL( valueChanged(int) ), this, SLOT(changeTransparency(int) ) );
}

void Plot3DDialog::initGeneralPage()
{
    QGridLayout *gl1 = new QGridLayout();
	boxLegend = new QCheckBox(tr("Show Legend"));
    gl1->addWidget(boxLegend, 0, 0);
	boxOrthogonal = new QCheckBox(tr("Orthogonal"));
    gl1->addWidget(boxOrthogonal, 0, 1);

    gl1->addWidget(new QLabel(tr( "Line Width" )), 1, 0);
	boxMeshLineWidth = new QDoubleSpinBox();
	boxMeshLineWidth->setDecimals(1);
	boxMeshLineWidth->setSingleStep(0.1);
    boxMeshLineWidth->setRange(1, 100);
    gl1->addWidget(boxMeshLineWidth, 1, 1);

    gl1->addWidget(new QLabel( tr( "Resolution" )), 2, 0);
	boxResolution = new QSpinBox();
    boxResolution->setRange(1, 100);
	boxResolution->setSpecialValueText( "1 (all data)" );
    gl1->addWidget(boxResolution, 2, 1);

    gl1->addWidget(new QLabel( tr( "Numbers Font" )), 3, 0);
	btnNumbersFont=new QPushButton(tr( "&Choose Font" ));
    gl1->addWidget(btnNumbersFont, 3, 1);

    gl1->addWidget(new QLabel( tr( "Distance labels - axis" )), 4, 0);
	boxDistance = new QSpinBox();
    boxDistance->setRange(0, 1000);
    boxDistance->setSingleStep(5);
    gl1->addWidget(boxDistance, 4, 1);
    gl1->setRowStretch(5, 1);

    QGroupBox *gb1 = new QGroupBox();
    gb1->setLayout(gl1);

    QGridLayout *gl2 = new QGridLayout();
    gl2->addWidget(new QLabel(tr( "Zoom (%)" )), 0, 0);
	boxZoom = new QSpinBox();
    boxZoom->setRange(1, 10000);
    boxZoom->setSingleStep(10);

    gl2->addWidget(boxZoom, 0, 1);
    gl2->addWidget(new QLabel(tr( "X Zoom (%)" )), 1, 0);
	boxXScale = new QSpinBox();
    boxXScale->setRange(1, 10000);
    boxXScale->setSingleStep(10);
    gl2->addWidget(boxXScale, 1, 1);

    gl2->addWidget(new QLabel(tr( "Y Zoom (%)" )), 2, 0);
	boxYScale = new QSpinBox();
    boxYScale->setRange(1, 10000);
    boxYScale->setSingleStep(10);
    gl2->addWidget(boxYScale, 2, 1);

    gl2->addWidget(new QLabel(tr( "Z Zoom (%)" )), 3, 0);
	boxZScale = new QSpinBox();
    boxZScale->setRange(1, 10000);
    boxZScale->setSingleStep(10);
    gl2->addWidget(boxZScale, 3, 1);
    gl2->setRowStretch(4, 1);

    QGroupBox *gb2 = new QGroupBox();
    gb2->setLayout(gl2);

	QHBoxLayout* hl = new QHBoxLayout();
	hl->addWidget(gb1);
	hl->addWidget(gb2);

    general = new QWidget();
    general->setLayout(hl);
	generalDialog->insertTab(general, tr("&General"));

	connect( boxZoom, SIGNAL(valueChanged(int)), this, SLOT(changeZoom(int)));
	connect( boxXScale, SIGNAL(valueChanged(int)), this, SLOT(changeZoom(int)));
	connect( boxYScale, SIGNAL(valueChanged(int)), this, SLOT(changeZoom(int)));
	connect( boxZScale, SIGNAL(valueChanged(int)), this, SLOT(changeZoom(int)));
	connect( btnNumbersFont, SIGNAL(clicked()), this, SLOT(pickNumbersFont()));
}

void Plot3DDialog::initPointsOptionsStack()
{
    QHBoxLayout* hl1 = new QHBoxLayout();
    hl1->addStretch();
	hl1->addWidget(new QLabel( tr( "Style" )));
	boxPointStyle = new QComboBox();
	boxPointStyle->addItem(tr("Dot"));
	boxPointStyle->addItem(tr("Cross Hair"));
	boxPointStyle->addItem(tr("Cone"));
    hl1->addWidget(boxPointStyle);

	optionStack = new QStackedWidget();
	optionStack->setFrameShape( QFrame::StyledPanel );
	optionStack->setFrameShadow( QStackedWidget::Plain );

    QGridLayout *gl1 = new QGridLayout();
    gl1->addWidget(new QLabel( tr( "Width" )), 0, 0);
	boxSize = new QLineEdit("5");
    gl1->addWidget(boxSize, 0, 1);

	gl1->addWidget(new QLabel( tr( "Smooth angles" )), 1, 0);
	boxSmooth = new QCheckBox();
	boxSmooth->setChecked(false);
    gl1->addWidget(boxSmooth, 1, 1);

    dotsPage = new QWidget();
    dotsPage->setLayout(gl1);
	optionStack->addWidget(dotsPage);

    QGridLayout *gl2 = new QGridLayout();
    gl2->addWidget(new QLabel( tr( "Radius" )), 0, 0);
	boxCrossRad = new QLineEdit("0.01");
    gl2->addWidget(boxCrossRad, 0, 1);
	gl2->addWidget(new QLabel(tr( "Line Width")), 1, 0);
	boxCrossLinewidth = new QLineEdit("1");
    gl2->addWidget(boxCrossLinewidth, 1, 1);
	gl2->addWidget(new QLabel(tr( "Smooth line" )), 2, 0);
	boxCrossSmooth = new QCheckBox();
    boxCrossSmooth->setChecked(true);
    gl2->addWidget(boxCrossSmooth, 2, 1);
	gl2->addWidget(new QLabel(tr( "Boxed" )), 3, 0);
	boxBoxed = new QCheckBox();
	boxBoxed->setChecked(false);
    gl2->addWidget(boxBoxed, 3, 1);

	crossPage = new QWidget();
    crossPage->setLayout(gl2);
	optionStack->addWidget(crossPage);

    QGridLayout *gl3 = new QGridLayout();
    gl3->addWidget(new QLabel( tr( "Width" )), 0, 0);
	boxConesRad = new QLineEdit("0.5");
    gl3->addWidget(boxConesRad, 0, 1);
    gl3->addWidget(new QLabel( tr( "Quality" )), 1, 0);
	boxQuality = new QSpinBox();
    boxQuality->setRange(0, 40);
	boxQuality->setValue(32);
    gl3->addWidget(boxQuality, 1, 1);

    conesPage = new QWidget();
    conesPage->setLayout(gl3);
	optionStack->addWidget(conesPage);

	QVBoxLayout* vl = new QVBoxLayout();
    vl->addLayout(hl1);
    vl->addWidget(optionStack);

    points = new QWidget();
    points->setLayout(vl);

	generalDialog->insertTab(points, tr( "Points" ),4 );
	connect( boxPointStyle, SIGNAL( activated(int) ), optionStack, SLOT( setCurrentIndex(int) ) );
}

void Plot3DDialog::setPlot(Graph3D *g)
{
	if (!g)
		return;

	d_plot = g;

	btnFromColor->setColor(g->minDataColor());
	btnToColor->setColor(g->maxDataColor());
	btnTitleColor->setColor(g->titleColor());
	btnMesh->setColor(g->meshColor());
	btnAxes->setColor(g->axesColor());
	btnNumbers->setColor(g->numColor());
	btnLabels->setColor(g->labelColor());
	btnBackground->setColor(g->bgColor());
	btnGrid->setColor(g->gridColor());

	boxMeshLineWidth->setValue(g->meshLineWidth());
	boxTransparency->setValue(static_cast<int>(100*g->transparency()));

	boxTitle->setText(g->plotTitle());
	titleFont = g->titleFont();

    xScale = d_plot->xScale();
    yScale = d_plot->yScale();
    zScale = d_plot->zScale();
    zoom   = d_plot->zoom();

	boxZoom->setValue(100);
	boxXScale->setValue(100);
	boxYScale->setValue(100);
	boxZScale->setValue(100);

	boxResolution->setValue(g->resolution());
	boxLegend->setChecked(g->isLegendOn());
	boxOrthogonal->setChecked(g->isOrthogonal());

	labels = g->axesLabels();
	boxLabel->setText(labels[0]);

	tickLengths = g->axisTickLengths();
	boxMajorLength->setText(tickLengths[0]);
	boxMinorLength->setText(tickLengths[1]);

	xAxisFont = g->xAxisLabelFont();
	yAxisFont = g->yAxisLabelFont();
	zAxisFont = g->zAxisLabelFont();

	scales = g->scaleLimits();
	boxFrom->setText(scales[0]);
	boxTo->setText(scales[1]);
	boxMajors->setValue(scales[2].toInt());
	boxMinors->setValue(scales[3].toInt());
	boxType->setCurrentIndex(scales[4].toInt());

	boxDistance->setValue(g->labelsDistance());
	numbersFont = g->numbersFont();

	if (g->coordStyle() == Qwt3D::NOCOORD){
		TicksGroupBox->setDisabled(true);
		generalDialog->setTabEnabled(axes,false);
		AxesColorGroupBox->setDisabled(true);
		boxDistance->setDisabled(true);
		btnNumbersFont->setDisabled(true);
	}

	Qwt3D::PLOTSTYLE style = g->plotStyle();
	Graph3D::PointStyle pt = g->pointType();

	if ( style == Qwt3D::USER ){
			switch (pt)
			{
				case Graph3D::None :
					break;

				case Graph3D::Dots :
					disableMeshOptions();
					initPointsOptionsStack();
					showPointsTab (g->pointsSize(), g->smoothPoints());
					break;

				case Graph3D::VerticalBars :
					showBarsTab(g->barsRadius());
					break;

				case Graph3D::HairCross :
					disableMeshOptions();
					initPointsOptionsStack();
					showCrossHairTab (g->crossHairRadius(), g->crossHairLinewidth(),
							g->smoothCrossHair(), g->boxedCrossHair());
					break;

				case Graph3D::Cones :
					disableMeshOptions();
					initPointsOptionsStack();
					showConesTab(g->coneRadius(), g->coneQuality());
					break;
			}
		}
		else if ( style == Qwt3D::FILLED )
			disableMeshOptions();
		else if (style == Qwt3D::HIDDENLINE || style == Qwt3D::WIREFRAME)
			boxLegend->setDisabled(true);

		if (g->grids() == 0)
			btnGrid->setDisabled(true);

		if (g->userFunction() || g->parametricSurface())
			btnTable->hide();
		else if (g->table())
            btnTable->setText(tr("&Worksheet"));
		else if (g->matrix())
			btnTable->setText(tr("&Matrix"));

	connect( boxMeshLineWidth, SIGNAL(valueChanged(double)), d_plot, SLOT(setMeshLineWidth(double)));
	connect( boxOrthogonal, SIGNAL(toggled(bool)), d_plot, SLOT(setOrthogonal(bool)));
	connect( boxLegend, SIGNAL(toggled(bool)), d_plot, SLOT(showColorLegend(bool)));
    connect( boxResolution, SIGNAL(valueChanged(int)), d_plot, SLOT(setResolution(int)));
	connect( boxDistance, SIGNAL(valueChanged(int)), d_plot, SLOT(setLabelsDistance(int)));
};

void Plot3DDialog::worksheet()
{
	d_plot->showWorksheet();
	close();
}

void Plot3DDialog::showBarsTab(double rad)
{
	bars = new QWidget( generalDialog );

	QHBoxLayout* hb = new QHBoxLayout(bars);
	hb->addWidget(new QLabel( tr( "Width" )));

	boxBarsRad = new QLineEdit();
	boxBarsRad->setText(QString::number(rad));
	hb->addWidget(boxBarsRad);

	generalDialog->insertTab(bars, tr( "Bars" ), 4);
}

void Plot3DDialog::showPointsTab(double rad, bool smooth)
{
	boxPointStyle->setCurrentIndex(0);
	boxSize->setText(QString::number(rad));
	boxSmooth->setChecked(smooth);
	optionStack->setCurrentIndex(0);
}

void Plot3DDialog::showConesTab(double rad, int quality)
{
	boxPointStyle->setCurrentIndex(2);
	boxConesRad->setText(QString::number(rad));
	boxQuality->setValue(quality);
	optionStack->setCurrentIndex (2);
}

void Plot3DDialog::showCrossHairTab(double rad, double linewidth, bool smooth, bool boxed)
{
	boxPointStyle->setCurrentIndex(1);
	boxCrossRad->setText(QString::number(rad));
	boxCrossLinewidth->setText(QString::number(linewidth));
	boxCrossSmooth->setChecked(smooth);
	boxBoxed->setChecked(boxed);
	optionStack->setCurrentIndex(1);
}

void Plot3DDialog::disableMeshOptions()
{
	btnMesh->setDisabled(true);
	boxMeshLineWidth->setDisabled(true);
}

void Plot3DDialog::pickDataColorMap()
{
QString fn = QFileDialog::getOpenFileName(d_plot->colorMap(), tr("Colormap files") + " (*.map *.MAP)", this);
if (!fn.isEmpty())
   d_plot->setDataColorMap(fn);
}

void Plot3DDialog::pickTitleFont()
{
	bool ok;
	QFont font = QFontDialog::getFont(&ok,titleFont,this);
	if ( ok ) {
		titleFont = font;
	} else {
		return;
	}
}

void Plot3DDialog::pickNumbersFont()
{
	bool ok;
	QFont font = QFontDialog::getFont(&ok,numbersFont,this);
	if ( ok ) {
		numbersFont = font;
	} else {
		return;
	}
}

void Plot3DDialog::viewAxisOptions(int axis)
{
	boxLabel->setText(labels[axis]);

	boxMajorLength->setText(tickLengths[2*axis+0]);
	boxMinorLength->setText(tickLengths[2*axis+1]);
}

void Plot3DDialog::viewScaleLimits(int axis)
{
	boxFrom->setText(scales[5*axis+0]);
	boxTo->setText(scales[5*axis+1]);
	boxMajors->setValue(scales[5*axis+2].toInt());
	boxMinors->setValue(scales[5*axis+3].toInt());
	boxType->setCurrentIndex(scales[5*axis+4].toInt());
}

void Plot3DDialog::accept()
{
	if (updatePlot())
		close();
}

void Plot3DDialog::changeZoom(int)
{
	if (generalDialog->currentPage() != (QWidget*)general)
		return;

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	d_plot->setZoom(zoom*boxZoom->value()*0.01);
    d_plot->setScale(xScale*boxXScale->value()*0.01, yScale*boxYScale->value()*0.01, zScale*boxZScale->value()*0.01);
    d_plot->update();
    QApplication::restoreOverrideCursor();
}

void Plot3DDialog::changeTransparency(int val)
{
	if (generalDialog->currentPage() != (QWidget*)colors)
		return;

	d_plot->changeTransparency(val*0.01);
	d_plot->update();
}

bool Plot3DDialog::updatePlot()
{
	if (!d_plot)
		return false;

    ApplicationWindow *app = (ApplicationWindow *)this->parent();
    if (!app)
        return false;

	if (generalDialog->currentPage()==(QWidget*)bars){
		d_plot->setBarRadius(boxBarsRad->text().toDouble());
		d_plot->setBarStyle();
	} else if (generalDialog->currentPage() == (QWidget*)points){
		if (boxPointStyle->currentIndex() == 0) {
			d_plot->setDotOptions(boxSize->text().toDouble(), boxSmooth->isChecked());
			d_plot->setDotStyle();
		} else if (boxPointStyle->currentIndex() == 1){
			d_plot->setCrossOptions(boxCrossRad->text().toDouble(), boxCrossLinewidth->text().toDouble(),
					boxCrossSmooth->isChecked(), boxBoxed->isChecked());
            d_plot->setCrossStyle();
        } else if (boxPointStyle->currentIndex() == 2) {
			d_plot->setConeOptions(boxConesRad->text().toDouble(), boxQuality->value());
			d_plot->setConeStyle();
        }

        app->custom3DActions(d_plot);
	} else if (generalDialog->currentPage()==(QWidget*)title){
		d_plot->setTitle(boxTitle->text().remove("\n"), btnTitleColor->color(), titleFont);
	} else if (generalDialog->currentPage()==(QWidget*)colors){
		d_plot->changeTransparency(boxTransparency->value()*0.01);
		d_plot->setDataColors(btnFromColor->color(), btnToColor->color());
		d_plot->setMeshColor(btnMesh->color());
		d_plot->setAxesColor(btnAxes->color());
		d_plot->setNumbersColor(btnNumbers->color());
		d_plot->setLabelsColor(btnLabels->color());
		d_plot->setBackgroundColor(btnBackground->color());
		d_plot->setGridColor(btnGrid->color());
	} else if (generalDialog->currentPage()==(QWidget*)general){
		d_plot->showColorLegend(boxLegend->isChecked());
		d_plot->setResolution(boxResolution->value());
		d_plot->setMeshLineWidth(boxMeshLineWidth->value());
		d_plot->setLabelsDistance(boxDistance->value());
		d_plot->setNumbersFont(numbersFont);
		d_plot->setZoom(zoom*boxZoom->value()*0.01);
		d_plot->setScale(xScale*boxXScale->value()*0.01, yScale*boxYScale->value()*0.01, zScale*boxZScale->value()*0.01);
	} else if (generalDialog->currentPage()==(QWidget*)scale){
		int axis = axesList->currentRow();
		QString from=boxFrom->text().lower();
		QString to=boxTo->text().lower();
		double start, end;
		try {
			MyParser parser;
			parser.SetExpr(from.ascii());
			start = parser.Eval();
		} catch(mu::ParserError &e){
			QMessageBox::critical(0,tr("MantidPlot - Start limit error"),  QString::fromStdString(e.GetMsg()));
			boxFrom->setFocus();
			return false;
		}

		try {
			MyParser parser;
			parser.SetExpr(to.ascii());
			end = parser.Eval();
		} catch(mu::ParserError &e){
			QMessageBox::critical(0,tr("MantidPlot - End limit error"), QString::fromStdString(e.GetMsg()));
			boxTo->setFocus();
			return false;
		}

        /*double xsc = d_plot->xScale();
        double ysc = d_plot->yScale();
        double zsc = d_plot->zScale();
        if (axis == 2)
        {
            double start0 = scales[0].toDouble();
            double end0 = scales[1].toDouble();
            zsc *= (end0 - start0)/(end - start);
            QMessageBox::information(this,"OK","OK");
        }*/

        d_plot->updateScale(axis, scaleOptions(axis, start, end, boxMajors->text(), boxMinors->text()));
        //d_plot->setScale(xsc,ysc,zsc*0.1);
	} else if (generalDialog->currentPage()==(QWidget*)axes){
		int axis = axesList2->currentRow();
		labels[axis] = boxLabel->text();

		switch(axis)
		{
		case 0:
			d_plot->setXAxisLabel(boxLabel->text().remove("\n"));
			d_plot->setXAxisLabelFont(axisFont(axis));
			d_plot->setXAxisTickLength(boxMajorLength->text().toDouble(), boxMinorLength->text().toDouble());
		break;
		case 1:
			d_plot->setYAxisLabel(boxLabel->text().remove("\n"));
			d_plot->setYAxisLabelFont(axisFont(axis));
			d_plot->setYAxisTickLength(boxMajorLength->text().toDouble(), boxMinorLength->text().toDouble());
		break;
		case 2:
			d_plot->setZAxisLabel(boxLabel->text().remove("\n"));
			d_plot->setZAxisLabelFont(axisFont(axis));
			d_plot->setZAxisTickLength(boxMajorLength->text().toDouble(), boxMinorLength->text().toDouble());
		break;
		}
	}

    d_plot->update();
    app->modifiedProject(d_plot);
	return true;
}

QStringList Plot3DDialog::scaleOptions(int axis, double start, double end,
		const QString& majors, const QString& minors)
{
	QStringList l;
	l<<QString::number(start);
	l<<QString::number(end);
	l<<majors;
	l<<minors;
	l<<QString::number(boxType->currentIndex());

	for (int i=0;i<5;i++)
		scales[5*axis+i]=l[i];
	return l;
}

void Plot3DDialog::pickAxisLabelFont()
{
	bool ok;
	QFont font;
	switch(axesList2->currentRow())
	{
		case 0:
			font= QFontDialog::getFont(&ok,xAxisFont,this);
			if ( ok )
				xAxisFont=font;
			else
				return;
			break;

		case 1:
			font= QFontDialog::getFont(&ok,yAxisFont,this);
			if ( ok )
				yAxisFont=font;
			else
				return;
			break;

		case 2:
			font= QFontDialog::getFont(&ok,zAxisFont,this);
			if ( ok )
				zAxisFont=font;
			else
				return;
			break;
	}
}

QFont Plot3DDialog::axisFont(int axis)
{
	QFont f;
	switch(axis)
	{
		case 0:
			f=xAxisFont;
			break;

		case 1:
			f=yAxisFont;
			break;

		case 2:
			f=zAxisFont;
			break;
	}
	return f;
}

void Plot3DDialog::showGeneralTab()
{
	generalDialog->setCurrentWidget (general);
}

void Plot3DDialog::showTitleTab()
{
	generalDialog->setCurrentWidget (title);
}

void Plot3DDialog::showAxisTab()
{
	generalDialog->setCurrentWidget (axes);
}
