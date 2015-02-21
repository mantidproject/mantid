/***************************************************************************
 File                 : PlotDialog.cpp
 Project              : QtiPlot
 --------------------------------------------------------------------
 Copyright            : (C) 2006 by Ion Vasilief, Tilman Hoener zu Siederdissen
 Email (use @ for *)  : ion_vasilief*yahoo.fr, thzs*gmx.net
 Description          : Custom curves dialog

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
#include "ApplicationWindow.h"
#include "PlotDialog.h"
#include "ColorBox.h"
#include "ColorButton.h"
#include "PatternBox.h"
#include "SymbolBox.h"
#include "Table.h"
#include "MyParser.h"
#include "QwtHistogram.h"
#include "VectorCurve.h"
#include "QwtErrorPlotCurve.h"
#include "BoxCurve.h"
#include "FunctionCurve.h"
#include "Spectrogram.h"
#include "QwtPieCurve.h"
#include "ColorMapEditor.h"
#include "pixmaps.h"
#include "DoubleSpinBox.h"
#include "Folder.h"
#include "ContourLinesEditor.h"
#include "PenStyleBox.h"

#include <QTreeWidget>
#include <QLineEdit>
#include <QLayout>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QRadioButton>
#include <QLabel>
#include <QWidget>
#include <QMessageBox>
#include <QComboBox>
#include <QWidgetList>
#include <QFileDialog>
#include <QGroupBox>
#include <QFontDialog>
#include <QShortcut>
#include <QKeySequence>
#include <QDoubleSpinBox>
#include <QMenu>
#include <QDateTime>
#include "MantidKernel/ConfigService.h"
#include "MantidQtAPI/MantidColorMap.h"

using Mantid::Kernel::ConfigService;

PlotDialog::PlotDialog(bool showExtended, ApplicationWindow* app, MultiLayer *ml, Qt::WFlags fl) :
    QDialog(ml, fl), d_app(app), d_ml(0)
{
  setName("PlotDialog");
  setWindowTitle(tr("MantidPlot - Plot details"));
  setModal(true);
  setSizeGripEnabled(true);
  listBox = new QTreeWidget();
  listBox->setColumnCount(1);
  listBox->header()->hide();
  listBox->setIndentation(15);

  QGridLayout *gl = new QGridLayout(this);
  gl->addWidget(listBox, 0, 0);

  privateTabWidget = new QTabWidget();
  gl->addWidget(privateTabWidget, 0, 1);

  curvePlotTypeBox = new QWidget();
  QHBoxLayout *hb1 = new QHBoxLayout(curvePlotTypeBox);
  hb1->addWidget(new QLabel(tr("Plot type")));
  boxPlotType = new QComboBox();
  boxPlotType->setEditable(false);
  hb1->addWidget(boxPlotType);
  gl->addWidget(curvePlotTypeBox, 1, 0);

  initAxesPage();
  initLinePage();
  initSymbolsPage();
  initHistogramPage();
  initErrorsPage();
  initSpacingPage();
  initVectPage();
  initBoxPage();
  initPercentilePage();
  initSpectrogramPage();
  initPiePage();
  initPieGeometryPage();
  initPieLabelsPage();
  initLayerPage();
  initLayerGeometryPage();
  initFontsPage();
  initPrintPage();
  initLabelsPage();
  initContourLinesPage();

  clearTabWidget();

  QHBoxLayout* hb2 = new QHBoxLayout();
  btnMore = new QPushButton("&<<");
  btnMore->setFixedWidth(30);
  btnMore->setCheckable(true);
  if (showExtended)
    btnMore->toggle();
  hb2->addWidget(btnMore);
  btnWorksheet = new QPushButton(tr("&Worksheet"));
  hb2->addWidget(btnWorksheet);
  buttonOk = new QPushButton(tr("&OK"));
  buttonOk->setDefault(true);
  hb2->addWidget(buttonOk);
  buttonCancel = new QPushButton(tr("&Cancel"));
  hb2->addWidget(buttonCancel);
  buttonApply = new QPushButton(tr("&Apply"));
  hb2->addWidget(buttonApply);
  btnEditCurve = new QPushButton(tr("&Plot Associations..."));
  hb2->addWidget(btnEditCurve);
  hb2->addStretch();
  gl->addLayout(hb2, 1, 1);

  connect(btnMore, SIGNAL(toggled(bool)), this, SLOT(showAll(bool)));

  connect(buttonOk, SIGNAL(clicked()), this, SLOT(quit()));
  connect(buttonCancel, SIGNAL(clicked()), this, SLOT(close()));
  connect(buttonApply, SIGNAL(clicked() ), this, SLOT(acceptParams()));
  connect(btnWorksheet, SIGNAL(clicked()), this, SLOT(showWorksheet()));
  connect(btnEditCurve, SIGNAL(clicked()), this, SLOT(editCurve()));
  connect(listBox, SIGNAL(itemDoubleClicked( QTreeWidgetItem *, int)), this,
  SLOT(showPlotAssociations( QTreeWidgetItem *, int)));
  connect(listBox, SIGNAL(currentItemChanged (QTreeWidgetItem *, QTreeWidgetItem *)), this,
  SLOT(updateTabWindow(QTreeWidgetItem *, QTreeWidgetItem *)));
  connect(listBox, SIGNAL(itemCollapsed(QTreeWidgetItem *)), this,
  SLOT(updateTreeWidgetItem(QTreeWidgetItem *)));
  connect(listBox, SIGNAL(itemExpanded(QTreeWidgetItem *)), this,
  SLOT(updateTreeWidgetItem(QTreeWidgetItem *)));
  connect(boxPlotType, SIGNAL(currentIndexChanged(int)), this, SLOT(changePlotType(int)));

  QShortcut *shortcut = new QShortcut(QKeySequence(Qt::Key_Delete), this);
  connect(shortcut, SIGNAL(activated()), this, SLOT(removeSelectedCurve()));

  setMultiLayer(ml);
}

void PlotDialog::showAll(bool all)
{
  if (all)
  {
    listBox->show();
    listBox->setFocus();

    QTreeWidgetItem *item = listBox->currentItem();
    if (item->type() == CurveTreeItem::PlotCurveTreeItem)
      curvePlotTypeBox->show();

    btnMore->setText("&>>");
  }
  else
  {
    listBox->hide();
    curvePlotTypeBox->hide();
    btnMore->setText("&<<");
  }
}
void PlotDialog::initContourLinesPage()
{
  QLocale locale = d_app->locale();

  contourLinesPage = new QWidget();

  levelsGroupBox = new QGroupBox(tr("&Show Contour Lines"));
  levelsGroupBox->setCheckable(true);
  QHBoxLayout *hl0 = new QHBoxLayout();

  QGroupBox *gb1 = new QGroupBox(tr("Set Equidistant Levels"));
  QGridLayout *hl1 = new QGridLayout(gb1);

  hl1->addWidget(new QLabel(tr("Levels")), 0, 0);
  levelsBox = new QSpinBox();
  levelsBox->setRange(2, 1000);
  hl1->addWidget(levelsBox, 0, 1);

  hl1->addWidget(new QLabel(tr("Start")), 1, 0);
  firstContourLineBox = new DoubleSpinBox();
  firstContourLineBox->setLocale(locale);
  firstContourLineBox->setDecimals(6);
  hl1->addWidget(firstContourLineBox, 1, 1);

  hl1->addWidget(new QLabel(tr("Step")), 2, 0);
  contourLinesDistanceBox = new DoubleSpinBox();
  contourLinesDistanceBox->setLocale(locale);
  contourLinesDistanceBox->setDecimals(6);
  hl1->addWidget(contourLinesDistanceBox, 2, 1);

  btnSetEquidistantLevels = new QPushButton(tr("Set &Levels"));
  connect(btnSetEquidistantLevels, SIGNAL(clicked()), this, SLOT(setEquidistantLevels()));
  hl1->addWidget(btnSetEquidistantLevels, 3, 1);

  hl1->setColumnStretch(1, 10);
  hl1->setRowStretch(4, 1);

  contourLinesEditor = new ContourLinesEditor(d_app->locale());
  hl0->addWidget(contourLinesEditor);
  hl0->addWidget(gb1);

  QGroupBox *penGroupBox = new QGroupBox(tr("Pen"));
  QHBoxLayout *hl2 = new QHBoxLayout(penGroupBox);

  QVBoxLayout *vl1 = new QVBoxLayout;
  autoContourBox = new QRadioButton(tr("Use &Color Map"));
  connect(autoContourBox, SIGNAL(toggled(bool)), this, SLOT(showDefaultContourLinesBox(bool)));
  vl1->addWidget(autoContourBox);

  defaultContourBox = new QRadioButton(tr("Use Default &Pen"));
  connect(defaultContourBox, SIGNAL(toggled(bool)), this, SLOT(showDefaultContourLinesBox(bool)));
  vl1->addWidget(defaultContourBox);

  customPenBtn = new QRadioButton(tr("Use &Table Custom Pen"));
  connect(customPenBtn, SIGNAL(toggled(bool)), this, SLOT(showCustomPenColumn(bool)));
  vl1->addWidget(customPenBtn);

  hl2->addLayout(vl1);

  defaultPenBox = new QGroupBox();
  QGridLayout *gl1 = new QGridLayout(defaultPenBox);
  gl1->addWidget(new QLabel(tr("Color")), 0, 0);

  levelsColorBox = new ColorButton(defaultPenBox);
  gl1->addWidget(levelsColorBox, 0, 1);

  gl1->addWidget(new QLabel(tr("Width")), 1, 0);
  contourWidthBox = new DoubleSpinBox('f');
  contourWidthBox->setLocale(locale);
  contourWidthBox->setSingleStep(0.1);
  contourWidthBox->setRange(0, 100);
  gl1->addWidget(contourWidthBox, 1, 1);

  gl1->addWidget(new QLabel(tr("Style")), 2, 0);
  //penContourStyle = new PenStyleBox();
  boxContourStyle = new PenStyleBox();
  //gl1->addWidget((QWidget*)penContourStyle, 2, 1);
  gl1->addWidget(boxContourStyle, 2, 1);
  hl2->addWidget(defaultPenBox);

  QVBoxLayout *vl0 = new QVBoxLayout(levelsGroupBox);
  vl0->addLayout(hl0);
  vl0->addWidget(penGroupBox);
  vl0->addStretch();

  QVBoxLayout* vl2 = new QVBoxLayout(contourLinesPage);
  vl2->addWidget(levelsGroupBox);

  privateTabWidget->insertTab(contourLinesPage, tr("Contour Lines"));
  //privateTabWidget->addTab(layerGeometryPage, tr("Contour Lines"));
}
void PlotDialog::setEquidistantLevels()
{
  QTreeWidgetItem *it = listBox->currentItem();
  if (!it)
    return;

  CurveTreeItem *item = dynamic_cast<CurveTreeItem*>(it);
  QwtPlotItem *plotItem = dynamic_cast<QwtPlotItem *>(item->plotItem());
  if (!plotItem)
    return;

  Spectrogram *sp = dynamic_cast<Spectrogram*>(plotItem);
  if (!sp || sp->rtti() != QwtPlotItem::Rtti_PlotSpectrogram)
    return;

  QwtValueList levels;
  double firstVal = firstContourLineBox->value();
  for (int i = 0; i < levelsBox->value(); i++)
    levels << firstVal + i * contourLinesDistanceBox->value();
  sp->setContourLevels(levels);
  sp->plot()->replot();
  contourLinesEditor->updateContents();
}
void PlotDialog::showCustomPenColumn(bool on)
{
  contourLinesEditor->showPenColumn(on);
  if (on)
    defaultPenBox->hide();
}

void PlotDialog::showPlotAssociations(QTreeWidgetItem *item, int)
{
  if (!item)
    return;

  if (!d_app)
    return;

  if (item->type() != CurveTreeItem::PlotCurveTreeItem)
    return;

  QwtPlotItem *it = dynamic_cast<QwtPlotItem *>(dynamic_cast<CurveTreeItem*>(item)->plotItem()); // crazy
  if (!it)
    return;

  if (it->rtti() == QwtPlotItem::Rtti_PlotSpectrogram)
  {
    Spectrogram *sp = dynamic_cast<Spectrogram*>(it);
    if (sp && sp->matrix())
      sp->matrix()->showMaximized();
    return;
  }

  hide();
  if (dynamic_cast<PlotCurve *>(it)->type() == Graph::Function)
  {
    FunctionDialog *fd = d_app->showFunctionDialog((dynamic_cast<CurveTreeItem*>(item))->graph(),
        dynamic_cast<CurveTreeItem*>(item)->plotItemIndex());
    if (fd)
      connect((QObject *) fd, SIGNAL(destroyed()), this, SLOT(show()));
  }
  else
  {
    AssociationsDialog* ad = d_app->showPlotAssociations(
        dynamic_cast<CurveTreeItem*>(item)->plotItemIndex());
    if (ad)
      connect((QObject *) ad, SIGNAL(destroyed()), this, SLOT(show()));
  }
}

void PlotDialog::editCurve()
{

  CurveTreeItem *item = dynamic_cast<CurveTreeItem *>(listBox->currentItem());
  if (!item)
    return;
  if (item->type() != CurveTreeItem::PlotCurveTreeItem)
    return;

  int index = item->plotItemIndex();
  PlotCurve* pc = dynamic_cast<PlotCurve *>(item->plotItem());
  if (!pc)
    return;
  int curveType = pc->type();

  hide();

  if (d_app)
  {
    if (curveType == Graph::Function)
    {
      FunctionDialog *fd = d_app->showFunctionDialog(item->graph(), index);
      if (fd)
        connect(reinterpret_cast<QObject *>(fd), SIGNAL(destroyed()), this, SLOT(show()));
    }
    else
    {
      AssociationsDialog* ad = d_app->showPlotAssociations(index);
      if (ad)
        connect(reinterpret_cast<QObject *>(ad), SIGNAL(destroyed()), this, SLOT(show()));
    }
  }
}

void PlotDialog::changePlotType(int plotType)
{
  if (boxPlotType->count() == 1)
    return;

  CurveTreeItem *item = dynamic_cast<CurveTreeItem *>(listBox->currentItem());
  if (!item)
    return;
  if (item->type() != CurveTreeItem::PlotCurveTreeItem)
    return;
  Graph *graph = item->graph();
  if (!graph)
    return;

  int curveType = item->plotItemType();
  if (curveType == Graph::ColorMap || curveType == Graph::Contour || curveType == Graph::GrayScale)
    clearTabWidget();
  else if (curveType == Graph::VectXYAM || curveType == Graph::VectXYXY)
  {
    if ((plotType && curveType == Graph::VectXYAM) || (!plotType && curveType == Graph::VectXYXY))
      return;

    clearTabWidget();
    insertTabs(curveType);

    VectorCurve *v = dynamic_cast<VectorCurve*>(item->plotItem());
    if (plotType)
    {
      graph->setCurveType(item->plotItemIndex(), Graph::VectXYAM);
      v->setVectorStyle(VectorCurve::XYAM);
    }
    else
    {
      graph->setCurveType(item->plotItemIndex(), Graph::VectXYXY);
      v->setVectorStyle(VectorCurve::XYXY);
    }
    customVectorsPage(plotType);
  }
  else
  {
    clearTabWidget();
    insertTabs(plotType);

    graph->setCurveType(item->plotItemIndex(), plotType);

    boxConnect->setCurrentIndex(1); //show line for Line and LineSymbol plots

    QwtSymbol s = QwtSymbol(QwtSymbol::Ellipse, QBrush(), QPen(), QSize(9, 9));
    if (plotType == Graph::Line)
      s.setStyle(QwtSymbol::NoSymbol);
    else if (plotType == Graph::Scatter)
      graph->setCurveStyle(item->plotItemIndex(), QwtPlotCurve::NoCurve);
    else if (plotType == Graph::LineSymbols)
      graph->setCurveStyle(item->plotItemIndex(), QwtPlotCurve::Lines);

    graph->setCurveSymbol(item->plotItemIndex(), s);

    if (plotType)
    {
      boxSymbolStyle->setCurrentIndex(1);
      boxFillSymbol->setChecked(false);
      boxFillColor->setEnabled(false);
    }
  }
  acceptParams();
}

/**
 * Changes the graph's plot stlye from somewhere other than the plot dialog.
 *
 * @params plotType :: This is the plot style number. i.e line is 0 and scatter is 1.
 * @params color :: The color the user specified for the plot.
 */
