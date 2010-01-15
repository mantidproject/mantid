/***************************************************************************
    File                 : TextDialog.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2004 - 2008 by Ion Vasilief
    Email (use @ for *)  : ion_vasilief*yahoo.fr
    Description          : Text label/axis label options dialog

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

#include "TextDialog.h"
#include "ApplicationWindow.h"
#include "LegendWidget.h"

#include <QFontDialog>
#include <QFont>
#include <QGroupBox>
#include <QTextEdit>
#include <QTextCursor>
#include <QComboBox>
#include <QLabel>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSpinBox>
#include <QCheckBox>

#include <qwt_scale_widget.h>

TextDialog::TextDialog(TextType type, QWidget* parent, Qt::WFlags fl)
	: QDialog( parent, fl)
{
	setAttribute(Qt::WA_DeleteOnClose);
	setWindowTitle( tr( "MantidPlot - Text options" ) );
	setSizeGripEnabled( true );

	d_graph = NULL;
	d_scale = NULL;
	d_legend = NULL;

	textType = type;

	// top groupbox
	groupBox1 = new QGroupBox(QString());
	QGridLayout * topLayout = new QGridLayout(groupBox1);
	topLayout->addWidget(new QLabel(tr("Text Color")), 0, 0);

	colorBtn = new ColorButton();
	topLayout->addWidget(colorBtn, 0, 1);

	buttonOk = new QPushButton(tr("&OK"));
	buttonOk->setAutoDefault( true );
	buttonOk->setDefault( true );

	topLayout->addWidget(buttonOk, 0, 3);
	topLayout->addWidget(new QLabel(tr("Font")), 1, 0);

	buttonFont = new QPushButton(tr( "&Font" ));
	topLayout->addWidget(buttonFont, 1, 1);

	buttonApply = new QPushButton(tr( "&Apply" ));
	buttonApply->setDefault( true );
	topLayout->addWidget( buttonApply, 1, 3 );

	if (textType != TextDialog::TextMarker){
		topLayout->addWidget(new QLabel(tr("Alignment")), 2, 0);
		alignmentBox = new QComboBox();
		alignmentBox->addItem( tr( "Center" ) );
		alignmentBox->addItem( tr( "Left" ) );
		alignmentBox->addItem( tr( "Right" ) );
		topLayout->addWidget(alignmentBox, 2, 1);

		boxApplyToAll = new QCheckBox(tr("Apply format to all &labels in layer"));
		topLayout->addWidget(boxApplyToAll, 3, 0 );
	} else {
		topLayout->addWidget(new QLabel(tr("Frame")), 2, 0);
		backgroundBox = new QComboBox();
		backgroundBox->addItem( tr( "None" ) );
		backgroundBox->addItem( tr( "Rectangle" ) );
		backgroundBox->addItem( tr( "Shadow" ) );
		topLayout->addWidget(backgroundBox, 2, 1);
	}

	buttonCancel = new QPushButton( tr( "&Cancel" ) );
	topLayout->addWidget( buttonCancel, 2, 3 );

	if (textType == TextMarker)
	{ //TODO: Sometime background features for axes lables should be implemented
		topLayout->addWidget(new QLabel(tr("Opacity")), 3, 0);
		boxBackgroundTransparency = new QSpinBox();
		boxBackgroundTransparency->setRange(0, 255);
     	boxBackgroundTransparency->setSingleStep(5);
		boxBackgroundTransparency->setWrapping(true);
     	boxBackgroundTransparency->setSpecialValueText(tr("Transparent"));

		topLayout->addWidget( boxBackgroundTransparency, 3, 1 );
		topLayout->addWidget(new QLabel(tr("Background color")), 4, 0);
		backgroundBtn = new ColorButton(groupBox1);
		backgroundBtn->setEnabled(false);
		topLayout->addWidget( backgroundBtn, 4, 1 );

		connect(boxBackgroundTransparency, SIGNAL(valueChanged(int)),
				this, SLOT(updateTransparency(int)));

		boxApplyToAll = new QCheckBox(tr("Apply format to all &labels in layer"));
		topLayout->addWidget(boxApplyToAll, 5, 0 );

		buttonDefault = new QPushButton( tr( "Set As &Default" ) );
		topLayout->addWidget( buttonDefault, 3, 3 );
		connect( buttonDefault, SIGNAL(clicked()), this, SLOT(setDefaultValues()));
	}

	// align the OK, Apply, and Cancel buttons to the right
	topLayout->setColumnStretch(2, 1);

	/* TODO: Angle feature not implemented, yet
	 * caution: This code is still the old Qt3 code
	   QLabel* rotate=new QLabel(tr( "Rotate (deg.)" ),GroupBox1, "TextLabel1_2",0);
	   rotate->hide();

	   rotateBox = new QComboBox( false, GroupBox1, "rotateBox" );
	   rotateBox->insertItem( tr( "0" ) );
	   rotateBox->insertItem( tr( "45" ) );
	   rotateBox->insertItem( tr( "90" ) );
	   rotateBox->insertItem( tr( "135" ) );
	   rotateBox->insertItem( tr( "180" ) );
	   rotateBox->insertItem( tr( "225" ) );
	   rotateBox->insertItem( tr( "270" ) );
	   rotateBox->insertItem( tr( "315" ) );
	   rotateBox->setEditable (true);
	   rotateBox->setCurrentItem(0);
	   rotateBox->hide();
	   */

	textEditBox = new QTextEdit();
	textEditBox->setTextFormat(Qt::PlainText);
	textEditBox->setFont(QFont());

	formatButtons =  new TextFormatButtons(textEditBox);
	formatButtons->toggleCurveButton(textType == TextMarker);

	setFocusPolicy(Qt::StrongFocus);
	setFocusProxy(textEditBox);

	QVBoxLayout* mainLayout = new QVBoxLayout();
	mainLayout->addWidget(groupBox1);
	mainLayout->addWidget(formatButtons);
	mainLayout->addWidget(textEditBox);
	setLayout( mainLayout );

	// signals and slots connections
	connect( buttonOk, SIGNAL( clicked() ), this, SLOT( accept() ) );
	connect( buttonApply, SIGNAL( clicked() ), this, SLOT( apply() ) );
	connect( buttonCancel, SIGNAL( clicked() ), this, SLOT( reject() ) );
	connect( buttonFont, SIGNAL( clicked() ), this, SLOT(customFont() ) );
}

