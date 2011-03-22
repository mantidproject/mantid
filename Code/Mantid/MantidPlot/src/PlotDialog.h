/***************************************************************************
    File                 : PlotDialog.h
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
#ifndef PLOTDIALOG_H
#define PLOTDIALOG_H

#include <QDialog>
#include <QTreeWidgetItem>
#include "MultiLayer.h"
//#include "MantidKernel/Logger.h"



class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QSpinBox;
class QTabWidget;
class QWidget;
class QStringList;
class QGroupBox;
class QDoubleSpinBox;
class QRadioButton;
class QTreeWidget;

class LayerItem;
class CurveTreeItem;
class ColorBox;
class PatternBox;
class ColorButton;
class MultiLayer;
class SymbolBox;
class ColorMapEditor;
class QwtPlotItem;
class DoubleSpinBox;
class PenStyleBox;
class ContourLinesEditor;

//! Custom plot/curves dialog
class PlotDialog : public QDialog
{
  Q_OBJECT

public:
  PlotDialog(bool showExtended, QWidget* parent = 0, Qt::WFlags fl = 0 );
  void initFonts(const QFont& titlefont, const QFont& axesfont, const QFont& numbersfont, const QFont& legendfont);
  void insertColumnsList(const QStringList& names){columnNames = names;};
  void setMultiLayer(MultiLayer *ml);

public slots:
  void showAll(bool all);
  void selectCurve(int index);

private slots:
  void showStatistics();
  void customVectorsPage(bool angleMag);
  void updateEndPointColumns(const QString& text);

  void fillBoxSymbols();
  void fillSymbols();
  bool acceptParams();
  void showWorksheet();
  void quit();

  int setPlotType(CurveTreeItem *item);
  void changePlotType(int plotType);
  void setActiveCurve(CurveTreeItem *item);

  void insertTabs(int plot_type);
  void updateTabWindow(QTreeWidgetItem *currentItem, QTreeWidgetItem *previousItem);
  void showAreaColor(bool show);

  void removeSelectedCurve();

  /******* error bars options **************/
  void pickErrorBarsColor();
  void changeErrorBarsType();
  void changeErrorBarsPlus();
  void changeErrorBarsMinus();
  void changeErrorBarsThrough();

  void setAutomaticBinning();
  bool validInput();
  void showPlotAssociations(QTreeWidgetItem *item, int);

  void setPenStyle(Qt::PenStyle style);
  void setPiePenStyle(const Qt::PenStyle& style);

  //box plots
  void setBoxType(int index);
  void setBoxRangeType(int index);
  void setWhiskersRange(int index);

  //spectrograms
  void showDefaultContourLinesBox(bool show);
  void showColorMapEditor(bool show);
  void showSelectColorMapButton(bool show);

  //layer geometry
  void adjustLayerHeight(int width);
  void adjustLayerWidth(int height);
  void changeColormap(const QString & filename = "");
  void setEquidistantLevels();

protected slots:
  void setActiveLayer(LayerItem *item);
  void updateTreeWidgetItem(QTreeWidgetItem *item);
  void updateBackgroundTransparency(int alpha);
  void updateCanvasTransparency(int alpha);
  void updateAntialiasing(bool on);
  void updateBorder(int width);
  void pickBackgroundColor();
  void pickCanvasColor();
  void pickBorderColor();
  void changeMargin(int);
  void setTitlesFont();
  void setAxesLabelsFont();
  void setAxesNumbersFont();
  void setLegendsFont();
  void editCurve();
  void chooseLabelsFont();
  void showCustomPenColumn(bool on);