void PlotDialog::setPlotType(int plotType, int curveNum, const QString & color)
{
  selectCurve(curveNum);
  CurveTreeItem *item = dynamic_cast<CurveTreeItem*>(listBox->currentItem());
  if (!item)
    return;
  if (item->type() != CurveTreeItem::PlotCurveTreeItem)
    return;
  Graph *graph = item->graph();
  if (!graph)
    return;

  QwtSymbol s = QwtSymbol(QwtSymbol::Ellipse, QBrush(), QPen(), QSize(5, 5));
  if (plotType == Graph::Line)
    s.setStyle(QwtSymbol::NoSymbol);
  else if (plotType == Graph::Scatter)
    graph->setCurveStyle(item->plotItemIndex(), QwtPlotCurve::NoCurve);
  else if (plotType == Graph::LineSymbols)
    graph->setCurveStyle(item->plotItemIndex(), QwtPlotCurve::Lines);

  if (color != "Default")
    s.setPen(QPen(QColor(color)));

  graph->setCurveSymbol(item->plotItemIndex(), s);
}

void PlotDialog::initFontsPage()
{
  QGroupBox *boxFonts = new QGroupBox();
  QGridLayout *fl = new QGridLayout(boxFonts);

  btnTitle = new QPushButton(tr("Titles"));
  btnAxesLabels = new QPushButton(tr("Axes Labels"));
  btnAxesNumbers = new QPushButton(tr("Axes Numbers"));
  btnLegend = new QPushButton(tr("Legends"));

  fl->addWidget(btnTitle, 0, 0);
  fl->addWidget(btnAxesLabels, 0, 1);
  fl->addWidget(btnAxesNumbers, 0, 2);
  fl->addWidget(btnLegend, 0, 3);
  fl->setRowStretch(1, 1);
  fl->setColumnStretch(4, 1);

  fontsPage = new QWidget();
  QHBoxLayout *hl = new QHBoxLayout(fontsPage);
  hl->addWidget(boxFonts);
  privateTabWidget->addTab(fontsPage, tr("Fonts"));

  connect(btnTitle, SIGNAL( clicked() ), this, SLOT( setTitlesFont() ));
  connect(btnAxesLabels, SIGNAL( clicked() ), this, SLOT( setAxesLabelsFont() ));
  connect(btnAxesNumbers, SIGNAL( clicked() ), this, SLOT( setAxesNumbersFont() ));
  connect(btnLegend, SIGNAL( clicked() ), this, SLOT( setLegendsFont() ));
}

void PlotDialog::initLayerPage()
{
  layerPage = new QWidget();

  QGroupBox * boxBkg = new QGroupBox();
  QGridLayout * boxBkgLayout = new QGridLayout(boxBkg);

  boxBkgLayout->addWidget(new QLabel(tr("Background Color")), 0, 0);
  boxBackgroundColor = new ColorButton();
  boxBkgLayout->addWidget(boxBackgroundColor, 0, 1);
  boxBkgLayout->addWidget(new QLabel(tr("Opacity")), 0, 2);
  boxBackgroundTransparency = new QSpinBox();
  boxBackgroundTransparency->setRange(0, 255);
  boxBackgroundTransparency->setSingleStep(5);
  boxBackgroundTransparency->setWrapping(true);
  boxBackgroundTransparency->setSpecialValueText(tr("Transparent"));
  boxBkgLayout->addWidget(boxBackgroundTransparency, 0, 3);

  boxBkgLayout->addWidget(new QLabel(tr("Canvas Color")), 1, 0);
  boxCanvasColor = new ColorButton();
  boxBkgLayout->addWidget(boxCanvasColor, 1, 1);
  boxBkgLayout->addWidget(new QLabel(tr("Opacity")), 1, 2);
  boxCanvasTransparency = new QSpinBox();
  boxCanvasTransparency->setRange(0, 255);
  boxCanvasTransparency->setSingleStep(5);
  boxCanvasTransparency->setWrapping(true);
  boxCanvasTransparency->setSpecialValueText(tr("Transparent"));
  boxBkgLayout->addWidget(boxCanvasTransparency, 1, 3);

  boxBkgLayout->addWidget(new QLabel(tr("Border Color")), 2, 0);
  boxBorderColor = new ColorButton();
  boxBkgLayout->addWidget(boxBorderColor, 2, 1);
  boxBkgLayout->addWidget(new QLabel(tr("Width")), 2, 2);
  boxBorderWidth = new QSpinBox();
  boxBkgLayout->addWidget(boxBorderWidth, 2, 3);
  boxBkgLayout->setRowStretch(4, 1);

  boxBkgLayout->addWidget(new QLabel(tr("Canvas Frame Color")), 3, 0);
  boxCanvasFrameColor = new ColorButton();
  boxBkgLayout->addWidget(boxCanvasFrameColor, 3, 1);
  boxBkgLayout->addWidget(new QLabel(tr("Width")), 3, 2);
  boxCanvasFrameWidth = new QSpinBox();
  boxBkgLayout->addWidget(boxCanvasFrameWidth, 3, 3);
  boxBkgLayout->setRowStretch(4, 1);

  QGroupBox * box4 = new QGroupBox(QString());
  QGridLayout * box4Layout = new QGridLayout(box4);

  box4Layout->addWidget(new QLabel(tr("Margin")), 0, 0);
  boxMargin = new QSpinBox();
  boxMargin->setRange(0, 1000);
  boxMargin->setSingleStep(5);
  box4Layout->addWidget(boxMargin, 0, 1);

  boxAntialiasing = new QCheckBox(tr("Antialiasing"));
  box4Layout->addWidget(boxAntialiasing, 1, 1);

  boxAll = new QCheckBox(tr("Apply to all layers"));
  box4Layout->addWidget(boxAll, 2, 1);
  box4Layout->setRowStretch(3, 1);

  QHBoxLayout * hl = new QHBoxLayout(layerPage);
  hl->addWidget(boxBkg);
  hl->addWidget(box4);

  privateTabWidget->addTab(layerPage, tr("Layer"));
}

void PlotDialog::initLayerGeometryPage()
{
  layerGeometryPage = new QWidget();

  QGroupBox *gb1 = new QGroupBox(tr("Origin"));
  boxX = new QSpinBox();
  boxX->setRange(0, 2000);
  boxX->setSuffix(tr(" pixels"));

  boxY = new QSpinBox();
  boxY->setRange(0, 2000);
  boxY->setSuffix(tr(" pixels"));

  QGridLayout *gl1 = new QGridLayout(gb1);
  gl1->addWidget(new QLabel(tr("X= ")), 0, 0);
  gl1->addWidget(boxX, 0, 1);
  gl1->addWidget(new QLabel(tr("Y= ")), 1, 0);
  gl1->addWidget(boxY, 1, 1);
  gl1->setRowStretch(2, 1);

  QGroupBox *gb2 = new QGroupBox(tr("Size"));
  boxLayerWidth = new QSpinBox();
  boxLayerWidth->setRange(0, 2000);
  boxLayerWidth->setSuffix(tr(" pixels"));

  boxLayerHeight = new QSpinBox();
  boxLayerHeight->setRange(0, 2000);
  boxLayerHeight->setSuffix(tr(" pixels"));

  QGridLayout *gl2 = new QGridLayout(gb2);
  gl2->addWidget(new QLabel(tr("width= ")), 0, 0);
  gl2->addWidget(boxLayerWidth, 0, 1);

  gl2->addWidget(new QLabel(tr("height= ")), 2, 0);
  gl2->addWidget(boxLayerHeight, 2, 1);

  keepRatioBox = new QCheckBox(tr("Keep aspect ratio"));
  keepRatioBox->setChecked(true);
  gl2->addWidget(keepRatioBox, 3, 1);

  gl2->setRowStretch(4, 1);

  QBoxLayout *bl1 = new QBoxLayout(QBoxLayout::LeftToRight);
  bl1->addWidget(gb1);
  bl1->addWidget(gb2);

  keepRatioOnResizeBox = new QCheckBox(tr("Keep aspect ratio on window resize"));
  QBoxLayout *bl2 = new QBoxLayout(QBoxLayout::LeftToRight);
  bl2->addWidget(keepRatioOnResizeBox);

  QVBoxLayout * vl = new QVBoxLayout(layerGeometryPage);
  vl->addLayout(bl1);
  vl->addLayout(bl2);

  privateTabWidget->addTab(layerGeometryPage, tr("Geometry"));

  connect(boxLayerWidth, SIGNAL( valueChanged ( int ) ), this, SLOT( adjustLayerHeight(int) ));
  connect(boxLayerHeight, SIGNAL( valueChanged ( int ) ), this, SLOT( adjustLayerWidth(int) ));
}

void PlotDialog::initPiePage()
{
  piePage = new QWidget();

  QGridLayout *gl1 = new QGridLayout();
  gl1->addWidget(new QLabel(tr("Color")), 0, 0);

  boxPieLineColor = new ColorBox();
  gl1->addWidget(boxPieLineColor, 0, 1);

  gl1->addWidget(new QLabel(tr("Style")), 1, 0);
  boxPieLineStyle = new QComboBox();
  boxPieLineStyle->insertItem("_____");
  boxPieLineStyle->insertItem("- - -");
  boxPieLineStyle->insertItem(".....");
  boxPieLineStyle->insertItem("_._._");
  boxPieLineStyle->insertItem("_.._..");
  gl1->addWidget(boxPieLineStyle);

  gl1->addWidget(new QLabel(tr("Width")), 2, 0);
  boxPieLineWidth = new DoubleSpinBox('f');
  boxPieLineWidth->setSingleStep(0.1);
  boxPieLineWidth->setMinimum(0.0);
  boxPieLineWidth->setLocale(d_app->locale());
  gl1->addWidget(boxPieLineWidth, 2, 1);
  gl1->setRowStretch(3, 1);

  QGroupBox *gb1 = new QGroupBox(tr("Border"));
  gb1->setLayout(gl1);

  QGridLayout *gl2 = new QGridLayout();
  gl2->addWidget(new QLabel(tr("First color")), 0, 0);

  boxFirstColor = new ColorBox();
  gl2->addWidget(boxFirstColor, 0, 1);

  gl2->addWidget(new QLabel(tr("Pattern")), 1, 0);
  boxPiePattern = new PatternBox();
  gl2->addWidget(boxPiePattern, 1, 1);
  gl2->setRowStretch(2, 1);

  QGroupBox *gb2 = new QGroupBox(tr("Fill"));
  gb2->setLayout(gl2);

  QHBoxLayout* hl = new QHBoxLayout();
  hl->addWidget(gb1);
  hl->addWidget(gb2);
  piePage->setLayout(hl);

  privateTabWidget->addTab(piePage, tr("Pattern"));
}

void PlotDialog::initPieGeometryPage()
{
  pieGeometryPage = new QWidget();

  QLocale locale = d_app->locale();

  QGroupBox *gb3 = new QGroupBox(tr("3D View"));
  QGridLayout *gl3 = new QGridLayout(gb3);
  gl3->addWidget(new QLabel(tr("View Angle (deg)")), 0, 0);
  boxPieViewAngle = new DoubleSpinBox('f');
  boxPieViewAngle->setWrapping(true);
  boxPieViewAngle->setRange(0.0, 90.0);
  boxPieViewAngle->setLocale(locale);
  gl3->addWidget(boxPieViewAngle, 0, 1);

  gl3->addWidget(new QLabel(tr("Thickness (% of radius)")), 1, 0);
  boxPieThickness = new DoubleSpinBox('f');
  boxPieThickness->setLocale(locale);
  boxPieThickness->setRange(0.0, 300.0);
  gl3->addWidget(boxPieThickness, 1, 1);
  gl3->setRowStretch(2, 1);

  QGroupBox *gb1 = new QGroupBox(tr("Rotation"));
  QGridLayout *gl1 = new QGridLayout(gb1);
  gl1->addWidget(new QLabel(tr("Starting Azimuth (deg)")), 0, 0);
  boxPieStartAzimuth = new DoubleSpinBox('f');
  boxPieStartAzimuth->setRange(0.0, 360.0);
  boxPieStartAzimuth->setWrapping(true);
  boxPieStartAzimuth->setSingleStep(10.0);
  boxPieStartAzimuth->setLocale(locale);
  gl1->addWidget(boxPieStartAzimuth, 0, 1);

  boxPieConterClockwise = new QCheckBox(tr("Counter cloc&kwise"));
  gl1->addWidget(boxPieConterClockwise, 1, 0);
  gl1->setRowStretch(2, 1);

  QGroupBox *gb2 = new QGroupBox(tr("Radius/Center"));
  QGridLayout *gl2 = new QGridLayout(gb2);
  gl2->addWidget(new QLabel(tr("Radius (% of frame)")), 0, 0);
  boxRadius = new QSpinBox();
  boxRadius->setRange(0, 300);
  boxRadius->setSingleStep(5);
  gl2->addWidget(boxRadius, 0, 1);
  gl2->addWidget(new QLabel(tr("Horizontal Offset (% of frame)")), 1, 0);
  boxPieOffset = new QSpinBox();
  boxPieOffset->setRange(-100, 100);
  gl2->addWidget(boxPieOffset, 1, 1);
  gl2->setRowStretch(2, 1);

  QVBoxLayout* vl = new QVBoxLayout(pieGeometryPage);
  vl->addWidget(gb3);
  vl->addWidget(gb1);
  vl->addWidget(gb2);

  privateTabWidget->addTab(pieGeometryPage, tr("Pie Geometry"));
}