void TextDialog::setGraph(Graph *g)
{
	if (!g)
		return;

	d_graph = g;
	QwtText l;
	if (textType == LayerTitle)
		l = d_graph->plotWidget()->title();
	else if (textType == AxisTitle){
		d_scale = g->currentScale();
		if (!d_scale)
			return;

		l =	d_scale->title();
		switch(d_scale->alignment()){
			case QwtScaleDraw::BottomScale:
				setWindowTitle(tr("MantidPlot") + " - " + tr("X Axis Title"));
			break;
			case QwtScaleDraw::LeftScale:
				setWindowTitle(tr("MantidPlot") + " - " + tr("Y Axis Title"));
			break;
			case QwtScaleDraw::TopScale:
				setWindowTitle(tr("MantidPlot") + " - " + tr("Top Axis Title"));
			break;
			case QwtScaleDraw::RightScale:
				setWindowTitle(tr("MantidPlot") + " - " + tr("Right Axis Title"));
			break;
		}
	}

	setAlignment(l.renderFlags());
	setText(l.text());
	selectedFont = l.font();
	colorBtn->setColor(l.color());
}

void TextDialog::setLegendWidget(LegendWidget *l)
{
	if (!l)
		return;

	d_graph = (Graph *)(l->plot()->parent());
	d_legend = l;

	setText(l->text());
	selectedFont = l->font();
	colorBtn->setColor(l->textColor());

	QColor bc = l->backgroundColor();
	boxBackgroundTransparency->setValue(bc.alpha());
	backgroundBtn->setEnabled(bc.alpha());
	backgroundBtn->setColor(bc);

	backgroundBox->setCurrentIndex(l->frameStyle());

	d_legend->setSelected(false);
}