private:
  int labelsAlignment();
  void closeEvent(QCloseEvent* e);

  void clearTabWidget();
  void initAxesPage();
  void initLinePage();
  void initSymbolsPage();
  void initHistogramPage();
  void initErrorsPage();
  void initSpacingPage();
  void initVectPage();
  void initBoxPage();
  void initPercentilePage();
  void initSpectrogramPage();
  void initLayerPage();
  void initLayerGeometryPage();
  void initFontsPage();
  void initPiePage();
  void initPieGeometryPage();
  void initPieLabelsPage();
  void initPrintPage();
  void initLabelsPage();
  void initContourLinesPage();
  void contextMenuEvent(QContextMenuEvent *e);
  void updateContourLevelsDisplay(Spectrogram *sp);

  void showAllLabelControls(bool show = true);

  double aspect_ratio;

  QFont titleFont, legendFont, axesFont, numbersFont;

  MultiLayer *d_ml;
  QStringList columnNames;

  QSpinBox* boxX, *boxY, *boxLayerWidth, *boxLayerHeight;
  QCheckBox *keepRatioBox;

  QPushButton *btnTitle, *btnAxesLabels, *btnAxesNumbers, *btnLegend;
  ColorMapEditor *colorMapEditor;
  QPushButton* mSelectColormap;
  QString mCurrentColorMap;
  QWidget *curvePlotTypeBox, *layerPage, *layerGeometryPage, *piePage, *fontsPage, *printPage, *contourLinesPage;;
  QTreeWidget* listBox;
  QCheckBox *boxAntialiasing, *boxAll, *boxScaleLayers, *boxPrintCrops;
  ColorButton *boxBorderColor, *boxBackgroundColor, *boxCanvasColor;
  QSpinBox *boxBackgroundTransparency, *boxCanvasTransparency, *boxBorderWidth, *boxMargin;
  QSpinBox *boxRadius;
  DoubleSpinBox *boxPieLineWidth;
  ColorBox *boxFirstColor, *boxPieLineColor;
  PatternBox *boxPiePattern;
  QComboBox* boxPieLineStyle;

  QPushButton* buttonApply, *btnWorksheet;
  QPushButton* buttonOk, *btnMore;
  QPushButton* buttonCancel;
  QComboBox* boxPlotType;
  QWidget* linePage;
  QComboBox* boxConnect;
  QComboBox* boxLineStyle;
  DoubleSpinBox *boxLineWidth, *boxPenWidth;
  ColorBox* boxLineColor, *boxAreaColor;
  QWidget* symbolPage;
  QSpinBox* boxSymbolSize;
  ColorBox* boxSymbolColor,*boxFillColor;
  SymbolBox* boxSymbolStyle;
  PatternBox *boxPattern;
  QTabWidget* privateTabWidget;
  QWidget *errorsPage, *spectrogramPage;
  QGroupBox* fillGroupBox;
  QCheckBox* plusBox;
  QCheckBox* minusBox;
  QCheckBox* xBox;
  ColorButton *colorBox, *levelsColorBox;
  ColorBox* vectColorBox;
  DoubleSpinBox* widthBox;
  QComboBox* capBox;
  QCheckBox* throughBox;
  QLabel *labelPosition, *labelXEnd, *labelYEnd,*justifyLabelsLbl,*labelsColumnLbl;
  QGroupBox* GroupBoxH;
  QWidget *histogramPage, *spacingPage;
  QLineEdit *binSizeBox, *histogramBeginBox, *histogramEndBox;
  QCheckBox *automaticBox;
  QPushButton* buttonStatistics, *btnEditCurve;
  QSpinBox* gapBox, *offsetBox, *boxWidth;
  QWidget *vectPage, *boxPage, *percentilePage, *axesPage;
  QComboBox *xEndBox, *yEndBox, *boxType, *boxWhiskersType, *boxWhiskersRange, *boxRange;
  QSpinBox* headAngleBox, *headLengthBox, *boxPercSize;
  DoubleSpinBox *vectWidthBox, *boxEdgeWidth;
  QCheckBox *filledHeadBox;
  QSpinBox *boxCoef, *boxWhiskersCoef;
  QCheckBox *boxFillSymbols, *boxFillSymbol;
  ColorBox *boxPercFillColor, *boxEdgeColor;
  QLabel 	*whiskerCoeffLabel, *whiskerRangeLabel, *boxCoeffLabel;
  QLabel *boxRangeLabel, *whiskerCntLabel, *boxCntLabel;
  QGroupBox *GroupBoxVectEnd;
  QComboBox *vectPosBox, *boxXAxis, *boxYAxis, *colorScaleBox;//, *boxContourStyle;
  PenStyleBox	*penContourStyle,*boxContourStyle;;
  QSpinBox *levelsBox, *colorScaleWidthBox;
  DoubleSpinBox *contourWidthBox;
  QGroupBox *levelsGroupBox, *axisScaleBox, *imageGroupBox;
  QGroupBox *defaultPenBox;
  QRadioButton *defaultScaleBox, *grayScaleBox, *customScaleBox, *defaultContourBox, *autoContourBox;

  SymbolBox *boxMaxStyle, *boxMinStyle, *boxMeanStyle, *box99Style, *box1Style;
  QDoubleSpinBox *whiskerCnt, *boxCnt;
  //!Labels page
  QGroupBox *labelsGroupBox;
  DoubleSpinBox *boxLabelsAngle;
  QSpinBox *boxLabelsXOffset, *boxLabelsYOffset;
  QCheckBox *boxLabelsWhiteOut;
  QPushButton *btnLabelsFont,*btnSetEquidistantLevels;
  QComboBox *boxLabelsAlign, *boxLabelsColumn;
  ColorBox* boxLabelsColor;
  QWidget *labelsPage;

  QGroupBox *pieAutoLabelsBox, *boxPieWedge;
  DoubleSpinBox *boxPieStartAzimuth, *boxPieEdgeDist, *boxPieViewAngle, *boxPieThickness,*firstContourLineBox,*contourLinesDistanceBox;
  QCheckBox *boxPieConterClockwise, *boxPieValues, *boxPiePercentages, *boxPieCategories;
  QWidget *pieLabelsPage;
  QSpinBox *boxPieOffset;
  QWidget *pieGeometryPage;
  ContourLinesEditor *contourLinesEditor;
  QRadioButton *customPenBtn;
  //static Mantid::Kernel::Logger &g_log;
};

/*****************************************************************************
 *
 * Class LayerItem
 *
 *****************************************************************************/
//! LayerItem tree widget item class
class LayerItem : public QTreeWidgetItem
{
public:
  enum {LayerTreeItem = 1001};
  LayerItem(Graph *g, QTreeWidgetItem *parent, const QString& s);

  Graph *graph() { return d_graph; };
  void setActive(bool select);

protected:
  void insertCurvesList();
  Graph *d_graph;
};

/*****************************************************************************
 *
 * Class CurveTreeItem
 *
 *****************************************************************************/
//! CurveTreeItem tree widget item class
class CurveTreeItem : public QTreeWidgetItem
{
public:
  enum {PlotCurveTreeItem = 1002};
  CurveTreeItem(QwtPlotItem *curve, LayerItem *parent, const QString& s);

  Graph* graph(){return ((LayerItem *)parent())->graph();};
  void setActive(bool on);

  QwtPlotItem *plotItem() const{ return d_curve; };
  int plotItemType();
  int plotItemIndex();

protected:
  QwtPlotItem *d_curve;
};

#endif