void PlotDialog::initPieLabelsPage()
{
  pieLabelsPage = new QWidget();

  pieAutoLabelsBox = new QGroupBox(tr("Automatic &Format"));
  pieAutoLabelsBox->setCheckable(true);

  QGridLayout *gl1 = new QGridLayout(pieAutoLabelsBox);
  boxPieValues = new QCheckBox(tr("&Values"));
  gl1->addWidget(boxPieValues, 0, 0);

  boxPiePercentages = new QCheckBox(tr("&Percentages"));
  gl1->addWidget(boxPiePercentages, 1, 0);

  boxPieCategories = new QCheckBox(tr("Categories/&Rows"));
  gl1->addWidget(boxPieCategories, 2, 0);

  gl1->setRowStretch(3, 1);

  boxPieWedge = new QGroupBox(tr("Associate Position with &Wedge"));
  boxPieWedge->setCheckable(true);

  QGridLayout *gl2 = new QGridLayout(boxPieWedge);
  gl2->addWidget(new QLabel(tr("Dist. from Pie Edge")), 0, 0);
  boxPieEdgeDist = new DoubleSpinBox('f');
  boxPieEdgeDist->setRange(-100, 100);
  boxPieEdgeDist->setLocale(d_app->locale());
  gl2->addWidget(boxPieEdgeDist, 0, 1);
  gl2->setRowStretch(1, 1);

  QVBoxLayout* vl = new QVBoxLayout(pieLabelsPage);
  vl->addWidget(pieAutoLabelsBox);
  vl->addWidget(boxPieWedge);

  privateTabWidget->addTab(pieLabelsPage, tr("Labels"));
}

void PlotDialog::initPrintPage()
{
  QGroupBox *gb = new QGroupBox();
  QVBoxLayout *vl = new QVBoxLayout(gb);
  boxScaleLayers = new QCheckBox(tr("&Scale layers to paper size"));
  vl->addWidget(boxScaleLayers);
  boxPrintCrops = new QCheckBox(tr("Print Crop&marks"));
  vl->addWidget(boxPrintCrops);
  vl->addStretch();

  printPage = new QWidget();
  QHBoxLayout* hlayout = new QHBoxLayout(printPage);
  hlayout->addWidget(gb);
  privateTabWidget->addTab(printPage, tr("Print"));
}

void PlotDialog::initLabelsPage()
{
  labelsGroupBox = new QGroupBox(tr("&Show"));
  labelsGroupBox->setCheckable(true);

  QGridLayout *gl = new QGridLayout(labelsGroupBox);
  labelsColumnLbl = new QLabel(tr("Column"));
  gl->addWidget(labelsColumnLbl, 0, 0);
  boxLabelsColumn = new QComboBox();
  gl->addWidget(boxLabelsColumn, 0, 1);

  gl->addWidget(new QLabel(tr("Color")), 1, 0);
  boxLabelsColor = new ColorBox();
  gl->addWidget(boxLabelsColor, 1, 1);
  boxLabelsWhiteOut = new QCheckBox(tr("White O&ut"));
  gl->addWidget(boxLabelsWhiteOut, 1, 2);

  justifyLabelsLbl = new QLabel(tr("Justify"));
  gl->addWidget(justifyLabelsLbl, 2, 0);
  boxLabelsAlign = new QComboBox();
  boxLabelsAlign->addItem(tr("Center"));
  boxLabelsAlign->addItem(tr("Left"));
  boxLabelsAlign->addItem(tr("Right"));
  gl->addWidget(boxLabelsAlign, 2, 1);

  gl->addWidget(new QLabel(tr("Rotate (deg)")), 3, 0);
  boxLabelsAngle = new DoubleSpinBox('f');
  boxLabelsAngle->setDecimals(1);
  boxLabelsAngle->setLocale(d_app->locale());
  boxLabelsAngle->setRange(0, 180);
  gl->addWidget(boxLabelsAngle, 3, 1);

  gl->addWidget(new QLabel(tr("X Offset (font height %)")), 4, 0);
  boxLabelsXOffset = new QSpinBox();
  boxLabelsXOffset->setRange(-INT_MAX, INT_MAX);
  boxLabelsXOffset->setSingleStep(10);
  gl->addWidget(boxLabelsXOffset, 4, 1);

  gl->addWidget(new QLabel(tr("Y Offset (font height %)")), 5, 0);
  boxLabelsYOffset = new QSpinBox();
  boxLabelsYOffset->setRange(-INT_MAX, INT_MAX);
  boxLabelsYOffset->setSingleStep(10);
  gl->addWidget(boxLabelsYOffset, 5, 1);
  gl->setRowStretch(6, 1);
  gl->setColumnStretch(3, 1);

  labelsPage = new QWidget();
  QHBoxLayout* hlayout = new QHBoxLayout(labelsPage);
  hlayout->addWidget(labelsGroupBox);
  privateTabWidget->addTab(labelsPage, tr("Labels"));
}

void PlotDialog::initAxesPage()
{
  QGroupBox *gb = new QGroupBox(tr("Attach curve to: "));
  QGridLayout *gl = new QGridLayout(gb);
  gl->addWidget(new QLabel(tr("x Axis")), 0, 0);
  boxXAxis = new QComboBox();
  boxXAxis->setEditable(false);
  boxXAxis->addItem(tr("Bottom"));
  boxXAxis->addItem(tr("Top"));
  gl->addWidget(boxXAxis, 0, 1);
  gl->addWidget(new QLabel(tr("y Axis")), 1, 0);
  boxYAxis = new QComboBox();
  boxYAxis->setEditable(false);
  boxYAxis->addItem(tr("Left"));
  boxYAxis->addItem(tr("Right"));
  gl->addWidget(boxYAxis, 1, 1);
  gl->setRowStretch(2, 1);

  axesPage = new QWidget();
  QHBoxLayout* hlayout = new QHBoxLayout(axesPage);
  hlayout->addWidget(gb);
  privateTabWidget->addTab(axesPage, tr("Axes"));
}

void PlotDialog::initLinePage()
{
  QGroupBox *gb = new QGroupBox();
  QGridLayout *gl1 = new QGridLayout(gb);
  gl1->addWidget(new QLabel(tr("Connect")), 0, 0);

  boxConnect = new QComboBox();
  boxConnect->setEditable(false);
  boxConnect->addItem(tr("No line"));
  boxConnect->addItem(tr("Lines"));
  boxConnect->addItem(tr("Sticks"));
  boxConnect->addItem(tr("Horizontal Steps"));
  boxConnect->addItem(tr("Dots"));
  boxConnect->addItem(tr("Spline"));
  boxConnect->addItem(tr("Vertical Steps"));
  gl1->addWidget(boxConnect, 0, 1);

  gl1->addWidget(new QLabel(tr("Style")), 1, 0);
  boxLineStyle = new QComboBox();
  boxLineStyle->setEditable(false);
  boxLineStyle->addItem("_____");
  boxLineStyle->addItem("_ _ _");
  boxLineStyle->addItem(".....");
  boxLineStyle->addItem("_._._");
  boxLineStyle->addItem("_.._..");
  gl1->addWidget(boxLineStyle, 1, 1);

  gl1->addWidget(new QLabel(tr("Width")), 2, 0);
  boxLineWidth = new DoubleSpinBox('f');
  boxLineWidth->setLocale(d_app->locale());
  boxLineWidth->setSingleStep(0.1);
  boxLineWidth->setMinimum(0.1);
  boxLineWidth->setValue(1);
  gl1->addWidget(boxLineWidth, 2, 1);

  gl1->addWidget(new QLabel(tr("Color")), 3, 0);
  boxLineColor = new ColorBox();
  gl1->addWidget(boxLineColor, 3, 1);
  gl1->setRowStretch(4, 1);

  fillGroupBox = new QGroupBox(tr("Fill area under curve"));
  fillGroupBox->setCheckable(true);
  QGridLayout *gl2 = new QGridLayout(fillGroupBox);
  gl2->addWidget(new QLabel(tr("Fill color")), 0, 0);
  boxAreaColor = new ColorBox();
  gl2->addWidget(boxAreaColor, 0, 1);
  gl2->addWidget(new QLabel(tr("Pattern")), 1, 0);
  boxPattern = new PatternBox();
  gl2->addWidget(boxPattern, 1, 1);
  gl2->setRowStretch(2, 1);

  linePage = new QWidget();
  QHBoxLayout* hlayout = new QHBoxLayout(linePage);
  hlayout->addWidget(gb);
  hlayout->addWidget(fillGroupBox);
  privateTabWidget->addTab(linePage, tr("Line"));

  connect(fillGroupBox, SIGNAL(toggled(bool)), this, SLOT(showAreaColor(bool)));
}

void PlotDialog::initSymbolsPage()
{
  QGroupBox *gb = new QGroupBox();
  QGridLayout *gl = new QGridLayout(gb);
  gl->addWidget(new QLabel(tr("Style")), 0, 0);
  boxSymbolStyle = new SymbolBox();
  gl->addWidget(boxSymbolStyle, 0, 1);
  gl->addWidget(new QLabel(tr("Size")), 1, 0);
  boxSymbolSize = new QSpinBox();
  boxSymbolSize->setRange(1, 100);
  boxSymbolSize->setValue(5);
  gl->addWidget(boxSymbolSize, 1, 1);
  boxFillSymbol = new QCheckBox(tr("Fill Color"));
  boxFillSymbol->setCheckable(true);
  gl->addWidget(boxFillSymbol, 2, 0);
  boxFillColor = new ColorBox();
  gl->addWidget(boxFillColor, 2, 1);
  gl->addWidget(new QLabel(tr("Edge Color")), 3, 0);
  boxSymbolColor = new ColorBox();
  gl->addWidget(boxSymbolColor, 3, 1);
  gl->addWidget(new QLabel(tr("Edge Width")), 4, 0);
  boxPenWidth = new DoubleSpinBox('f');
  boxPenWidth->setLocale(d_app->locale());
  boxPenWidth->setSingleStep(0.1);
  boxPenWidth->setRange(0.1, 100);
  gl->addWidget(boxPenWidth, 4, 1);
  gl->setRowStretch(5, 1);

  symbolPage = new QWidget();
  QHBoxLayout* hl = new QHBoxLayout(symbolPage);
  hl->addWidget(gb);

  privateTabWidget->insertTab(symbolPage, tr("Symbol"));

  connect(boxFillSymbol, SIGNAL(toggled(bool)), this, SLOT(showBoxSymbols(bool)));
}

void PlotDialog::initBoxPage()
{
  QGroupBox *gb1 = new QGroupBox(tr("Box"));
  QGridLayout *gl1 = new QGridLayout(gb1);
  gl1->addWidget(new QLabel(tr("Type")), 0, 0);

  boxType = new QComboBox();
  boxType->setEditable(false);
  boxType->addItem(tr("No Box"));
  boxType->addItem(tr("Rectangle"));
  boxType->addItem(tr("Diamond"));
  boxType->addItem(tr("Perc 10, 25, 75, 90"));
  boxType->addItem(tr("Notch"));
  gl1->addWidget(boxType, 0, 1);

  boxRangeLabel = new QLabel(tr("Range"));
  gl1->addWidget(boxRangeLabel, 1, 0);
  boxRange = new QComboBox();
  boxRange->setEditable(false);
  boxRange->addItem(tr("Standard Deviation"));
  boxRange->addItem(tr("Standard Error"));
  boxRange->addItem(tr("Perc 25, 75"));
  boxRange->addItem(tr("Perc 10, 90"));
  boxRange->addItem(tr("Perc 5, 95"));
  boxRange->addItem(tr("Perc 1, 99"));
  boxRange->addItem(tr("Max-Min"));
  boxRange->addItem(tr("Constant"));
  gl1->addWidget(boxRange, 1, 1);

  boxCoeffLabel = new QLabel(tr("Percentile (%)"));
  gl1->addWidget(boxCoeffLabel, 2, 0);
  boxCoef = new QSpinBox();
  boxCoef->setRange(50, 100);
  boxCoef->setSingleStep(5);
  gl1->addWidget(boxCoef, 2, 1);

  boxCntLabel = new QLabel(tr("Coefficient"));
  gl1->addWidget(boxCntLabel, 3, 0);
  boxCnt = new QDoubleSpinBox();
  boxCnt->setRange(0.0, 100.0);
  boxCnt->setSingleStep(0.01);
  boxCnt->setValue(1.0);
  gl1->addWidget(boxCnt, 3, 1);

  gl1->addWidget(new QLabel(tr("Box Width")), 4, 0);
  boxWidth = new QSpinBox();
  boxWidth->setRange(0, 100);
  boxWidth->setSingleStep(5);
  gl1->addWidget(boxWidth, 4, 1);

  QGroupBox *gb2 = new QGroupBox(tr("Whiskers"));
  QGridLayout *gl2 = new QGridLayout(gb2);
  whiskerRangeLabel = new QLabel(tr("Range"));
  gl2->addWidget(whiskerRangeLabel, 0, 0);

  boxWhiskersRange = new QComboBox();
  boxWhiskersRange->setEditable(false);
  boxWhiskersRange->addItem(tr("No Whiskers"));
  boxWhiskersRange->addItem(tr("Standard Deviation"));
  boxWhiskersRange->addItem(tr("Standard Error"));
  boxWhiskersRange->addItem(tr("75-25"));
  boxWhiskersRange->addItem(tr("90-10"));
  boxWhiskersRange->addItem(tr("95-5"));
  boxWhiskersRange->addItem(tr("99-1"));
  boxWhiskersRange->addItem(tr("Max-Min"));
  boxWhiskersRange->addItem(tr("Constant"));
  gl2->addWidget(boxWhiskersRange, 0, 1);

  whiskerCoeffLabel = new QLabel(tr("Percentile (%)"));
  gl2->addWidget(whiskerCoeffLabel, 1, 0);
  boxWhiskersCoef = new QSpinBox();
  boxWhiskersCoef->setRange(50, 100);
  boxWhiskersCoef->setSingleStep(5);
  gl2->addWidget(boxWhiskersCoef, 1, 1);

  whiskerCntLabel = new QLabel(tr("Coef"));
  gl2->addWidget(whiskerCntLabel, 2, 0);
  whiskerCnt = new QDoubleSpinBox();
  whiskerCnt->setRange(0.0, 100.0);
  whiskerCnt->setSingleStep(0.01);
  whiskerCnt->setValue(1.0);
  gl2->addWidget(whiskerCnt, 2, 1);

  QVBoxLayout *vl1 = new QVBoxLayout();
  vl1->addWidget(gb1);
  vl1->addStretch();

  QVBoxLayout *vl2 = new QVBoxLayout();
  vl2->addWidget(gb2);
  vl2->addStretch();

  boxPage = new QWidget();
  QHBoxLayout* hl = new QHBoxLayout(boxPage);
  hl->addLayout(vl1);
  hl->addLayout(vl2);
  privateTabWidget->insertTab(boxPage, tr("Box/Whiskers"));

  connect(boxType, SIGNAL(activated(int)), this, SLOT(setBoxType(int)));
  connect(boxRange, SIGNAL(activated(int)), this, SLOT(setBoxRangeType(int)));
  connect(boxWhiskersRange, SIGNAL(activated(int)), this, SLOT(setWhiskersRange(int)));
}