void TextDialog::apply()
{
	if (textType == AxisTitle){
		if (!d_graph || !d_scale)
			return;

		QwtText t =	d_scale->title();
		t.setRenderFlags(alignment());
		t.setText(textEditBox->toPlainText());
		d_scale->setTitle(t);

		if (boxApplyToAll->isChecked())
			formatAllLabels();
		else {
			t.setFont(selectedFont);
			t.setColor(colorBtn->color());
			d_scale->setTitle(t);
			d_graph->replot();
		}
	} else if (textType == TextMarker && d_legend){
		QColor tc = colorBtn->color();
		QColor c = backgroundBtn->color();
		c.setAlpha(boxBackgroundTransparency->value());

		d_legend->setText(textEditBox->text());
		if (boxApplyToAll->isChecked())
			formatAllLabels();
		else {
			d_legend->setBackgroundColor(c);
			d_legend->setTextColor(colorBtn->color());
			d_legend->setFrameStyle(backgroundBox->currentIndex());
			d_legend->setFont(selectedFont);
			d_legend->repaint();
		}
	} else if (textType == LayerTitle){
		if (!d_graph)
			return;

		Plot *plot = d_graph->plotWidget();
		QwtText t =	plot->title();
		t.setRenderFlags(alignment());
		t.setText(textEditBox->toPlainText());
		plot->setTitle(t);

		if (boxApplyToAll->isChecked())
			formatAllLabels();
		else {
			t.setFont(selectedFont);
			t.setColor(colorBtn->color());
			plot->setTitle(t);
			plot->replot();
		}
	}

	if (d_graph)
		d_graph->notifyChanges();
}

void TextDialog::formatAllLabels()
{
	if (!d_graph)
		return;

	Plot *plot = d_graph->plotWidget();
	if (!plot)
		return;

	QColor tc = colorBtn->color();
	QObjectList lst = plot->children();
	foreach(QObject *o, lst){
		if (o->inherits("LegendWidget")){
			LegendWidget *l = (LegendWidget *)o;
        	l->setTextColor(tc);
			l->setFont(selectedFont);
			if(textType == TextMarker){
				QColor c = backgroundBtn->color();
				c.setAlpha(boxBackgroundTransparency->value());
				l->setBackgroundColor(c);
				l->setFrameStyle(backgroundBox->currentIndex());
			}
		}
	}

	for (int i=0; i < QwtPlot::axisCnt; i++){
		QwtScaleWidget *scale = (QwtScaleWidget *)plot->axisWidget(i);
		if (scale){
			QwtText t = scale->title();
			t.setColor(tc);
			t.setFont(selectedFont);
			scale->setTitle(t);
		}
	}

	QwtText t = plot->title();
	t.setColor(tc);
	t.setFont(selectedFont);
	plot->setTitle (t);
	plot->replot();
}

void TextDialog::setDefaultValues()
{
	ApplicationWindow *app = (ApplicationWindow *)this->parent();
	if (!app)
		return;

	QColor c = backgroundBtn->color();
	c.setAlpha(boxBackgroundTransparency->value());
	app->setLegendDefaultSettings(backgroundBox->currentIndex(), selectedFont, colorBtn->color(), c);
}

void TextDialog::accept()
{
	apply();
	close();
}

int TextDialog::alignment()
{
	int align=-1;
	switch (alignmentBox->currentIndex())
	{
		case 0:
			align = Qt::AlignHCenter;
			break;

		case 1:
			align = Qt::AlignLeft;
			break;

		case 2:
			align = Qt::AlignRight;
			break;
	}
	return align;
}

void TextDialog::setAlignment(int align)
{
	switch(align)
	{
		case Qt::AlignHCenter:
			alignmentBox->setCurrentIndex(0);
			break;
		case Qt::AlignLeft:
			alignmentBox->setCurrentIndex(1);
			break;
		case Qt::AlignRight:
			alignmentBox->setCurrentIndex(2);
			break;
	}
}

void TextDialog::customFont()
{
	bool okF;
	QFont fnt = QFontDialog::getFont( &okF, selectedFont, this);
	if (okF && fnt != selectedFont)
		selectedFont = fnt;
}

void TextDialog::setText(const QString & t)
{
	QTextCursor cursor = textEditBox->textCursor();
	// select the whole (old) text
	cursor.movePosition(QTextCursor::Start);
	cursor.movePosition(QTextCursor::End,QTextCursor::KeepAnchor);
	// replace old text
	cursor.insertText(t);
	// select the whole (new) text
	cursor.movePosition(QTextCursor::Start);
	cursor.movePosition(QTextCursor::End,QTextCursor::KeepAnchor);
	// this line makes the selection visible to the user
	// (the 2 lines above only change the selection in the
	// underlying QTextDocument)
	textEditBox->setTextCursor(cursor);
	// give focus back to text edit
	textEditBox->setFocus();
}

void TextDialog::updateTransparency(int alpha)
{
backgroundBtn->setEnabled(alpha);
}