void PlotDialog::initPercentilePage()
{
  QGroupBox *gb1 = new QGroupBox(tr("Type"));
  QGridLayout *gl1 = new QGridLayout(gb1);
  gl1->addWidget(new QLabel(tr("Max")), 0, 0);

  boxMaxStyle = new SymbolBox();
  gl1->addWidget(boxMaxStyle, 0, 1);

  gl1->addWidget(new QLabel(tr("99%")), 1, 0);
  box99Style = new SymbolBox();
  gl1->addWidget(box99Style, 1, 1);

  gl1->addWidget(new QLabel(tr("Mean")), 2, 0);
  boxMeanStyle = new SymbolBox();
  gl1->addWidget(boxMeanStyle, 2, 1);

  gl1->addWidget(new QLabel(tr("1%")), 3, 0);
  box1Style = new SymbolBox();
  gl1->addWidget(box1Style, 3, 1);

  gl1->addWidget(new QLabel(tr("Min")), 4, 0);
  boxMinStyle = new SymbolBox();
  gl1->addWidget(boxMinStyle, 4, 1);
  gl1->setRowStretch(5, 1);

  QGroupBox *gb2 = new QGroupBox(tr("Symbol"));
  QGridLayout *gl2 = new QGridLayout(gb2);
  gl2->addWidget(new QLabel(tr("Size")), 0, 0);

  boxPercSize = new QSpinBox();
  boxPercSize->setMinValue(1);
  gl2->addWidget(boxPercSize, 0, 1);

  boxFillSymbols = new QCheckBox(tr("Fill Color"));
  boxFillSymbols->setCheckable(true);
  gl2->addWidget(boxFillSymbols, 1, 0);
  boxPercFillColor = new ColorBox();
  gl2->addWidget(boxPercFillColor, 1, 1);

  gl2->addWidget(new QLabel(tr("Edge Color")), 2, 0);
  boxEdgeColor = new ColorBox();
  gl2->addWidget(boxEdgeColor, 2, 1);

  gl2->addWidget(new QLabel(tr("Edge Width")), 3, 0);
  boxEdgeWidth = new DoubleSpinBox('f');
  boxEdgeWidth->setLocale(d_app->locale());
  boxEdgeWidth->setSingleStep(0.1);
  boxEdgeWidth->setRange(0, 100);
  gl2->addWidget(boxEdgeWidth, 3, 1);
  gl2->setRowStretch(4, 1);

  percentilePage = new QWidget();
  QHBoxLayout* hl = new QHBoxLayout(percentilePage);
  hl->addWidget(gb1);
  hl->addWidget(gb2);
  privateTabWidget->insertTab(percentilePage, tr("Percentile"));

  connect(boxFillSymbols, SIGNAL(toggled(bool)), this, SLOT(showBoxSymbols(bool)));
}

/*
 * Hides the "Custom color map" button when the user has not selected that specific button.
 */
void PlotDialog::showColorMapEditor(bool)
{
  if (grayScaleBox->isChecked() || defaultScaleBox->isChecked())
  {
    mSelectColormap->hide();
  }
  else
  {
    mSelectColormap->show();
  }
}

void PlotDialog::initSpectrogramPage()
{
  spectrogramPage = new QWidget();

  imageGroupBox = new QGroupBox(tr("Image"));
  imageGroupBox->setCheckable(true);

  QVBoxLayout *vl = new QVBoxLayout();

  grayScaleBox = new QRadioButton(tr("&Gray Scale"));
  connect(grayScaleBox, SIGNAL(toggled(bool)), this, SLOT(showColorMapEditor(bool)));
  vl->addWidget(grayScaleBox);

  defaultScaleBox = new QRadioButton(tr("&Default Color Map"));
  connect(defaultScaleBox, SIGNAL(toggled(bool)), this, SLOT(showColorMapEditor(bool)));
  vl->addWidget(defaultScaleBox);

  customScaleBox = new QRadioButton(tr("&Custom Color Map"));
  connect(customScaleBox, SIGNAL(toggled(bool)), this, SLOT(showColorMapEditor(bool)));
  vl->addWidget(customScaleBox);

  QHBoxLayout *hl = new QHBoxLayout(imageGroupBox);
  hl->addLayout(vl);

  mSelectColormap = new QPushButton(tr("Select ColorMap"));
  hl->addWidget(mSelectColormap);
  connect(mSelectColormap, SIGNAL(clicked()), this, SLOT(changeColormap()));

  axisScaleBox = new QGroupBox(tr("Color Bar Scale"));
  axisScaleBox->setCheckable(true);

  QGridLayout *gl2 = new QGridLayout(axisScaleBox);
  gl2->addWidget(new QLabel(tr("Axis")), 0, 0);

  colorScaleBox = new QComboBox();
  colorScaleBox->addItem(tr("Left"));
  colorScaleBox->addItem(tr("Right"));
  colorScaleBox->addItem(tr("Bottom"));
  colorScaleBox->addItem(tr("Top"));
  gl2->addWidget(colorScaleBox, 0, 1);
  gl2->addWidget(new QLabel(tr("Width")), 1, 0);
  colorScaleWidthBox = new QSpinBox();
  colorScaleWidthBox->setRange(2, 10000);
  gl2->addWidget(colorScaleWidthBox, 1, 1);

  QVBoxLayout* vl2 = new QVBoxLayout(spectrogramPage);
  vl2->addWidget(imageGroupBox);
  vl2->addWidget(axisScaleBox);
  vl2->addStretch();

  privateTabWidget->insertTab(spectrogramPage, tr("Contour") + " / " + tr("Image"));
}

void PlotDialog::showBoxSymbols(bool show)
{
  boxPercFillColor->setEnabled(show);
  boxFillColor->setEnabled(show);
}

void PlotDialog::initErrorsPage()
{
  QGroupBox *gb1 = new QGroupBox(tr("Direction"));

  QVBoxLayout* vl = new QVBoxLayout(gb1);
  plusBox = new QCheckBox(tr("Plus"));
  vl->addWidget(plusBox);
  minusBox = new QCheckBox(tr("Minus"));
  vl->addWidget(minusBox);
  xBox = new QCheckBox(tr("&X Error Bar"));
  vl->addWidget(xBox);
  vl->addWidget(xBox);
  vl->addStretch();

  QGroupBox *gb2 = new QGroupBox(tr("Style"));
  QGridLayout *gl = new QGridLayout(gb2);
  gl->addWidget(new QLabel(tr("Color")), 0, 0);

  colorBox = new ColorButton();
  gl->addWidget(colorBox, 0, 1);

  gl->addWidget(new QLabel(tr("Line Width")), 1, 0);
  widthBox = new DoubleSpinBox('f');
  widthBox->setLocale(d_app->locale());
  widthBox->setSingleStep(0.1);
  widthBox->setRange(0, 100);
  gl->addWidget(widthBox, 1, 1);

  gl->addWidget(new QLabel(tr("Cap Width")), 2, 0);
  capBox = new QComboBox();
  capBox->addItem(tr("8"));
  capBox->addItem(tr("10"));
  capBox->addItem(tr("12"));
  capBox->addItem(tr("16"));
  capBox->addItem(tr("20"));
  capBox->setEditable(true);
  gl->addWidget(capBox, 2, 1);

  throughBox = new QCheckBox(tr("Through Symbol"));
  gl->addWidget(throughBox, 3, 0);
  gl->setRowStretch(4, 1);

  errorsPage = new QWidget();
  QHBoxLayout* hl = new QHBoxLayout(errorsPage);
  hl->addWidget(gb1);
  hl->addWidget(gb2);
  privateTabWidget->insertTab(errorsPage, tr("Error Bars"));
}

void PlotDialog::initHistogramPage()
{
  QHBoxLayout* hl = new QHBoxLayout();
  automaticBox = new QCheckBox(tr("Automatic Binning"));
  hl->addWidget(automaticBox);
  hl->addStretch();
  buttonStatistics = new QPushButton(tr("&Show statistics"));
  hl->addWidget(buttonStatistics);

  GroupBoxH = new QGroupBox();
  QGridLayout *gl = new QGridLayout(GroupBoxH);
  gl->addWidget(new QLabel(tr("Bin Size")), 0, 0);
  binSizeBox = new QLineEdit();
  gl->addWidget(binSizeBox, 0, 1);
  gl->addWidget(new QLabel(tr("Begin")), 1, 0);
  histogramBeginBox = new QLineEdit();
  gl->addWidget(histogramBeginBox, 1, 1);
  gl->addWidget(new QLabel(tr("End")), 2, 0);
  histogramEndBox = new QLineEdit();
  gl->addWidget(histogramEndBox, 2, 1);

  histogramPage = new QWidget();
  QVBoxLayout* vl = new QVBoxLayout(histogramPage);
  vl->addLayout(hl);
  vl->addWidget(GroupBoxH);
  vl->addStretch();

  privateTabWidget->insertTab(histogramPage, tr("Histogram Data"));

  connect(automaticBox, SIGNAL(clicked()), this, SLOT(setAutomaticBinning()));
  connect(buttonStatistics, SIGNAL(clicked()), this, SLOT(showStatistics() ));
}

void PlotDialog::initSpacingPage()
{
  spacingPage = new QWidget();

  QGridLayout *gl = new QGridLayout(spacingPage);
  gl->addWidget(new QLabel(tr("Gap Between Bars (in %)")), 0, 0);
  gapBox = new QSpinBox();
  gapBox->setRange(0, 100);
  gapBox->setSingleStep(10);
  gl->addWidget(gapBox, 0, 1);
  gl->addWidget(new QLabel(tr("Offset (in %)")), 1, 0);
  offsetBox = new QSpinBox();
  offsetBox->setRange(-1000, 1000);
  offsetBox->setSingleStep(50);
  gl->addWidget(offsetBox, 1, 1);
  gl->setRowStretch(2, 1);

  privateTabWidget->insertTab(spacingPage, tr("Spacing"));
}

void PlotDialog::initVectPage()
{
  QGroupBox *gb1 = new QGroupBox();
  QGridLayout *gl1 = new QGridLayout(gb1);
  gl1->addWidget(new QLabel(tr("Color")), 0, 0);
  vectColorBox = new ColorBox();
  gl1->addWidget(vectColorBox, 0, 1);
  gl1->addWidget(new QLabel(tr("Line Width")), 1, 0);
  vectWidthBox = new DoubleSpinBox('f');
  vectWidthBox->setLocale(d_app->locale());
  vectWidthBox->setSingleStep(0.1);
  vectWidthBox->setRange(0, 100);
  gl1->addWidget(vectWidthBox, 1, 1);

  QGroupBox *gb2 = new QGroupBox(tr("Arrowheads"));
  QGridLayout *gl2 = new QGridLayout(gb2);
  gl2->addWidget(new QLabel(tr("Length")), 0, 0);
  headLengthBox = new QSpinBox();
  headLengthBox->setRange(0, 100);
  gl2->addWidget(headLengthBox, 0, 1);
  gl2->addWidget(new QLabel(tr("Angle")), 1, 0);
  headAngleBox = new QSpinBox();
  headAngleBox->setRange(0, 85);
  headAngleBox->setSingleStep(5);
  gl2->addWidget(headAngleBox, 1, 1);
  filledHeadBox = new QCheckBox(tr("&Filled"));
  gl2->addWidget(filledHeadBox, 2, 0);
  gl2->setRowStretch(3, 1);

  GroupBoxVectEnd = new QGroupBox(tr("End Point"));
  QGridLayout *gl3 = new QGridLayout(GroupBoxVectEnd);
  labelXEnd = new QLabel(tr("X End"));
  gl3->addWidget(labelXEnd, 0, 0);
  xEndBox = new QComboBox();
  gl3->addWidget(xEndBox, 0, 1);

  labelYEnd = new QLabel(tr("Y End"));
  gl3->addWidget(labelYEnd, 1, 0);
  yEndBox = new QComboBox();
  gl3->addWidget(yEndBox, 1, 1);

  labelPosition = new QLabel(tr("Position"));
  gl3->addWidget(labelPosition, 2, 0);
  vectPosBox = new QComboBox();
  vectPosBox->addItem(tr("Tail"));
  vectPosBox->addItem(tr("Middle"));
  vectPosBox->addItem(tr("Head"));
  gl3->addWidget(vectPosBox, 2, 1);
  gl3->setRowStretch(3, 1);

  vectPage = new QWidget();

  QVBoxLayout *vl1 = new QVBoxLayout();
  vl1->addWidget(gb1);
  vl1->addWidget(gb2);

  QHBoxLayout *hl = new QHBoxLayout(vectPage);
  hl->addLayout(vl1);
  hl->addWidget(GroupBoxVectEnd);

  privateTabWidget->insertTab(vectPage, tr("Vector"));
}

void PlotDialog::setMultiLayer(MultiLayer *ml)
{
  if (!ml)
    return;

  d_ml = ml;
  boxScaleLayers->setChecked(d_ml->scaleLayersOnPrint());
  boxPrintCrops->setChecked(d_ml->printCropmarksEnabled());

  // the plot (dataset) name will be displayed in the tree entry (leftmost/topmost tree level)
  QTreeWidgetItem *item = new QTreeWidgetItem(listBox, QStringList(ml->name()));
  item->setIcon(0, QIcon(getQPixmap("folder_open")));
  listBox->addTopLevelItem(item);
  listBox->setCurrentItem(item);

  QList<Graph *> layers = ml->layersList();
  int i = 0;
  foreach(Graph *g, layers)
  {
    // builds the names/labels of layers (Layer1, Layer2, etc.) visible in the tree
    LayerItem *layer = new LayerItem(g, item, tr("Layer ") + QString::number(++i));
    item->addChild(layer);

    if (g == ml->activeGraph())
    {
      layer->setExpanded(true);
      layer->setActive(true);
      listBox->setCurrentItem(layer);

      keepRatioOnResizeBox->setChecked(g->isFixedAspectRatioEnabled());
      if (g->isSpectrogram()) keepRatioOnResizeBox->show();
      else keepRatioOnResizeBox->hide();
    }
  }
}

void PlotDialog::selectCurve(int index)
{
  LayerItem *layerItem = dynamic_cast<LayerItem*>(listBox->currentItem());
  if (!layerItem)
    return;
  if (layerItem->type() != LayerItem::LayerTreeItem)
    return;
  QTreeWidgetItem *item = layerItem->child(index);
  if (item)
  {
    (dynamic_cast<CurveTreeItem*>(item))->setActive(true);
    listBox->setCurrentItem(item);
  }
}

void PlotDialog::showStatistics()
{
  if (!d_app)
    return;

  QTreeWidgetItem *it = listBox->currentItem();
  if (!it)
    return;
  if (it->type() != CurveTreeItem::PlotCurveTreeItem)
    return;

  QwtPlotItem *plotItem = dynamic_cast<QwtPlotItem *>(dynamic_cast<CurveTreeItem*>(it)->plotItem());
  if (!plotItem)
    return;

  QwtHistogram *h = dynamic_cast<QwtHistogram*>(plotItem);
  if (!h)
    return;

  QString tableName = d_app->generateUniqueName(tr("Bins"));
  Table *t = d_app->newTable(h->dataSize(), 4, tableName,
      tr("Histogram and Probabilities for") + " " + h->title().text());
  if (t)
  {
    double h_sum = 0.0;
    for (int i = 0; i < h->dataSize(); i++)
      h_sum += h->y(i);

    double sum = 0.0;
    for (int i = 0; i < h->dataSize(); i++)
    {
      sum += h->y(i);
      t->setText(i, 0, QString::number(h->x(i)));
      t->setText(i, 1, QString::number(h->y(i)));
      t->setText(i, 2, QString::number(sum));
      t->setText(i, 3, QString::number(sum / h_sum * 100));
    }
    t->setHeader(QStringList() << tr("Bins") << tr("Quantity") << tr("Sum") << tr("Percent"));
    t->showMaximized();
  }

  QDateTime dt = QDateTime::currentDateTime();
  QString info = dt.toString(Qt::LocalDate) + "\t" + tr("Histogram and Probabilities for") + " "
      + h->title().text() + "\n";
  info += tr("Mean") + " = " + QString::number(h->mean()) + "\t";
  info += tr("Standard Deviation") + " = " + QString::number(h->standardDeviation()) + "\n";
  info += tr("Minimum") + " = " + QString::number(h->minimum()) + "\t";
  info += tr("Maximum") + " = " + QString::number(h->maximum()) + "\t";
  info += tr("Bins") + " = " + QString::number(h->dataSize()) + "\n";
  info += "-------------------------------------------------------------\n";
  if (!info.isEmpty())
  {
    d_app->currentFolder()->appendLogInfo(info);
    d_app->showResults(true);
  }

  close();
}

void PlotDialog::contextMenuEvent(QContextMenuEvent *e)
{
  QTreeWidgetItem *item = listBox->currentItem();
  if (!item)
    return;
  if (item->type() != CurveTreeItem::PlotCurveTreeItem)
    return;
  QwtPlotItem *it = dynamic_cast<QwtPlotItem *>(dynamic_cast<CurveTreeItem*>(item)->plotItem());
  if (!it)
    return;

  QPoint pos = listBox->viewport()->mapFromGlobal(QCursor::pos());
  QRect rect = listBox->visualItemRect(listBox->currentItem());
  if (rect.contains(pos))
  {
    QMenu contextMenu(this);
    contextMenu.insertItem(tr("&Delete"), this, SLOT(removeSelectedCurve()));

    if (it->rtti() == QwtPlotItem::Rtti_PlotCurve)
    {
      if (dynamic_cast<PlotCurve *>(it)->type() == Graph::Function)
        contextMenu.insertItem(tr("&Edit..."), this, SLOT(editCurve()));
      else
        contextMenu.insertItem(tr("&Plot Associations..."), this, SLOT(editCurve()));
    }
    contextMenu.exec(QCursor::pos());
  }
  e->accept();
}

void PlotDialog::removeSelectedCurve()
{
  CurveTreeItem *item = dynamic_cast<CurveTreeItem*>(listBox->currentItem());
  if (!item)
    return;
  if (item->type() != CurveTreeItem::PlotCurveTreeItem)
    return;

  Graph *graph = item->graph();
  if (graph)
  {
    graph->removeCurve(item->plotItemIndex());
    graph->updatePlot();

    int index = item->parent()->indexOfChild(item);
    QTreeWidgetItem *it = item->parent()->takeChild(index);
    if (it)
      delete it;
  }
}

void PlotDialog::showAreaColor(bool show)
{
  boxAreaColor->setEnabled(show);
  boxPattern->setEnabled(show);
}

void PlotDialog::updateTabWindow(QTreeWidgetItem *currentItem, QTreeWidgetItem *previousItem)
{
  if (!currentItem)
    return;

  bool forceClearTabs = false;
  if (!previousItem || previousItem == currentItem)
  {
    previousItem = currentItem;
    forceClearTabs = true;
  }

  if (previousItem->type() == CurveTreeItem::PlotCurveTreeItem)
    dynamic_cast<CurveTreeItem *>(previousItem)->setActive(false);
  else if (previousItem->type() == LayerItem::LayerTreeItem)
    dynamic_cast<LayerItem *>(previousItem)->setActive(false);

  boxPlotType->blockSignals(true);

  if (currentItem->type() == CurveTreeItem::PlotCurveTreeItem)
  {
    CurveTreeItem *curveItem = dynamic_cast<CurveTreeItem *>(currentItem);
    if (previousItem->type() != CurveTreeItem::PlotCurveTreeItem
        || dynamic_cast<CurveTreeItem *>(previousItem)->plotItemType() != curveItem->plotItemType()
        || forceClearTabs)
    {
      clearTabWidget();
      int plot_type = setPlotType(curveItem);
      if (plot_type >= 0)
      {
        insertTabs(plot_type);
      }
      if (!curvePlotTypeBox->isVisible())
      {
        curvePlotTypeBox->show();
      }
    }
    setActiveCurve(curveItem);
  }
  else if (currentItem->type() == LayerItem::LayerTreeItem)
  {
    if (previousItem->type() != LayerItem::LayerTreeItem)
    {
      clearTabWidget();
      privateTabWidget->addTab(layerPage, tr("Layer"));
      privateTabWidget->addTab(layerGeometryPage, tr("Geometry"));
      privateTabWidget->showPage(layerPage);
    }
    setActiveLayer(dynamic_cast<LayerItem*>(currentItem));
  }
  else
  {
    clearTabWidget();
    privateTabWidget->addTab(printPage, tr("Print"));
    privateTabWidget->addTab(fontsPage, tr("Fonts"));
    privateTabWidget->showPage(printPage);

    curvePlotTypeBox->hide();
    btnWorksheet->hide();
    btnEditCurve->hide();
  }
  boxPlotType->blockSignals(false);
}

void PlotDialog::insertTabs(int plot_type)
{
  if (plot_type == Graph::Pie)
  {
    privateTabWidget->addTab(piePage, tr("Pattern"));
    privateTabWidget->addTab(pieGeometryPage, tr("Pie Geometry"));
    privateTabWidget->addTab(pieLabelsPage, tr("Labels"));
    privateTabWidget->showPage(piePage);
    return;
  }

  privateTabWidget->addTab(axesPage, tr("Axes"));
  if (plot_type == Graph::Line)
  {
    boxConnect->setEnabled(true);
    privateTabWidget->addTab(linePage, tr("Line"));
    privateTabWidget->showPage(linePage);
  }
  else if (plot_type == Graph::Scatter)
  {
    boxConnect->setEnabled(true);
    privateTabWidget->addTab(symbolPage, tr("Symbol"));
    privateTabWidget->showPage(symbolPage);
  }
  else if (plot_type == Graph::LineSymbols)
  {
    boxConnect->setEnabled(true);
    privateTabWidget->addTab(linePage, tr("Line"));
    privateTabWidget->addTab(symbolPage, tr("Symbol"));
    privateTabWidget->showPage(symbolPage);
  }
  else if (plot_type == Graph::VerticalBars || plot_type == Graph::HorizontalBars
      || plot_type == Graph::Histogram)
  {
    boxConnect->setEnabled(false);
    privateTabWidget->addTab(linePage, tr("Pattern"));
    privateTabWidget->addTab(spacingPage, tr("Spacing"));

    if (plot_type == Graph::Histogram)
    {
      privateTabWidget->addTab(histogramPage, tr("Histogram Data"));
      privateTabWidget->showPage(histogramPage);
    }
    else
      privateTabWidget->showPage(linePage);
  }
  else if (plot_type == Graph::VectXYXY || plot_type == Graph::VectXYAM)
  {
    boxConnect->setEnabled(true);
    privateTabWidget->addTab(linePage, tr("Line"));
    privateTabWidget->addTab(vectPage, tr("Vector"));
    customVectorsPage(plot_type == Graph::VectXYAM);
    privateTabWidget->showPage(vectPage);
  }
  else if (plot_type == Graph::ErrorBars)
  {
    privateTabWidget->addTab(errorsPage, tr("Error Bars"));
    privateTabWidget->showPage(errorsPage);
  }
  else if (plot_type == Graph::Box)
  {
    boxConnect->setEnabled(false);
    privateTabWidget->addTab(linePage, tr("Pattern"));
    privateTabWidget->addTab(boxPage, tr("Box/Whiskers"));
    privateTabWidget->addTab(percentilePage, tr("Percentile"));
    privateTabWidget->showPage(linePage);
    return;
  }
  else if (plot_type == Graph::ColorMap || plot_type == Graph::GrayScale || plot_type == Graph::Contour
      || plot_type == Graph::ColorMapContour)
  {
    privateTabWidget->addTab(spectrogramPage, tr("Colors"));
    privateTabWidget->addTab(contourLinesPage, tr("Contour Lines"));
    privateTabWidget->showPage(spectrogramPage);
    privateTabWidget->addTab(labelsPage, tr("Labels"));
    return;
  }

  QTreeWidgetItem *item = listBox->currentItem();
  if (!item || item->type() != CurveTreeItem::PlotCurveTreeItem)
    return;

  const DataCurve *c = dynamic_cast<const DataCurve *>((dynamic_cast<CurveTreeItem*>(item))->plotItem());
  if (!c)
    return;
  if (c && c->type() != Graph::Function)
  {
    privateTabWidget->addTab(labelsPage, tr("Labels"));
    if (c->hasSelectedLabels())
      privateTabWidget->showPage(labelsPage);
  }
}

void PlotDialog::clearTabWidget()
{
  privateTabWidget->removeTab(privateTabWidget->indexOf(labelsPage));
  privateTabWidget->removeTab(privateTabWidget->indexOf(axesPage));
  privateTabWidget->removeTab(privateTabWidget->indexOf(linePage));
  privateTabWidget->removeTab(privateTabWidget->indexOf(symbolPage));
  privateTabWidget->removeTab(privateTabWidget->indexOf(errorsPage));
  privateTabWidget->removeTab(privateTabWidget->indexOf(histogramPage));
  privateTabWidget->removeTab(privateTabWidget->indexOf(spacingPage));
  privateTabWidget->removeTab(privateTabWidget->indexOf(vectPage));
  privateTabWidget->removeTab(privateTabWidget->indexOf(boxPage));
  privateTabWidget->removeTab(privateTabWidget->indexOf(percentilePage));
  privateTabWidget->removeTab(privateTabWidget->indexOf(spectrogramPage));
  privateTabWidget->removeTab(privateTabWidget->indexOf(piePage));
  privateTabWidget->removeTab(privateTabWidget->indexOf(pieGeometryPage));
  privateTabWidget->removeTab(privateTabWidget->indexOf(pieLabelsPage));
  privateTabWidget->removeTab(privateTabWidget->indexOf(layerPage));
  privateTabWidget->removeTab(privateTabWidget->indexOf(layerGeometryPage));
  privateTabWidget->removeTab(privateTabWidget->indexOf(fontsPage));
  privateTabWidget->removeTab(privateTabWidget->indexOf(printPage));
  privateTabWidget->removeTab(privateTabWidget->indexOf(contourLinesPage));
}

void PlotDialog::quit()
{
  if (acceptParams())
    close();
}

void PlotDialog::showWorksheet()
{
  if (!d_app)
    return;

  CurveTreeItem *item = dynamic_cast<CurveTreeItem*>(listBox->currentItem());
  if (!item)
    return;
  if (item->type() != CurveTreeItem::PlotCurveTreeItem)
    return;

  d_app->showCurveWorksheet(item->graph(), item->plotItemIndex());
  close();
}

int PlotDialog::setPlotType(CurveTreeItem *item)
{
  int curveType = item->plotItemType();
  if (curveType >= 0)
  {
    boxPlotType->clear();

    if (curveType == Graph::ErrorBars)
      boxPlotType->addItem(tr("Error Bars"));
    else if (curveType == Graph::Pie)
      boxPlotType->addItem(tr("Pie"));
    else if (curveType == Graph::VerticalBars)
      boxPlotType->addItem(tr("Vertical Bars"));
    else if (curveType == Graph::HorizontalBars)
      boxPlotType->addItem(tr("Horizontal Bars"));
    else if (curveType == Graph::Histogram)
      boxPlotType->addItem(tr("Histogram"));
    else if (curveType == Graph::VectXYXY || curveType == Graph::VectXYAM)
    {
      boxPlotType->addItem(tr("Vector XYXY"));
      boxPlotType->addItem(tr("Vector XYAM"));
      if (curveType == Graph::VectXYAM)
        boxPlotType->setCurrentIndex(1);
    }
    else if (curveType == Graph::Box)
      boxPlotType->addItem(tr("Box"));
    else if (curveType == Graph::ColorMap || curveType == Graph::GrayScale || curveType == Graph::Contour
        || curveType == Graph::ColorMapContour)
      boxPlotType->insertItem(tr("Contour") + " / " + tr("Image"));
    else
    {
      boxPlotType->addItem(tr("Line"));
      boxPlotType->addItem(tr("Scatter"));
      boxPlotType->addItem(tr("Line + Symbol"));

      QwtPlotCurve *c = dynamic_cast<QwtPlotCurve*>(item->plotItem());
      if (!c)
        return -1;

      QwtSymbol s = c->symbol();
      if (s.style() == QwtSymbol::NoSymbol)
      {
        boxPlotType->setCurrentIndex(0);
        return Graph::Line;
      }
      else if (c->style() == QwtPlotCurve::NoCurve)
      {
        boxPlotType->setCurrentIndex(1);
        return Graph::Scatter;
      }
      else
      {
        boxPlotType->setCurrentIndex(2);
        return Graph::LineSymbols;
      }
    }
  }
  return curveType;
}

void PlotDialog::setActiveLayer(LayerItem *item)
{
  if (!item)
    return;
  item->setActive(true);

  Graph *g = item->graph();
  if (!g)
    return;

  curvePlotTypeBox->hide();
  btnWorksheet->hide();
  btnEditCurve->hide();

  boxBackgroundTransparency->blockSignals(true);
  boxCanvasTransparency->blockSignals(true);
  boxBorderWidth->blockSignals(true);

  Plot *p = g->plotWidget();
  boxMargin->setValue(p->margin());
  boxBorderWidth->setValue(p->lineWidth());
  boxBorderColor->setColor(p->frameColor());

  QColor c = p->paletteBackgroundColor();
  boxBackgroundTransparency->setValue(c.alpha());
  boxBackgroundColor->setEnabled(c.alpha());
  c.setAlpha(255);
  boxBackgroundColor->setColor(c);

  c = p->canvasBackground();
  boxCanvasTransparency->setValue(c.alpha());
  boxCanvasColor->setEnabled(c.alpha());
  c.setAlpha(255);
  boxCanvasColor->setColor(c);

  boxAntialiasing->setChecked(g->antialiasing());

  boxBackgroundTransparency->blockSignals(false);
  boxCanvasTransparency->blockSignals(false);
  boxBorderWidth->blockSignals(false);

  boxCanvasFrameColor->setColor(g->canvasFrameColor());
  boxCanvasFrameWidth->setValue(g->canvasFrameWidth());

  boxX->setValue(g->pos().x());
  boxY->setValue(g->pos().y());

  boxLayerWidth->blockSignals(true);
  boxLayerWidth->setValue(p->size().width());
  boxLayerWidth->blockSignals(false);

  boxLayerHeight->blockSignals(true);
  boxLayerHeight->setValue(p->size().height());
  boxLayerHeight->blockSignals(false);

  aspect_ratio = (double) p->size().width() / (double) p->size().height();
}
void PlotDialog::updateContourLevelsDisplay(Spectrogram *sp)
{
  QwtValueList levels = sp->contourLevels();
  levelsBox->setValue(levels.size());
  if (levels.size() >= 1)
    firstContourLineBox->setValue(levels[0]);
  if (levels.size() >= 2)
    contourLinesDistanceBox->setValue(fabs(levels[1] - levels[0]));
}
void PlotDialog::setActiveCurve(CurveTreeItem *item)
{
  if (!item)
    return;

  QwtPlotItem *i = item->plotItem();
  if (!i)
    return;

  item->setActive(true);
  btnWorksheet->show();
  btnEditCurve->show();

  //axes page
  boxXAxis->setCurrentIndex(i->xAxis() - 2);
  boxYAxis->setCurrentIndex(i->yAxis());

  if (i->rtti() == QwtPlotItem::Rtti_PlotSpectrogram)
  {

    btnEditCurve->hide();
    Spectrogram *sp = dynamic_cast<Spectrogram *>(i);

    imageGroupBox->setChecked(sp->testDisplayMode(QwtPlotSpectrogram::ImageMode));
    grayScaleBox->setChecked(sp->colorMapPolicy() == Spectrogram::GrayScale);
    defaultScaleBox->setChecked(sp->colorMapPolicy() == Spectrogram::Default);
    customScaleBox->setChecked(sp->colorMapPolicy() == Spectrogram::Custom);

    //colorMapEditor->setRange(sp->data().range().minValue(), sp->data().range().maxValue());
    //colorMapEditor->setColorMap((const QwtLinearColorMap &)sp->colorMap());

    levelsGroupBox->setChecked(sp->testDisplayMode(QwtPlotSpectrogram::ContourMode));
    levelsBox->setValue(sp->levels());

    //autoContourBox->setChecked(sp->defaultContourPen().style() == Qt::NoPen);
    autoContourBox->setChecked(sp->useColorMapPen() && sp->defaultContourPen().style() == Qt::NoPen);
    //customPenBtn->setChecked(sp->defaultContourPen().style() == Qt::NoPen);
    customPenBtn->setChecked(!sp->useColorMapPen() && sp->defaultContourPen().style() == Qt::NoPen);
    defaultContourBox->setChecked(
        (sp->defaultContourPen().style() != Qt::NoPen) && !(sp->useColorMapPen()));

    levelsColorBox->setColor(sp->defaultContourPen().color());
    contourWidthBox->setValue(sp->defaultContourPen().widthF());

    if (sp->defaultContourPen().style() != Qt::NoPen)
      boxContourStyle->setCurrentIndex(sp->defaultContourPen().style() - 1);
    else
      boxContourStyle->setCurrentIndex(0);
    contourLinesEditor->setSpectrogram(sp);
    updateContourLevelsDisplay(sp);

    axisScaleBox->setChecked(sp->hasColorScale());
    colorScaleBox->setCurrentItem((int) sp->colorScaleAxis());
    colorScaleWidthBox->setValue(sp->colorBarWidth());

    //labels page

    showAllLabelControls(false);

    labelsGroupBox->blockSignals(true);
    labelsGroupBox->setEnabled(sp->testDisplayMode(QwtPlotSpectrogram::ContourMode));
    labelsGroupBox->setChecked(sp->hasLabels() && sp->testDisplayMode(QwtPlotSpectrogram::ContourMode));
    boxLabelsColor->setColor(sp->labelsColor());
    boxLabelsAngle->setValue(sp->labelsRotation());

    boxLabelsXOffset->blockSignals(true);
    boxLabelsXOffset->setValue(qRound(sp->labelsXOffset()));
    boxLabelsXOffset->blockSignals(false);

    boxLabelsYOffset->blockSignals(true);
    boxLabelsYOffset->setValue(qRound(sp->labelsYOffset()));
    boxLabelsYOffset->blockSignals(false);
    boxLabelsWhiteOut->setChecked(sp->labelsWhiteOut());
    labelsGroupBox->blockSignals(false);

    if (sp->hasSelectedLabels())
      privateTabWidget->showPage(labelsPage);
    return;
  }
  PlotCurve *c = dynamic_cast<PlotCurve*>(i);
  if (c->type() == Graph::Function)
    btnEditCurve->setText(tr("&Edit..."));
  else
    btnEditCurve->setText(tr("&Plot Associations..."));

  int curveType = item->plotItemType();
  if (curveType == Graph::Pie)
  {
    QwtPieCurve *pie = dynamic_cast<QwtPieCurve*>(i);
    boxPiePattern->setPattern(pie->pattern());
    boxPieLineWidth->setValue(pie->pen().widthF());
    boxPieLineColor->setColor(pie->pen().color());
    setPiePenStyle(pie->pen().style());
    boxFirstColor->setCurrentIndex(pie->firstColor());

    boxPieViewAngle->blockSignals(true);
    boxPieViewAngle->setValue(pie->viewAngle());
    boxPieViewAngle->blockSignals(false);
    boxPieThickness->blockSignals(true);
    boxPieThickness->setValue(pie->thickness());
    boxPieThickness->blockSignals(false);
    boxPieStartAzimuth->blockSignals(true);
    boxPieStartAzimuth->setValue(pie->startAzimuth());
    boxPieStartAzimuth->blockSignals(false);
    boxPieConterClockwise->blockSignals(true);
    boxPieConterClockwise->setChecked(pie->counterClockwise());
    boxPieConterClockwise->blockSignals(false);
    boxRadius->blockSignals(true);
    boxRadius->setValue(pie->radius());
    boxRadius->blockSignals(false);
    boxPieOffset->blockSignals(true);
    boxPieOffset->setValue((int) pie->horizontalOffset());
    boxPieOffset->blockSignals(false);

    pieAutoLabelsBox->setChecked(pie->labelsAutoFormat());
    boxPieValues->setChecked(pie->labelsValuesFormat());
    boxPiePercentages->setChecked(pie->labelsPercentagesFormat());
    boxPieCategories->setChecked(pie->labelCategories());
    boxPieEdgeDist->blockSignals(true);
    boxPieEdgeDist->setValue(pie->labelsEdgeDistance());
    boxPieEdgeDist->blockSignals(false);
    boxPieWedge->setChecked(pie->fixedLabelsPosition());
    return;
  }
  //line page
  int style = c->style();
  if (curveType == Graph::Spline)
    style = 5;
  else if (curveType == Graph::VerticalSteps)
    style = 6;
  boxConnect->setCurrentIndex(style);
  setPenStyle(c->pen().style());
  boxLineColor->setColor(c->pen().color());
  boxLineWidth->blockSignals(true);
  boxLineWidth->setValue(c->pen().widthF());
  boxLineWidth->blockSignals(false);
  fillGroupBox->blockSignals(true);
  fillGroupBox->setChecked(c->brush().style() != Qt::NoBrush);
  fillGroupBox->blockSignals(false);
  boxAreaColor->setColor(c->brush().color());

  //symbol page
  const QwtSymbol s = c->symbol();
  boxSymbolSize->setValue(s.size().width() / 2);
  boxSymbolStyle->setStyle(s.style());
  boxSymbolColor->setColor(s.pen().color());
  boxPenWidth->setValue(s.pen().widthF());
  boxFillSymbol->setChecked(s.brush() != Qt::NoBrush);
  boxFillColor->setEnabled(s.brush() != Qt::NoBrush);
  boxFillColor->setColor(s.brush().color());

  if (c->type() == Graph::Function)
    return;

  if (curveType == Graph::VerticalBars || curveType == Graph::HorizontalBars
      || curveType == Graph::Histogram)
  {        //spacing page
    QwtBarCurve *b = dynamic_cast<QwtBarCurve*>(i);
    if (b)
    {
      gapBox->setValue(b->gap());
      offsetBox->setValue(b->offset());
    }
  }

  if (curveType == Graph::Histogram)
  {        //Histogram page
    QwtHistogram *h = dynamic_cast<QwtHistogram*>(i);
    if (h)
    {
      automaticBox->setChecked(h->autoBinning());
      binSizeBox->setText(QString::number(h->binSize()));
      histogramBeginBox->setText(QString::number(h->begin()));
      histogramEndBox->setText(QString::number(h->end()));
      setAutomaticBinning();
    }
  }

  if (curveType == Graph::VectXYXY || curveType == Graph::VectXYAM)
  {        //Vector page
    VectorCurve *v = dynamic_cast<VectorCurve*>(i);
    if (v)
    {
      vectColorBox->setColor(v->color());
      vectWidthBox->setValue(v->width());
      headLengthBox->setValue(v->headLength());
      headAngleBox->setValue(v->headAngle());
      filledHeadBox->setChecked(v->filledArrowHead());
      vectPosBox->setCurrentIndex(v->position());
      updateEndPointColumns(item->text(0));
    }
  }

  if (curveType == Graph::ErrorBars)
  {
    QwtErrorPlotCurve *err = dynamic_cast<QwtErrorPlotCurve*>(i);
    if (err)
    {
      widthBox->blockSignals(true);
      widthBox->setValue(err->width());
      widthBox->blockSignals(false);
      capBox->setEditText(QString::number(err->capLength()));

      colorBox->blockSignals(true);
      colorBox->setColor(err->color());
      colorBox->blockSignals(false);

      throughBox->blockSignals(true);
      throughBox->setChecked(err->throughSymbol());
      throughBox->blockSignals(false);
      plusBox->blockSignals(true);
      plusBox->setChecked(err->plusSide());
      plusBox->blockSignals(false);
      minusBox->blockSignals(true);
      minusBox->setChecked(err->minusSide());
      minusBox->blockSignals(false);
      xBox->blockSignals(true);
      xBox->setChecked(err->xErrors());
      xBox->blockSignals(false);
    }
    return;
  }

  if (curveType == Graph::Box)
  {
    BoxCurve *b = dynamic_cast<BoxCurve*>(i);
    if (b)
    {
      boxMaxStyle->setStyle(b->maxStyle());
      boxMinStyle->setStyle(b->minStyle());
      boxMeanStyle->setStyle(b->meanStyle());
      box99Style->setStyle(b->p99Style());
      box1Style->setStyle(b->p1Style());

      boxPercSize->setValue(s.size().width() / 2);
      boxFillSymbols->setChecked(s.brush() != Qt::NoBrush);
      boxPercFillColor->setEnabled(s.brush() != Qt::NoBrush);
      boxPercFillColor->setColor(s.brush().color());
      boxEdgeColor->setColor(s.pen().color());
      boxEdgeWidth->setValue(s.pen().widthF());

      boxRange->setCurrentIndex(b->boxRangeType() - 1);
      boxType->setCurrentIndex(b->boxStyle());
      boxWidth->setValue(b->boxWidth());
      setBoxRangeType(boxRange->currentIndex());
      setBoxType(boxType->currentIndex());
      if (b->boxRangeType() == BoxCurve::SD || b->boxRangeType() == BoxCurve::SE)
        boxCnt->setValue(b->boxRange());
      else
        boxCoef->setValue((int) b->boxRange());

      boxWhiskersRange->setCurrentIndex(b->whiskersRangeType());
      setWhiskersRange(boxWhiskersRange->currentIndex());
      if (b->whiskersRangeType() == BoxCurve::SD || b->whiskersRangeType() == BoxCurve::SE)
        whiskerCnt->setValue(b->whiskersRange());
      else
        boxWhiskersCoef->setValue((int) b->whiskersRange());
    }
    return;
  }

  const DataCurve *dc = dynamic_cast<const DataCurve *>(i);
  if (!dc)
    return;
  if (!dc->table())
  {
    privateTabWidget->removeTab(privateTabWidget->indexOf(labelsPage));
    return;
  }
  labelsGroupBox->blockSignals(true);
  labelsGroupBox->setChecked(dc->hasLabels());

  QStringList cols = dc->table()->columnsList();
  boxLabelsColumn->blockSignals(true);
  boxLabelsColumn->clear();
  boxLabelsColumn->addItems(cols);
  int labelsColIndex = cols.indexOf(dc->labelsColumnName());
  if (labelsColIndex >= 0)
    boxLabelsColumn->setCurrentIndex(labelsColIndex);
  boxLabelsColumn->blockSignals(false);

  boxLabelsAngle->setValue(dc->labelsRotation());
  boxLabelsColor->setColor(dc->labelsColor());
  boxLabelsXOffset->setValue(dc->labelsXOffset());
  boxLabelsYOffset->setValue(dc->labelsYOffset());
  boxLabelsWhiteOut->setChecked(dc->labelsWhiteOut());
  switch (dc->labelsAlignment())
  {
  case Qt::AlignHCenter:
    boxLabelsAlign->setCurrentIndex(0);
    break;
  case Qt::AlignLeft:
    boxLabelsAlign->setCurrentIndex(1);
    break;
  case Qt::AlignRight:
    boxLabelsAlign->setCurrentIndex(2);
    break;
  }
  labelsGroupBox->blockSignals(false);
}
void PlotDialog::showAllLabelControls(bool show)
{
  if (show)
  {
    boxLabelsColumn->show();
    boxLabelsAlign->show();
    justifyLabelsLbl->show();
    labelsColumnLbl->show();
  }
  else
  {
    boxLabelsColumn->hide();
    boxLabelsAlign->hide();
    justifyLabelsLbl->hide();
    labelsColumnLbl->hide();
  }
}
void PlotDialog::updateEndPointColumns(const QString& text)
{
  QStringList cols = text.split(",", QString::SkipEmptyParts);
  QStringList aux = cols[0].split(":", QString::SkipEmptyParts);
  QString table = aux[0];
  QStringList list;
  for (int i = 0; i < (int) columnNames.count(); i++)
  {
    QString s = columnNames[i];
    if (s.contains(table))
      list << s;
  }

  xEndBox->clear();
  xEndBox->insertStringList(list);
  xEndBox->setCurrentText(table + "_" + cols[2].remove("(X)").remove("(A)"));

  yEndBox->clear();
  yEndBox->insertStringList(list);
  yEndBox->setCurrentText(table + "_" + cols[3].remove("(Y)").remove("(M)"));
}

bool PlotDialog::acceptParams()
{
  if (privateTabWidget->currentWidget() == fontsPage)
  {
    d_ml->setFonts(titleFont, axesFont, numbersFont, legendFont);
    return true;
  }
  else if (privateTabWidget->currentWidget() == printPage)
  {
    d_ml->setScaleLayersOnPrint(boxScaleLayers->isChecked());
    d_ml->printCropmarks(boxPrintCrops->isChecked());
    return true;
  }
  else if (privateTabWidget->currentWidget() == layerPage)
  {
    if (boxAll->isChecked())
    {
      QList<Graph *> layers = d_ml->layersList();
      foreach(Graph *g, layers)
      {
        g->setFrame(boxBorderWidth->value(), boxBorderColor->color());
        g->setMargin(boxMargin->value());

        QColor c = boxBackgroundColor->color();
        c.setAlpha(boxBackgroundTransparency->value());
        g->setBackgroundColor(c);

        c = boxCanvasColor->color();
        c.setAlpha(boxCanvasTransparency->value());
        g->setCanvasBackground(c);

        g->setAntialiasing(boxAntialiasing->isChecked());

        //Canvas frame
        g->setCanvasFrame(boxCanvasFrameWidth->value(), boxCanvasFrameColor->color());
      }
    }
    else
    {
      LayerItem *item = dynamic_cast<LayerItem*>(listBox->currentItem());
      if (!item)
      {
        return (true);
      }
      else
      {
        Graph *g = item->graph();
        QColor c = boxBackgroundColor->color();

        //background opacity & color
        c.setAlpha(boxBackgroundTransparency->value());
        g->setBackgroundColor(c);
        //Canvas color & opacity
        c = boxCanvasColor->color();
        c.setAlpha(boxCanvasTransparency->value());
        g->setCanvasBackground(c);
        //Border color & width
        g->setFrame(boxBorderWidth->value(), boxBorderColor->color());
        //Anti-aliasing
        g->setAntialiasing(boxAntialiasing->isChecked());
        //Margin
        g->setMargin(boxMargin->value());
        //Canvas frame
        g->setCanvasFrame(boxCanvasFrameWidth->value(), boxCanvasFrameColor->color());
      }
    }
    return true;
  }
  else if (privateTabWidget->currentWidget() == layerGeometryPage)
  {
    LayerItem *item = dynamic_cast<LayerItem*>(listBox->currentItem());
    if (!item)
      return false;
    Graph *g = item->graph();
    if (!g)
      return false;

    g->enableFixedAspectRatio(keepRatioOnResizeBox->isChecked());

    QPoint pos = QPoint(boxX->value(), boxY->value());
    QSize size = QSize(boxLayerWidth->value(), boxLayerHeight->value());
    if (g->pos() == pos && g->size() == size)
      return true;

    g->setGeometry(QRect(pos, size));
    g->plotWidget()->resize(size);
    d_ml->repaint();
    return true;
  }

  QTreeWidgetItem *it = listBox->currentItem();

  if (!it)
    return false;

  CurveTreeItem *item = dynamic_cast<CurveTreeItem*>(it);
  QwtPlotItem *plotItem = dynamic_cast<QwtPlotItem *>(item->plotItem());
  if (!plotItem)
    return false;

  Graph *graph = item->graph();
  if (!graph)
    return false;

  if (privateTabWidget->currentPage() == axesPage)
  {
    plotItem->setAxis(boxXAxis->currentIndex() + 2, boxYAxis->currentIndex());
    graph->setAutoScale();
    return true;
  }
  else if (privateTabWidget->currentPage() == spectrogramPage)
  {
    Spectrogram *sp = dynamic_cast<Spectrogram*>(plotItem);

    if (!sp || sp->rtti() != QwtPlotItem::Rtti_PlotSpectrogram)
    {
      return false;
    }

    if (grayScaleBox->isChecked())
    {
      sp->setGrayScale();
    }
    else if (defaultScaleBox->isChecked())
    {
      sp->setDefaultColorMap();
    }
    //if customScaleBox radiobutton is selected it loads the colormap file and
    //updates the spectrogram,colormap widget with the loaded file
    else if (customScaleBox->isChecked())
    {
      sp->mutableColorMap().loadMap(mCurrentColorMap);
      // sp->setDefaultColorMap();
      sp->setCustomColorMap(sp->mutableColorMap());
      //sets the selected colormapfile name to spectrogram
      sp->setColorMapFileName(mCurrentColorMap);
      //saves the settings
      sp->saveSettings();
    }

    sp->showColorScale((QwtPlot::Axis) colorScaleBox->currentItem(), axisScaleBox->isChecked());
    sp->setColorBarWidth(colorScaleWidthBox->value());

    //Update axes page
    boxXAxis->setCurrentItem(sp->xAxis() - 2);
    boxYAxis->setCurrentItem(sp->yAxis());
  }
  else if (privateTabWidget->currentPage() == contourLinesPage)
  {
    Spectrogram *sp = dynamic_cast<Spectrogram*>(plotItem);

    if (!sp || sp->rtti() != QwtPlotItem::Rtti_PlotSpectrogram)
    {
      return false;
    }

    if (defaultContourBox->isChecked())
    {
      QPen pen = QPen(levelsColorBox->color(), contourWidthBox->value(),
          Graph::getPenStyle(boxContourStyle->currentIndex()));
      pen.setCosmetic(true);
      sp->setDefaultContourPen(pen);
      sp->setColorMapPen(false);
    }
    else if (customPenBtn->isChecked())
      contourLinesEditor->updateContourPens();
    else
      sp->setColorMapPen();

    contourLinesEditor->updateContourLevels();
    sp->setDisplayMode(QwtPlotSpectrogram::ContourMode, levelsGroupBox->isChecked());
    labelsGroupBox->setChecked(levelsGroupBox->isChecked());
    labelsGroupBox->setEnabled(levelsGroupBox->isChecked());
    sp->showContourLineLabels(levelsGroupBox->isChecked());
  }
  else if (privateTabWidget->currentPage() == linePage)
  {
    int index = item->plotItemIndex();
    graph->setCurveStyle(index, boxConnect->currentIndex());
    QBrush br = QBrush(boxAreaColor->color(), boxPattern->getSelectedPattern());
    if (!fillGroupBox->isChecked())
      br = QBrush();
    graph->setCurveBrush(index, br);

    QPen pen = QPen(boxLineColor->color(), boxLineWidth->value(),
        Graph::getPenStyle(boxLineStyle->currentIndex()));
    QwtPlotCurve *curve = dynamic_cast<QwtPlotCurve *>(plotItem);
    curve->setPen(pen);
  }
  else if (privateTabWidget->currentPage() == symbolPage)
  {
    int size = 2 * boxSymbolSize->value() + 1;
    QBrush br = QBrush(boxFillColor->color(), Qt::SolidPattern);

    if (!boxFillSymbol->isChecked())
      br = QBrush();

    QPen pen = QPen(boxSymbolColor->color(), boxPenWidth->value(), Qt::SolidLine);
    QwtSymbol s = QwtSymbol(boxSymbolStyle->selectedSymbol(), br, pen, QSize(size, size));
    QwtPlotCurve *curve = dynamic_cast<QwtPlotCurve*>(plotItem);
    curve->setSymbol(s);
  }
  else if (privateTabWidget->currentPage() == histogramPage)
  {
    QwtHistogram *h = dynamic_cast<QwtHistogram*>(plotItem);
    if (!h)
      return false;

    if (validInput())
    {
      if (h->autoBinning() == automaticBox->isChecked() && h->binSize() == binSizeBox->text().toDouble()
          && h->begin() == histogramBeginBox->text().toDouble()
          && h->end() == histogramEndBox->text().toDouble())
        return true;

      h->setBinning(automaticBox->isChecked(), binSizeBox->text().toDouble(),
          histogramBeginBox->text().toDouble(), histogramEndBox->text().toDouble());
      h->loadData();

      graph->updateScale();
      graph->notifyChanges();

      return true;
    }
  }
  else if (privateTabWidget->currentPage() == spacingPage)
    graph->setBarsGap(item->plotItemIndex(), gapBox->value(), offsetBox->value());
  else if (privateTabWidget->currentPage() == vectPage)
  {
    int index = item->plotItemIndex();
    if (!d_app)
      return false;

    QString xEndCol = xEndBox->currentText();
    QString yEndCol = yEndBox->currentText();
    Table* w = d_app->table(xEndCol);
    if (!w)
      return false;

    graph->updateVectorsLayout(index, vectColorBox->color(), vectWidthBox->value(),
        headLengthBox->value(), headAngleBox->value(), filledHeadBox->isChecked(),
        vectPosBox->currentIndex(), xEndCol, yEndCol);

    QString text = item->text(0);
    QStringList t = text.split(": ", QString::SkipEmptyParts);
    QString table = t[0];

    QStringList cols = t[1].split(",", QString::SkipEmptyParts);
    if (graph->curveType(index) == Graph::VectXYXY)
    {
      xEndCol = xEndCol.remove(table + "_") + "(X)";
      yEndCol = yEndCol.remove(table + "_") + "(Y)";
    }
    else
    {
      xEndCol = xEndCol.remove(table + "_") + "(A)";
      yEndCol = yEndCol.remove(table + "_") + "(M)";
    }

    if (cols[2] != xEndCol || cols[3] != yEndCol)
    {
      cols[2] = xEndCol;
      cols[3] = yEndCol;
      item->setText(0, table + ": " + cols.join(","));
    }
    return true;
  }
  else if (privateTabWidget->currentPage() == errorsPage)
  {
    graph->updateErrorBars(dynamic_cast<QwtErrorPlotCurve*>(item->plotItem()), xBox->isChecked(),
        widthBox->value(), capBox->currentText().toInt(), colorBox->color(), plusBox->isChecked(),
        minusBox->isChecked(), throughBox->isChecked());
    return true;
  }
  else if (privateTabWidget->currentPage() == piePage)
  {
    QwtPieCurve *pie = dynamic_cast<QwtPieCurve*>(plotItem);
    pie->setPen(
        QPen(boxPieLineColor->color(), boxPieLineWidth->value(),
            Graph::getPenStyle(boxPieLineStyle->currentIndex())));
    pie->setBrushStyle(boxPiePattern->getSelectedPattern());
    pie->setFirstColor(boxFirstColor->currentIndex());
  }
  else if (privateTabWidget->currentPage() == pieGeometryPage)
  {
    QwtPieCurve *pie = dynamic_cast<QwtPieCurve*>(plotItem);
    pie->setViewAngle(boxPieViewAngle->value());
    pie->setThickness(boxPieThickness->value());
    pie->setRadius(boxRadius->value());
    pie->setHorizontalOffset(boxPieOffset->value());
    pie->setStartAzimuth(boxPieStartAzimuth->value());
    pie->setCounterClockwise(boxPieConterClockwise->isChecked());
  }
  else if (privateTabWidget->currentPage() == pieLabelsPage)
  {
    QwtPieCurve *pie = dynamic_cast<QwtPieCurve*>(plotItem);
    pie->setLabelsAutoFormat(pieAutoLabelsBox->isChecked());
    pie->setLabelValuesFormat(boxPieValues->isChecked());
    pie->setLabelPercentagesFormat(boxPiePercentages->isChecked());
    pie->setLabelCategories(boxPieCategories->isChecked());
    pie->setFixedLabelsPosition(boxPieWedge->isChecked());
    pie->setLabelsEdgeDistance(boxPieEdgeDist->value());
    graph->replot();
  }
  else if (privateTabWidget->currentPage() == percentilePage)
  {
    BoxCurve *b = dynamic_cast<BoxCurve*>(plotItem);
    if (b)
    {
      b->setMaxStyle(boxMaxStyle->selectedSymbol());
      b->setP99Style(box99Style->selectedSymbol());
      b->setMeanStyle(boxMeanStyle->selectedSymbol());
      b->setP1Style(box1Style->selectedSymbol());
      b->setMinStyle(boxMinStyle->selectedSymbol());

      int size = 2 * boxPercSize->value() + 1;
      QBrush br = QBrush(boxPercFillColor->color(), Qt::SolidPattern);
      if (!boxFillSymbols->isChecked())
        br = QBrush();
      QwtSymbol s = QwtSymbol(QwtSymbol::NoSymbol, br,
          QPen(boxEdgeColor->color(), boxEdgeWidth->value(), Qt::SolidLine), QSize(size, size));
      b->setSymbol(s);
    }
  }
  else if (privateTabWidget->currentPage() == boxPage)
  {
    BoxCurve *b = dynamic_cast<BoxCurve*>(plotItem);
    if (b)
    {
      b->setBoxWidth(boxWidth->value());
      b->setBoxStyle(boxType->currentIndex());
      if (boxCnt->isVisible())
        b->setBoxRange(boxRange->currentIndex() + 1, boxCnt->value());
      else
        b->setBoxRange(boxRange->currentIndex() + 1, (double) boxCoef->value());

      if (whiskerCnt->isVisible())
        b->setWhiskersRange(boxWhiskersRange->currentIndex(), whiskerCnt->value());
      else
        b->setWhiskersRange(boxWhiskersRange->currentIndex(), (double) boxWhiskersCoef->value());
    }
  }
  else if (privateTabWidget->currentPage() == labelsPage)
  {

    Spectrogram *sp = dynamic_cast<Spectrogram*>(plotItem);
    if (sp && sp->rtti() == QwtPlotItem::Rtti_PlotSpectrogram)
    {
      sp->setLabelsRotation(boxLabelsAngle->value());
      sp->setLabelsColor(boxLabelsColor->color());
      sp->showContourLineLabels(labelsGroupBox->isChecked());
      sp->setLabelsWhiteOut(boxLabelsWhiteOut->isChecked());
      sp->setLabelsOffset((double) boxLabelsXOffset->value(), (double) boxLabelsYOffset->value());

    }
    else
    {
      DataCurve *c = dynamic_cast<DataCurve *>(plotItem);
      if (!c)
        return false;

      QString text = item->text(0);
      QStringList t = text.split(": ", QString::SkipEmptyParts);
      QString table = t[0];
      QStringList cols = t[1].split(",", QString::SkipEmptyParts);

      if (labelsGroupBox->isChecked())
      {
        c->setLabelsColumnName(boxLabelsColumn->currentText());

        if (cols.count() == 3)
          cols[2] = boxLabelsColumn->currentText().remove(table + "_") + "(L)";
        else if (cols.count() == 5)     //vector curves
          cols[4] = boxLabelsColumn->currentText().remove(table + "_") + "(L)";
        else
          cols << boxLabelsColumn->currentText().remove(table + "_") + "(L)";
        item->setText(0, table + ": " + cols.join(","));
      }
      else
      {
        c->setLabelsColumnName(QString());
        cols.pop_back();
        item->setText(0, table + ": " + cols.join(","));
      }

      c->setLabelsRotation(boxLabelsAngle->value());
      c->setLabelsWhiteOut(boxLabelsWhiteOut->isChecked());
      c->setLabelsOffset(boxLabelsXOffset->value(), boxLabelsYOffset->value());
      c->setLabelsColor(boxLabelsColor->color());
      c->setLabelsAlignment(labelsAlignment());
    }
  }

  graph->replot();
  graph->notifyChanges();
  return true;
}

void PlotDialog::setAutomaticBinning()
{
  GroupBoxH->setEnabled(!automaticBox->isChecked());
}

bool PlotDialog::validInput()
{
  QString from = histogramBeginBox->text();
  QString to = histogramEndBox->text();
  QString step = binSizeBox->text();
  QRegExp nonDigit("\\D");

  if (histogramBeginBox->text().isEmpty())
  {
    QMessageBox::critical(this, tr("MantidPlot - Input error"), tr("Please enter a valid start limit!"));
    histogramBeginBox->setFocus();
    return false;
  }

  if (histogramEndBox->text().isEmpty())
  {
    QMessageBox::critical(this, tr("MantidPlot - Input error"), tr("Please enter a valid end limit!"));
    histogramEndBox->setFocus();
    return false;
  }

  if (binSizeBox->text().isEmpty())
  {
    QMessageBox::critical(this, tr("MantidPlot - Input error"),
        tr("Please enter a valid bin size value!"));
    binSizeBox->setFocus();
    return false;
  }

  from = from.remove(".");
  to = to.remove(".");
  step = step.remove(".");

  int pos = from.find("-", 0);
  if (pos == 0)
    from = from.replace(pos, 1, "");

  pos = to.find("-", 0);
  if (pos == 0)
    to = to.replace(pos, 1, "");

  double start, end, stp;
  if (from.contains(nonDigit))
  {
    try
    {
      MyParser parser;
      parser.SetExpr((histogramBeginBox->text()).ascii());
      start = parser.Eval();
    } catch (mu::ParserError &e)
    {
      QMessageBox::critical(this, tr("MantidPlot - Start limit error"),
          QString::fromStdString(e.GetMsg()));
      histogramBeginBox->setFocus();
      return false;
    }
  }
  else
    start = histogramBeginBox->text().toDouble();

  if (to.contains(nonDigit))
  {
    try
    {
      MyParser parser;
      parser.SetExpr((histogramEndBox->text()).ascii());
      end = parser.Eval();
    } catch (mu::ParserError &e)
    {
      QMessageBox::critical(this, tr("MantidPlot - End limit error"),
          QString::fromStdString(e.GetMsg()));
      histogramEndBox->setFocus();
      return false;
    }
  }
  else
    end = histogramEndBox->text().toDouble();

  if (start >= end)
  {
    QMessageBox::critical(this, tr("MantidPlot - Input error"),
        tr("Please enter limits that satisfy: begin < end!"));
    histogramEndBox->setFocus();
    return false;
  }

  if (step.contains(nonDigit))
  {
    try
    {
      MyParser parser;
      parser.SetExpr((binSizeBox->text()).ascii());
      stp = parser.Eval();
    } catch (mu::ParserError &e)
    {
      QMessageBox::critical(this, tr("MantidPlot - Bin size input error"),
          QString::fromStdString(e.GetMsg()));
      binSizeBox->setFocus();
      return false;
    }
  }
  else
    stp = binSizeBox->text().toDouble();

  if (stp <= 0)
  {
    QMessageBox::critical(this, tr("MantidPlot - Bin size input error"),
        tr("Please enter a positive bin size value!"));
    binSizeBox->setFocus();
    return false;
  }

  return true;
}
void PlotDialog::setPiePenStyle(const Qt::PenStyle& style)
{
  if (style == Qt::SolidLine)
    boxPieLineStyle->setCurrentItem(0);
  if (style == Qt::DashLine)
    boxPieLineStyle->setCurrentItem(1);
  if (style == Qt::DotLine)
    boxPieLineStyle->setCurrentItem(2);
  if (style == Qt::DashDotLine)
    boxPieLineStyle->setCurrentItem(3);
  if (style == Qt::DashDotDotLine)
    boxPieLineStyle->setCurrentItem(4);
}

void PlotDialog::setPenStyle(Qt::PenStyle style)
{
  switch (style)
  {
  case Qt::SolidLine:
    boxLineStyle->setCurrentIndex(0);
    break;
  case Qt::DashLine:
    boxLineStyle->setCurrentIndex(1);
    break;
  case Qt::DotLine:
    boxLineStyle->setCurrentIndex(2);
    break;
  case Qt::DashDotLine:
    boxLineStyle->setCurrentIndex(3);
    break;
  case Qt::DashDotDotLine:
    boxLineStyle->setCurrentIndex(4);
    break;
  default:
    boxLineStyle->setCurrentIndex(0);
    break;
  }
}

void PlotDialog::setBoxType(int index)
{
  boxCoeffLabel->hide();
  boxRangeLabel->hide();
  boxRange->hide();
  boxCoef->hide();
  boxCntLabel->hide();
  boxCnt->hide();

  if (index != BoxCurve::NoBox && index != BoxCurve::WindBox)
  {
    boxRange->show();
    boxRangeLabel->show();
    int id = boxRange->currentIndex() + 1;
    if (id == BoxCurve::UserDef)
    {
      boxCoef->show();
      boxCoeffLabel->show();
    }
    else if (id == BoxCurve::SD || id == BoxCurve::SE)
    {
      boxCntLabel->show();
      boxCnt->show();
    }
  }
}

void PlotDialog::setBoxRangeType(int index)
{
  boxCoeffLabel->hide();
  boxCoef->hide();
  boxCntLabel->hide();
  boxCnt->hide();

  index++;
  if (index == BoxCurve::UserDef)
  {
    boxCoeffLabel->show();
    boxCoef->show();
  }
  else if (index == BoxCurve::SD || index == BoxCurve::SE)
  {
    boxCntLabel->show();
    boxCnt->show();
  }
}

void PlotDialog::setWhiskersRange(int index)
{
  whiskerCoeffLabel->hide();
  boxWhiskersCoef->hide();
  whiskerCntLabel->hide();
  whiskerCnt->hide();

  if (index == BoxCurve::UserDef)
  {
    whiskerCoeffLabel->show();
    boxWhiskersCoef->show();
  }
  else if (index == BoxCurve::SD || index == BoxCurve::SE)
  {
    whiskerCntLabel->show();
    whiskerCnt->show();
  }
}

void PlotDialog::customVectorsPage(bool angleMag)
{
  if (angleMag)
  {
    GroupBoxVectEnd->setTitle(tr("Vector Data"));
    labelXEnd->setText(tr("Angle"));
    labelYEnd->setText(tr("Magnitude"));
    labelPosition->show();
    vectPosBox->show();
  }
  else
  {
    GroupBoxVectEnd->setTitle(tr("End Point"));
    labelXEnd->setText(tr("X End"));
    labelYEnd->setText(tr("Y End"));
    labelPosition->hide();
    vectPosBox->hide();
  }
}

/**
 This slot gets called on clicking slect colormap button
 */
void PlotDialog::changeColormap(const QString &filename)
{
  //loads the settings to get the colormap file name.
  //as theres is no spectrgram valid pointer here i'm directly using Qsetting
  // instead of Spectrogram::loadSettings()
  //mCurrentColorMap gives the last selected colormap file name
  QSettings settings;
  settings.beginGroup("Mantid/2DPlotSpectrogram");
  //Load Colormap. If the file is invalid the default stored colour map is used
  mCurrentColorMap = settings.value("ColormapFile", "").toString();
  settings.endGroup();

  QString fileselection;
  //Use a file dialog if no parameter is passed
  if (filename.isEmpty())
  {
    fileselection = MantidColorMap::loadMapDialog(mCurrentColorMap, this);
    if (fileselection.isEmpty())
      return;
  }
  else
  {
    fileselection = QFileInfo(filename).absoluteFilePath();
    if (!QFileInfo(fileselection).exists())
      return;
  }

  if (fileselection == mCurrentColorMap)
    return;

  mCurrentColorMap = fileselection;
}

void PlotDialog::showDefaultContourLinesBox(bool)
{
  if (autoContourBox->isChecked())
    defaultPenBox->hide();
  else
    defaultPenBox->show();
}

void PlotDialog::updateTreeWidgetItem(QTreeWidgetItem *item)
{
  if (item->type() != QTreeWidgetItem::Type)
    return;

  if (item->isExpanded())
    item->setIcon(0, QIcon(getQPixmap("folder_open")));
  else
    item->setIcon(0, QIcon(getQPixmap("folder_closed")));
}

void PlotDialog::setTitlesFont()
{
  bool ok;
  QFont font = QFontDialog::getFont(&ok, titleFont, this);
  if (ok)
  {
    titleFont = font;
  }
  else
  {
    return;
  }
}

void PlotDialog::setAxesLabelsFont()
{
  bool ok;
  QFont font = QFontDialog::getFont(&ok, axesFont, this);
  if (ok)
  {
    axesFont = font;
  }
  else
  {
    return;
  }
}

void PlotDialog::setAxesNumbersFont()
{
  bool ok;
  QFont font = QFontDialog::getFont(&ok, numbersFont, this);
  if (ok)
  {
    numbersFont = font;
  }
  else
  {
    return;
  }
}

void PlotDialog::setLegendsFont()
{
  bool ok;
  QFont font = QFontDialog::getFont(&ok, legendFont, this);
  if (ok)
  {
    legendFont = font;
  }
  else
  {
    return;
  }
}

void PlotDialog::initFonts(const QFont& titlefont, const QFont& axesfont, const QFont& numbersfont,
    const QFont& legendfont)
{
  axesFont = axesfont;
  titleFont = titlefont;
  numbersFont = numbersfont;
  legendFont = legendfont;
}

void PlotDialog::adjustLayerHeight(int width)
{
  if (keepRatioBox->isChecked())
  {
    disconnect(boxLayerHeight, SIGNAL( valueChanged ( int ) ), this, SLOT( adjustLayerWidth(int) ));
    boxLayerHeight->setValue(static_cast<int>(width / aspect_ratio));
    connect(boxLayerHeight, SIGNAL( valueChanged ( int ) ), this, SLOT( adjustLayerWidth(int) ));
  }
  else
    aspect_ratio = (double) width / double(boxLayerHeight->value());
}

void PlotDialog::adjustLayerWidth(int height)
{
  if (keepRatioBox->isChecked())
  {
    disconnect(boxLayerWidth, SIGNAL( valueChanged ( int ) ), this, SLOT( adjustLayerHeight(int) ));
    boxLayerWidth->setValue(static_cast<int>(height * aspect_ratio));
    connect(boxLayerWidth, SIGNAL( valueChanged ( int ) ), this, SLOT( adjustLayerHeight(int) ));
  }
  else
    aspect_ratio = double(boxLayerWidth->value()) / (double) height;
}

void PlotDialog::closeEvent(QCloseEvent* e)
{
  if (d_app)
    d_app->d_extended_plot_dialog = btnMore->isChecked();

  e->accept();
}

void PlotDialog::chooseLabelsFont()
{
  QTreeWidgetItem *item = listBox->currentItem();
  if (!item || item->type() != CurveTreeItem::PlotCurveTreeItem)
    return;

  QwtPlotItem *i = dynamic_cast<CurveTreeItem*>(item)->plotItem();
  Graph *graph = dynamic_cast<CurveTreeItem*>(item)->graph();
  if (!i || !graph)
    return;

  DataCurve *c = dynamic_cast<DataCurve *>(i);
  if (!c)
    return;
  bool okF;
  QFont fnt = QFontDialog::getFont(&okF, c->labelsFont(), this);
  if (okF && fnt != c->labelsFont())
  {
    c->setLabelsFont(fnt);
    graph->replot();
    graph->notifyChanges();
  }
}

int PlotDialog::labelsAlignment()
{
  int align = -1;
  switch (boxLabelsAlign->currentIndex())
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

/*****************************************************************************
 *
 * Class LayerItem
 *
 *****************************************************************************/

LayerItem::LayerItem(Graph *g, QTreeWidgetItem *parent, const QString& s) :
    QTreeWidgetItem(parent, QStringList(s), LayerTreeItem), d_graph(g)
{
  setIcon(0, getQPixmap("layer_disabled_xpm"));
  if (g)
    insertCurvesList();
}

void LayerItem::setActive(bool on)
{
  if (on)
    setIcon(0, getQPixmap("layer_enabled_xpm"));
  else
    setIcon(0, getQPixmap("layer_disabled_xpm"));
}

void LayerItem::insertCurvesList()
{
  for (int i = 0; i < d_graph->curves(); i++)
  {
    QString plotAssociation = QString();
    QwtPlotItem *it = dynamic_cast<QwtPlotItem *>(d_graph->plotItem(i));
    if (!it)
      continue;

    if (it->rtti() == QwtPlotItem::Rtti_PlotCurve)
    {
      PlotCurve *c = dynamic_cast<PlotCurve *>(it);
      DataCurve* dc = dynamic_cast<DataCurve *>(it);
      if (!c || !dc)
        continue;

      if (dc && c->type() != Graph::Function && dc->table())
      {
        QString s = dc->plotAssociation();
        QString table = dc->table()->name();
        plotAssociation = table + ": " + s.remove(table + "_");
      }
      else
      {
        plotAssociation = it->title().text();
      }
    }
    else  // builds the names/labels of special (non-) curves within layers displayed in tree entries
    {
      if (d_graph->isSpectrogram() || it->title().isEmpty())
        plotAssociation = it->title().text() + " Layer details (editable)";
      else
        plotAssociation = it->title().text();
    }

    addChild(new CurveTreeItem(it, this, plotAssociation));
  }
}

/*****************************************************************************
 *
 * Class CurveTreeItem
 *
 *****************************************************************************/

CurveTreeItem::CurveTreeItem(QwtPlotItem *curve, LayerItem *parent, const QString& s) :
    QTreeWidgetItem(parent, QStringList(s), PlotCurveTreeItem), d_curve(curve)
{
  setIcon(0, getQPixmap("graph_disabled_xpm"));
}

Graph* CurveTreeItem::graph()
{
  LayerItem *l = dynamic_cast<LayerItem *>(parent());
  if (l) {
    return l->graph();
  } else {
    return NULL;
  }
}

void CurveTreeItem::setActive(bool on)
{
  if (on)
    setIcon(0, getQPixmap("graph_xpm"));
  else
    setIcon(0, getQPixmap("graph_disabled_xpm"));
}

int CurveTreeItem::plotItemIndex()
{
  Graph *g = graph();
  if (!g)
    return -1;

  return g->plotItemIndex(dynamic_cast<QwtPlotItem *>(d_curve));
}

int CurveTreeItem::plotItemType()
{
  Graph *g = graph();
  if (!g)
    return -1;

  int index = g->plotItemIndex(dynamic_cast<QwtPlotItem *>(d_curve));
  return g->curveType(index);
}
