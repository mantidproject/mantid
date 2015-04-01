/* vim: set filetype=cpp : */
/* due to a limitation of sip, this file has to use Unix style line endings */
/***************************************************************************
	File                 : qti.sip
	Project              : QtiPlot
--------------------------------------------------------------------
	Copyright            : (C) 2006 by Knut Franke, Ion Vasilief
	Email                : knut.franke@gmx.de, ion_vasilief@yahoo.fr
	Description          : Specifications for Python bindings

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

%Module _qti

%UnitCode
  #if defined(__GNUC__) && !(defined(__INTEL_COMPILER))
    #pragma GCC system_header
  #endif
%End

%Import QtCore/QtCoremod.sip
%Import QtGui/QtGuimod.sip
%Import mantidqt.sip

class MdiSubWindow: QFrame /PyName=MDIWindow/
{
%TypeHeaderCode
#include "src/MdiSubWindow.h"
%End
public:
  enum CaptionPolicy{Name = 0, Label = 1, Both = 2};

  SIP_PYOBJECT windowLabel();
%MethodCode
  sipRes = PyString_FromString(sipCpp->windowLabel());
%End
  void setWindowLabel(const QString&);

  CaptionPolicy captionPolicy();
  void setCaptionPolicy(CaptionPolicy);

  virtual void restore(const QStringList& );
  //Folder* folder();
  QString name();

  void confirmClose(bool);
  void show();
  void hide();
  bool close();

  void setFocus();

  void undock();
  bool isFloating() const;
  void dock();
  bool isDocked() const;

private:
  MdiSubWindow(const MdiSubWindow&);
};

typedef QList<MdiSubWindow*> MDIWindowList;

class Table: MdiSubWindow
{
%TypeHeaderCode
#include "src/Table.h"
#include <QMessageBox>
#define CHECK_TABLE_COL(arg)\
    int col;\
    if (PyInt_Check(arg)) {\
      col = (int)PyInt_AsLong(arg) - 1;\
		if (col < 0 || col >= sipCpp->numCols()) {\
			sipIsErr = 1;\
			PyErr_Format(PyExc_ValueError, "There's no column %d in table %s!", col+1, sipCpp->name().ascii());\
		}\
	 } else {\
      PyObject *tmp = PyObject_Str(arg);\
		if (!tmp) {\
			sipIsErr = 1;\
			PyErr_Format(PyExc_TypeError, "Column argument must be either int or string.");\
		} else {\
			col = sipCpp->colNames().findIndex(PyString_AsString(tmp));\
			if (col < 0) {\
				sipIsErr = 1;\
				PyErr_Format(PyExc_ValueError, "There's no column named %s in table %s!", PyString_AsString(tmp),\
						sipCpp->name().ascii());\
				Py_DECREF(tmp);\
			}\
		}\
	 }

#define CHECK_TABLE_ROW(arg)\
   int row = arg-1;\
	if (row < 0 || row >= sipCpp->numRows()) {\
		sipIsErr = 1;\
		PyErr_Format(PyExc_ValueError, "There's no row %d in table %s!", row+1, sipCpp->name().ascii());\
	}
%End

public:
  enum PlotDesignation{None = 0, X = 1, Y = 2, Z = 3, xErr = 4, yErr = 5, Label = 6};
  enum ImportMode{NewColumns, NewRows, Overwrite};

  int numRows() /PyName=numRows/;
  int numCols() /PyName=numCols/;
  void resizeRows(int) /PyName=setNumRows/;
  void resizeCols(int) /PyName=setNumCols/;

  void deleteRows(int, int);

  SIP_PYOBJECT text(SIP_PYOBJECT, int);
%MethodCode
  sipIsErr = 0;
  CHECK_TABLE_COL(a0);
  CHECK_TABLE_ROW(a1);
  if (sipIsErr == 0) {
		PyObject *encstr = PyString_FromString(sipCpp->text(row, col).utf8());
		if (encstr) {
			sipRes = PyUnicode_FromEncodedObject(encstr, "utf8", 0);
			Py_DECREF(encstr);
		} else {
			sipRes = NULL;
			sipIsErr = 1;
		}
	}
%End

  double cell(SIP_PYOBJECT, int);
%MethodCode
  sipIsErr = 0;
  CHECK_TABLE_COL(a0);
  CHECK_TABLE_ROW(a1);
	if (sipIsErr == 0){
		if (sipCpp->text(row, col) != "")
			sipRes = sipCpp->cell(row, col);
		else {
			sipRes = 0.0;
			PyErr_SetString(PyExc_ValueError, "Empty table cell");
		}
	}
%End
  void setText(SIP_PYOBJECT, int, const QString&);
%MethodCode
  sipIsErr = 0;
  CHECK_TABLE_COL(a0);
  CHECK_TABLE_ROW(a1);
	if (sipIsErr == 0)
		sipCpp->setText(row, col, *a2);
%End
  void setCell(SIP_PYOBJECT, int, double);
%MethodCode
  sipIsErr = 0;
  CHECK_TABLE_COL(a0);
  CHECK_TABLE_ROW(a1);
	if (sipIsErr == 0)
		sipCpp->setCell(row, col, a2);
%End
  SIP_PYOBJECT colName(int);
%MethodCode
   if (a0 < 1 || a0 > sipCpp->numCols()) {\
		sipIsErr = 1;\
		PyErr_SetString(PyExc_ValueError, "Invalid column argument");\
	} else
		sipRes = PyString_FromString(sipCpp->colLabel(a0-1));
%End
  void setColName(SIP_PYOBJECT, const QString&, bool=false);
%MethodCode
	sipIsErr = 0;
   CHECK_TABLE_COL(a0);
	if (sipIsErr == 0)
		sipCpp->setColName(col, *a1, a2);
%End
  void notifyChanges();

  void importASCII(const QString&, const QString&="\t", int=0, bool=false,
  					bool=true, bool=false, bool=false, const QString&="#", bool=false,
					ImportMode = Overwrite, int = 0, int = -1);
  bool exportASCII(const QString&, const QString&="\t", bool=false, bool=false, bool=false);
  void normalizeCol(SIP_PYOBJECT) /PyName=normalize/;
%MethodCode
	sipIsErr = 0;
	CHECK_TABLE_COL(a0);
	if (sipIsErr == 0)
		sipCpp->normalizeCol(col);
%End
  void normalize();

  void sortColumn(SIP_PYOBJECT, int order = 0);
%MethodCode
  	sipIsErr = 0;
  	CHECK_TABLE_COL(a0);
	if (sipIsErr == 0)
		sipCpp->sortColumn(col, a1);
%End
  void sort(int type = 0, int order = 0, const QString& leadCol = QString());
  void sortColumns(SIP_PYTUPLE, int=0, int=0, const QString&=QString());
%MethodCode
  QStringList l;
  int n = PyTuple_Size(a0);
  for (int i=0; i<n; i++) {
    PyObject *str = PyObject_Str(PyTuple_GET_ITEM(a0,i));
    if (str) {
      l << PyString_AsString(str);
		Py_DECREF(str);
	 } else {
      sipIsErr = 1;
      break;
    }
  }
  sipCpp->sortColumns(l, a1, a2, *a3);
%End

	void setCommand(SIP_PYOBJECT, const QString&);
%MethodCode
  	sipIsErr = 0;
  	CHECK_TABLE_COL(a0);
	if (sipIsErr == 0)
		sipCpp->setCommand(col, *a1);
%End

	void setComment(SIP_PYOBJECT, const QString&);
%MethodCode
  	sipIsErr = 0;
  	CHECK_TABLE_COL(a0);
	if (sipIsErr == 0)
		sipCpp->setColComment(col, *a1);
%End

    void setReadOnlyColumn(SIP_PYOBJECT, bool = true);
%MethodCode
  	sipIsErr = 0;
  	CHECK_TABLE_COL(a0);
	if (sipIsErr == 0)
		sipCpp->setReadOnlyColumn(col, a1);
%End

    void setColumnRole(SIP_PYOBJECT, PlotDesignation);
%MethodCode
  	sipIsErr = 0;
  	CHECK_TABLE_COL(a0);
	if (sipIsErr == 0){
		sipCpp->setColPlotDesignation(col, a1);
		sipCpp->setHeaderColType();
	}
%End

	void setColumnWidth(SIP_PYOBJECT, int);
%MethodCode
  	sipIsErr = 0;
  	CHECK_TABLE_COL(a0);
	if (sipIsErr == 0)
		sipCpp->setColumnWidth(col, a1);
%End

	void hideColumn(SIP_PYOBJECT, bool = true);
%MethodCode
  	sipIsErr = 0;
  	CHECK_TABLE_COL(a0);
	if (sipIsErr == 0)
		sipCpp->hideColumn(col, a1);
%End

private:
  Table(const Table&);
};

class Matrix: MdiSubWindow
{
%TypeHeaderCode
#include "src/Matrix.h"
#define CHECK_MATRIX_COL(arg)\
	int col = arg-1;\
   if (col < 0 || col >= sipCpp->numCols()) {\
		sipIsErr = 1;\
		PyErr_Format(PyExc_ValueError, "There's no column %d in matrix %s!", col+1, sipCpp->name().ascii());\
	}
#define CHECK_MATRIX_ROW(arg)\
   int row = arg-1;\
	if (row < 0 || row >= sipCpp->numRows()) {\
		sipIsErr = 1;\
		PyErr_Format(PyExc_ValueError, "There's no row %d in matrix %s!", row+1, sipCpp->name().ascii());\
	}
%End
public:
  enum HeaderViewType{ColumnRow, XY};
  enum ViewType{TableView, ImageView};
  enum ImportMode{NewColumns, NewRows, Overwrite};

  int numRows();
  void setNumRows(int);
  int numCols();
  void setNumCols(int);
  void setDimensions(int rows, int cols);

  SIP_PYOBJECT text(int, int);
%MethodCode
	sipIsErr = 0;
	CHECK_MATRIX_ROW(a0);
	CHECK_MATRIX_COL(a1);
	if (sipIsErr == 0)
		sipRes = PyString_FromString(sipCpp->text(row, col));
%End
  double cell(int, int);
%MethodCode
	sipIsErr = 0;
	CHECK_MATRIX_ROW(a0);
	CHECK_MATRIX_COL(a1);
	if (sipIsErr == 0)
		sipRes = sipCpp->cell(row, col);
%End
  void setText(int, int, const QString&);
%MethodCode
	sipIsErr = 0;
	CHECK_MATRIX_ROW(a0);
	CHECK_MATRIX_COL(a1);
	if (sipIsErr == 0)
		sipCpp->setText(row, col, *a2);
%End
  void setCell(int, int, double);
%MethodCode
	sipIsErr = 0;
	CHECK_MATRIX_ROW(a0);
	CHECK_MATRIX_COL(a1);
	if (sipIsErr == 0)
		sipCpp->setCell(row, col, a2);
%End

	double dx();
	double dy();
    double xStart();
	double xEnd();
	double yStart();
	double yEnd();
    void setCoordinates(double xs, double xe, double ys, double ye);

    void setFormula(const QString &);
    bool calculate(int startRow = 0, int endRow = -1, int startCol = 0, int endCol = -1, bool muParser = true);

    void setNumericPrecision(int prec);

    void transpose();
	void invert();
	void flipVertically();
	void flipHorizontally();
	void rotate90(bool = true);
	double determinant();
	double integrate();

	void setViewType(ViewType);
	void setHeaderViewType(HeaderViewType);
	void setGrayScale();
	void setRainbowColorMap();
	void resetView();

	QImage image();
	void importImage(const QString&);
	void exportRasterImage(const QString&, int = 100);
	void exportToFile(const QString&)  /PyName=export/;
	void exportVector(const QString&, int res = 0, bool color = true, bool keepAspect = true, QPrinter::PageSize = QPrinter::Custom);

	void importASCII(const QString&, const QString&="\t", int=0, bool=false, bool=false,
			const QString&="#", ImportMode = Overwrite, const QLocale& = QLocale(), int = 0, int = -1);
	bool exportASCII(const QString&, const QString&="\t", bool=false);

private:
  Matrix(const Matrix&);
};

class QwtData /PyName = CurveData/
{
%TypeHeaderCode
#include <qwt_data.h>
%End

public:
   // Note that the 'unsigned int' in this class should be size_t. But if you typedef size_t you'll get a crash
   //  at least on linux/gcc (and you'll still get warnings)

   virtual unsigned int size() const = 0;
   // this MethodCode is only needed for windoze, but apparently it's not possible to nest it inside an %If
   // Beware: the treatment of typedefs in sip changes with versions, so errors/warnings can vanish/show up anytime
   // in the future
%MethodCode
   size_t v = sipCpp->size();
   // sipRes will be declared as unsigned int (uint)
   sipRes = static_cast<unsigned int>(v);
%End
   virtual double x(unsigned int i) const = 0;
   virtual double y(unsigned int i) const = 0;

private:
   QwtData(const QwtData&);
};

class QwtSymbol /PyName = PlotSymbol/
{
%TypeHeaderCode
#include <qwt_symbol.h>
%End
public:
  enum Style
  {
    NoSymbol = -1,
    Ellipse,
    Rect,
    Diamond,
    Triangle,
    DTriangle,
    UTriangle,
    LTriangle,
    RTriangle,
    Cross,
    XCross,
    HLine,
    VLine,
    Star1,
    Star2,
    Hexagon,

    StyleCnt
  };

  QwtSymbol();
  QwtSymbol(Style st, const QBrush &bd, const QPen &pn, const QSize &s);

  void setSize(const QSize &s);
  void setSize(int a, int b = -1);
  void setBrush(const QBrush& b);
  void setPen(const QPen &p);
  void setStyle (Style s);

  const QBrush& brush() const;
  const QPen& pen() const;
  const QSize& size() const;
  Style style() const;

private:
  QwtSymbol(const QwtSymbol&);
};

class ImageSymbol : QwtSymbol
{
%TypeHeaderCode
#include "src/plot2D/ImageSymbol.h"
%End

public:
  ImageSymbol(const QString& fileName);
  ImageSymbol(const QPixmap& pixmap, const QString& fileName = QString());

  QPixmap pixmap();
  QString imagePath();

private:
  ImageSymbol(const ImageSymbol&);
};


class ArrowMarker // : QwtPlotMarker
{
%TypeHeaderCode
#include "src/ArrowMarker.h"
%End
public:

  ArrowMarker();

  void setStartPoint(double, double) /PyName=setStart/;
  void setEndPoint(double, double) /PyName=setEnd/;

  void setStyle(Qt::PenStyle);
  void setColor(const QColor&);
  void setWidth(double);
  void drawStartArrow(bool = true);
  void drawEndArrow(bool = true);
  void setHeadLength(int);
  void setHeadAngle(int);
  void fillArrowHead(bool = true);

private:
  ArrowMarker(const ArrowMarker&);
};

class ImageMarker // : QwtPlotMarker
{
%TypeHeaderCode
#include "src/ImageMarker.h"
%End
public:

  ImageMarker(const QString&);
  QString fileName();

  QSize size();
  void setSize(int, int);
  void setOrigin(int, int);
  void setRect(int, int, int, int);
  void setBoundingRect(double, double, double, double) /PyName=setCoordinates/;

private:
  ImageMarker(const ImageMarker&);
};

class LegendWidget: QWidget
{
%TypeHeaderCode
#include "src/LegendWidget.h"
%End
public:
    enum FrameStyle{None = 0, Line = 1, Shadow=2};

  void setText(const QString&);
  void setTextColor(const QColor&);
  void setFrameStyle(int);
  void setBackgroundColor(const QColor&);
  void setFont(const QFont&);
  void setOriginCoord(double, double);
  void setOrigin(int, int);

private:
  LegendWidget(const LegendWidget&);
};

class QwtPlotSpectrogram
{
%TypeHeaderCode
#include <qwt_plot_spectrogram.h>
%End
public:
  enum DisplayMode {
    ImageMode = 1,
    ContourMode = 2
  };

  void setDefaultContourPen (const QPen &);
  void setDisplayMode (DisplayMode mode, bool on = true);

private:
  QwtPlotSpectrogram(const QwtPlotSpectrogram&);
};

class Spectrogram : QObject, QwtPlotSpectrogram
{
%TypeHeaderCode
#include "src/Spectrogram.h"
%End
public:

  Matrix * matrix();

  int levels();
  void setLevelsNumber(int);
  void setContourLevels (SIP_PYTUPLE);
%MethodCode
  QList<double> l;
  int n = PyTuple_Size(a0);
  for (int i=0; i<n; i++) {
    PyObject *element = PyTuple_GET_ITEM(a0,i);
    if (element) {
    l << PyFloat_AS_DOUBLE(element);
    Py_DECREF(element);
   } else {
    sipIsErr = 1;
    break;
    }
  }
  sipCpp->setContourLevels(l);
%End

  bool hasColorScale();
  int colorScaleAxis();
  void showColorScale(int axis, bool on = true);

  int colorBarWidth();
  void setColorBarWidth(int width);

  void setGrayScale();
  void setDefaultColorMap();

  void loadColorMap(const QString& file) /PyName=changeColorMap/;

  bool hasLabels();
  void showContourLineLabels(bool show = true);

  QFont labelsFont();
  void setLabelsFont(const QFont& font);

  QColor labelsColor();
  void setLabelsColor(const QColor& c);

  bool labelsWhiteOut();
  void setLabelsWhiteOut(bool whiteOut = true);

  double labelsXOffset();
  double labelsYOffset();
  void setLabelsOffset(double x, double y);

  double labelsRotation();
  void setLabelsRotation(double angle);

  void setColorMapPen(bool on = true);
  void setContourLinePen(int, const QPen &);

private:
  Spectrogram(const Spectrogram&);
};

class QwtPlotCurve  // /PyName = PlotCurve/
{
%TypeHeaderCode
#include <qwt_plot_curve.h>
%End
public:
   QwtPlotCurve();

   double minXValue() const;
   double maxXValue() const;
   double minYValue() const;
   double maxYValue() const;
   const QwtData &data() const;

private:
   QwtPlotCurve(const QwtPlotCurve&);
};

class Grid : QObject // : QwtPlotGrid
{
%TypeHeaderCode
#include "src/Grid.h"
%End
public:

	Grid();

	void enableX(bool = true) /PyName=enableXMax/;
	bool xEnabled() /PyName=xMaxEnabled/;
	void enableXMin(bool = true);
	bool xMinEnabled();

	void enableY(bool = true) /PyName=enableYMax/;
	bool yEnabled() /PyName=yMaxEnabled/;
	void enableYMin(bool = true);
	bool yMinEnabled();

	bool xZeroLineEnabled();
	void enableZeroLineX(bool = true);
	bool yZeroLineEnabled();
	void enableZeroLineY(bool = true);

	void setMajPenX(const QPen &p);
	const QPen& majPenX();

	void setMinPenX(const QPen &p);
	const QPen& minPenX();

	void setMajPenY(const QPen &p);
	const QPen& majPenY();

	void setMinPenY(const QPen &p);
	const QPen& minPenY();

private:
	Grid(const Grid&);
};

namespace GraphOptions
{
%TypeHeaderCode
#include "MantidQtAPI/GraphOptions.h"
%End

  enum ScaleType {Linear, Log10};

  enum Axis{Left, Right, Bottom, Top};
};

class Graph : QWidget /PyName=Layer/
{
%TypeHeaderCode
#include "src/Graph.h"
#include "src/LegendWidget.h"
%End
public:
  enum TicksStyle{NoTicks = 0, Out = 1, InOut = 2, In = 3};
  enum CurveType{Line, Scatter, LineSymbols, VerticalBars, Area, Pie, VerticalDropLines,
			Spline, HorizontalSteps, Histogram, HorizontalBars, VectXYXY, ErrorBars,
			Box, VectXYAM, VerticalSteps, ColorMap, GrayScale, ColorMapContour, Contour, Function, ImagePlot};

  bool isPiePlot();
  SIP_PYOBJECT pieLegendText() /PyName=pieLegend/;
%MethodCode
  sipRes = PyString_FromString(sipCpp->pieLegendText());
%End

  bool insertCurve(Table*, const QString&, int style = 1, int startRow = 0, int endRow = -1);
  bool insertCurve(Table*, const QString&, const QString&, int style = 1, int startRow = 0, int endRow = -1);
  bool insertCurve(QString, int, bool=false, Graph::CurveType style = Graph::Unspecified);
  void insertCurve(Graph*, int);
  bool addCurves(Table*, SIP_PYTUPLE, int=0, double=1, int=3, int=0, int=-1);
%MethodCode
  QStringList l;
  int n = PyTuple_Size(a1);
  for (int i=0; i<n; i++) {
    PyObject *str = PyObject_Str(PyTuple_GET_ITEM(a1,i));
    if (str) {
      l << PyString_AsString(str);
		Py_DECREF(str);
	 } else {
      sipIsErr = 1;
      break;
    }
  }
  sipRes = sipCpp->addCurves(a0, l, a2, a3, a4, a5, a6);
%End
  bool addCurve(Table*, const QString&, int=0, double=1, int=3, int=0, int=-1);
%MethodCode
  QStringList l;
  l << *a1;
  sipRes = sipCpp->addCurves(a0, l, a2, a3, a4, a5, a6);
%End
  void removeCurve(int);
  void removeCurve(const QString&);
  int curves() /PyName=numCurves/;
  QwtPlotCurve* curve(int index);
  QString curveTitle(int);

  void setCurveLineColor(int, int);
  void setCurveLineColor(int, QColor);
  void setCurveLineStyle(int, Qt::PenStyle);
  void setCurveLineWidth(int, double);
  void setGrayScale();
  void setIndexedColors();

  void setCurveSymbol(int, const QwtSymbol&);
  void setCurvePen(int, const QPen&);
  void setCurveBrush(int, const QBrush&);
  void setCurveSkipSymbolsCount(int, int);

	void addFunction(const QString&, double, double, int=100);
%MethodCode
  QStringList l;
  l << *a0;
  sipCpp->addFunction(l, a1, a2, a3);
%End
	void addPolarFunction(const QString&, const QString&, double, double, int=100, const QString & = "t");
%MethodCode
  QStringList l;
  l << *a0;
  l << *a1;
  sipCpp->addFunction(l, a2, a3, a4, *a5, 2);
%End
	void addParametricFunction(const QString&, const QString&, double, double, int=100, const QString & = "m");
%MethodCode
  QStringList l;
  l << *a0;
  l << *a1;
  sipCpp->addFunction(l, a2, a3, a4, *a5, 1);
%End

  void addErrorBars(const QString&, Table *, const QString&,
                    int type = 1, double width = 1, int cap = 8, const QColor& color = QColor(Qt::black),
                    bool through = false, bool minus = true, bool plus = true);

  ErrorBarSettings* errorBarSettings(int curveIndex, int errorBarIndex = 0);

  void addHistogram(Matrix*);
  void addArrow(ArrowMarker*);

  void addImage(ImageMarker*);
  void addImage(const QString& fileName);

  void setTitle(const QString& t);
  void setTitleFont(const QFont &fnt);
  void setTitleColor(const QColor &c);
  void setTitleAlignment(int align);
  void removeTitle();

  LegendWidget* newLegend(const QString& = QString());
  void setLegend(const QString&);
%MethodCode
  sipCpp->legend()->setText(*a0);
%End

  LegendWidget* legend();

  void removeLegend();
  void addTimeStamp();

  void setXAxisTitle(const QString& text) /PyName=setXTitle/;
  void setYAxisTitle(const QString& text) /PyName=setYTitle/;

  void enableAxis(int axis, bool on = true);
  void setAxisColor(int axis, const QColor& color);
  void setAxisFont(int axis, const QFont &fnt);

  void setAxisTitle(int axis, const QString& text);
  void setAxisTitleFont(int axis,const QFont &fnt);
  void setAxisTitleColor(int axis, const QColor& c);
  void setAxisTitleAlignment(int axis, int align);

  void setAxisTicksLength(int axis, int majTicksType, int minTicksType, int minLength, int majLength);
  void setAxisLabelRotation(int axis, int rotation);

  void setAxesLinewidth(int width);
  void drawAxesBackbones(bool yes);
  void setTicksLength(int minLength, int majLength);

  void enableAxisLabels(int axis, bool on);
  void setAxisLabelsColor(int axis, const QColor& color);
  void setLabelsNumericFormat(int axis, int format, int = 6, const QString& = QString()) /PyName=setAxisNumericFormat/;
  void setScale(int axis, double start, double end, double step = 0.0,
                int majorTicks = 5, int minorTicks = 5, int type = 0, bool inverted = false,
				double left_break = -DBL_MAX, double right_break = DBL_MAX, int pos = 50,
				double stepBeforeBreak = 0.0, double stepAfterBreak = 0.0, int minTicksBeforeBreak = 4,
				int minTicksAfterBreak = 4, bool log10AfterBreak = false, int breakWidth = 4, bool breakDecoration = true);

  void logLogAxes();
  void logXLinY();
  void logYlinX();
  void linearAxes();
  void logColor();
  void linColor();

  void setAxisScale(int, double, double, int = -1, double=0.0, int=5, int=5);
  void setAutoScale();
  void setMargin(int);
  void setFrame(int width = 1, const QColor& color = QColor(Qt::black));
  void setCanvasFrame(int width = 1, const QColor& color =  QColor(Qt::black));
  void setBackgroundColor(const QColor& color);
  void setCanvasBackground(const QColor& color) /PyName=setCanvasColor/;

  void setCurveTitle(int, const QString &);

  Grid* grid();
  void showGrid(int);
  void showGrid();

  void replot();

  void exportImage(const QString& fileName, int quality = 100, bool transparent = false);
  void exportVector(const QString& fileName, int res = 0, bool color = true,
                    bool keepAspect = true, QPrinter::PageSize pageSize = QPrinter::Custom);
  void exportToFile(const QString& fileName) /PyName=export/;

  void enableAutoscaling(bool = true);
  void setIgnoreResizeEvents(bool = true)/PyName=setIgnoreResize/;
  void setAutoscaleFonts(bool = true);
  void setAntialiasing(bool on = true, bool update = true);

  void setCurveAxes(int,int,int);
%MethodCode
    sipIsErr = 0;
    if ((a1)>1 || (a1<0) || (a2>1) || (a2<0))
    {
      sipIsErr=1;
      PyErr_Format(PyExc_ValueError, "Invalid axis attachment!");\
    }
    if (a0>=sipCpp->curves() || a0<0)
    {
      sipIsErr=1;
      PyErr_Format(PyExc_ValueError, "Invalid curve index! is %d, should be less than %d",a0,sipCpp->curves());\
    }
    if (sipIsErr == 0)
    {
      QwtPlotItem* pItem;
      pItem = sipCpp->plotItem(a0);
      pItem->setAxis(a1 + 2, a2);
      sipCpp->setAutoScale();
    }
%End

  Spectrogram* spectrogram();

  void updateWaterfallFill(bool on = true) /PyName=enableWaterfallFill/;
  void setWaterfallFillColor(const QColor&);
  void setWaterfallOffset(int x, int y, bool = true);

private:
  Graph(const Graph&);
};

class MultiLayer : MdiSubWindow /PyName=Graph/
{
%TypeHeaderCode
#include "src/MultiLayer.h"
%End
public:

  enum HorAlignement{HCenter, Left, Right};
  enum VertAlignement{VCenter, Top, Bottom};

  Graph *activeGraph() /PyName=activeLayer/;
  void setActiveGraph(Graph*) /PyName=setActiveLayer/;
  int layers() /PyName=numLayers/;
  void setLayersNumber(int n) /PyName=setNumLayers/;
  Graph* layer(int num);
  Graph* addLayer(int = 0, int = 0, int = 0, int = 0);
  void insertCurve(MultiLayer* ml, int i);
  void setCols(int);
  void setRows(int);
  void setName(const QString&);
  void setSpacing (int, int);
  void setMargins (int, int, int, int);
  void setLayerCanvasSize (int, int);
  void setAlignement (int, int);
  void arrangeLayers(bool fit = true, bool userSize = false);
  bool swapLayers(int, int);

  void exportToFile(const QString& fileName) /PyName=export/;
  void exportImage(const QString& fileName, int quality = 100, bool transparent = false);
  void exportVector(const QString& fileName, int res = 0, bool color = true,
                    bool keepAspect = true, QPrinter::PageSize pageSize = QPrinter::Custom);

  void reverseWaterfallOrder();

private:
  MultiLayer(const MultiLayer&);
};

class ErrorBarSettings : QObject
{
%TypeHeaderCode
#include "src/Mantid/ErrorBarSettings.h"
%End
public:
  int capLength();
  void setCapLength(int t);

  double width();
  void setWidth(double w);

  QColor color();
  void setColor(const QColor& c);

  bool throughSymbol();
  void drawThroughSymbol(bool);

  bool plusSide();
  void drawPlusSide(bool);

  bool minusSide();
  void drawMinusSide(bool);

private:
  ErrorBarSettings(const ErrorBarSettings&);
};

class Note: MdiSubWindow
{
%TypeHeaderCode
#include "src/Note.h"
%End
public:
  QString text();
  void setText(const QString &s);

  QString exportASCII(const QString &file=QString::null);

private:
  Note(const Note&);
};

class Graph3D: MdiSubWindow
{
%TypeHeaderCode
#include "src/Graph3D.h"
%End
public:
	enum PlotType{Scatter = 0, Trajectory = 1, Bars = 2, Ribbon = 3};

    void exportToFile(const QString& fileName) /PyName=export/;
    void exportVector(const QString& fileName);
    void exportImage(const QString& fileName, int quality = 100, bool transparent = false);

    void setRotation(double xVal, double yVal, double zVal);
	void setScale(double xVal, double yVal, double zVal);
	void setShift(double xVal, double yVal, double zVal);
    void setZoom(double val);
    void setOrthogonal(bool = true);
    void setMeshLineWidth(int);
	void setMeshColor(const QColor&);
	void setAxesColor(const QColor&);
	void setNumbersColor(const QColor&);
	void setLabelsColor(const QColor&);
    void setLabelsDistance(int);
	void setBackgroundColor(const QColor&);
	void setGridColor(const QColor&);
    void setDataColors(const QColor& cMax, const QColor& cMin);
    void setDataColorMap(const QString& fileName);
    void changeTransparency(double) /PyName=setOpacity/;
    void setResolution(int);
    void showColorLegend(bool = true) /PyName=showLegend/;
    void setTitle(const QString&, const QColor& color = QColor(Qt::black), const QFont& font = QFont());
    void setAntialiasing(bool = true);
    void setLeftGrid(bool = true);
	void setRightGrid(bool = true);
	void setCeilGrid(bool = true);
	void setFloorGrid(bool = true);
	void setFrontGrid(bool = true);
	void setBackGrid(bool = true);

    void setFramed();
	void setBoxed();
	void setNoAxes();

	void setFloorData() /PyName=showFloorProjection/;
	void setFloorIsolines() /PyName=showFloorIsolines/;
	void setEmptyFloor();

    void setPolygonStyle();
	void setHiddenLineStyle();
	void setWireframeStyle();
	void setFilledMeshStyle();
	void setDotStyle();
	void setBarStyle();
    void setConeStyle();
    void setCrossStyle();

    void setConeOptions(double rad, int quality);
    void setCrossOptions(double rad, double linewidth, bool smooth, bool boxed);
    void setDotOptions(double size, bool smooth);
    void setBarRadius(double rad);

    void animate(bool = true);
    void findBestLayout();

    void changeDataColumn(Table* table, const QString& colName, int = 0) /PyName=setData/;
    void addMatrixData(Matrix* ) /PyName=setMatrix/;
    void addFunction(const QString&, double, double, double, double, double, double, int = 40, int = 40) /PyName=setFunction/;
    void addParametricSurface(const QString&, const QString&, const QString&, double,
                        double, double, double, int=40, int=40, bool=true, bool=true) /PyName=setParametricSurface/;

    void update();

    void setXAxisLabel(const QString&);
    void setYAxisLabel(const QString&);
	void setZAxisLabel(const QString&);

	void setXAxisTickLength(double majorLength, double minorLength);
    void setYAxisTickLength(double majorLength, double minorLength);
	void setZAxisTickLength(double majorLength, double minorLength);

    void setScales(double xl, double xr, double yl, double yr, double zl, double zr);

private:
  Graph3D(const Graph3D&);
};

class TiledWindow: MdiSubWindow
{
%TypeHeaderCode
#include "src/TiledWindow.h"
%End
public:
    int rowCount() const;
    int columnCount() const;
    void addWidget(MdiSubWindow *widget, int row, int col);
    void insertWidget(MdiSubWindow *widget, int row, int col);
    MdiSubWindow *getWidget(int row, int col);
    void removeWidgetToDocked(int row, int col);
    void removeWidgetToFloating(int row, int col);
    void selectWidget(int row, int col);
    void deselectWidget(int row, int col);
    bool isSelected(int row, int col);
    void selectRange(int row1, int col1,int row2, int col2);
    void clearSelection();
    void removeSelectionToDocked();
    void removeSelectionToFloating();
    void clear();
    void reshape(int cols);
private:
    TiledWindow(const TiledWindow&);
};

class ApplicationWindow: QMainWindow
{
%TypeHeaderCode
#include "src/ApplicationWindow.h"
%End
%ConvertToSubClassCode
// we have to do this to override casting in qt/qobject.sip (PyQt 3.16)
// Russell Taylor, 4/8/09: Change needed for mac build.
//   Taken from main qtiplot svn revision 1187.
sipClass=sipFindClass(sipCpp->className());
%End

public:
  void setExitCode(int code);
  bool runPythonScript(const QString & code, const bool async = false,
      bool quiet=false, bool redirect=true);

  enum MatrixToTableConversion{Direct, XYZ, YXZ};
  Table* table(const QString&);
%MethodCode
  sipRes = sipCpp->currentFolder()->table(*a0, false);
  if(!sipRes)
    sipRes = sipCpp->projectFolder()->table(*a0, true);
%End
  Table* newTable();
  Table* newTable(const QString&, int=30, int=2);
%MethodCode
  sipRes = sipCpp->newTable(*a0, a1, a2);
%End
  Matrix* matrix(const QString&);
%MethodCode
  sipRes = sipCpp->currentFolder()->matrix(*a0, false);
  if(!sipRes)
    sipRes = sipCpp->projectFolder()->matrix(*a0, true);
%End
  Matrix* newMatrix();
  Matrix* newMatrix(const QString&, int=32, int=32);
  
  TiledWindow *newTiledWindow();
  
  MultiLayer *plot(const QString&) /PyName=graph/;
%MethodCode
  sipRes = sipCpp->currentFolder()->graph(*a0, false);
  if(!sipRes)
    sipRes = sipCpp->projectFolder()->graph(*a0, true);
%End
  MultiLayer* newGraph();
  MultiLayer* multilayerPlot(const QString&, int, int, int)/PyName=newGraph/;

  Note *note(const QString&);
%MethodCode
  sipRes = sipCpp->currentFolder()->note(*a0, false);
  if(!sipRes)
    sipRes = sipCpp->projectFolder()->note(*a0, true);
%End
  Note* newNote(const QString& = QString::null);

  MultiLayer *multilayerPlot(Table*, SIP_PYTUPLE, int=1) /PyName=plot/;
%MethodCode
  QStringList l;
  int n = PyTuple_Size(a1);
  for (int i=0; i<n; i++) {
    PyObject *str = PyObject_Str(PyTuple_GET_ITEM(a1,i));
    if (str) {
      l << PyString_AsString(str);
		Py_DECREF(str);
	 } else {
      sipIsErr = 1;
      break;
    }
  }
  sipRes = sipCpp->multilayerPlot(a0, l, a2);
%End
  MultiLayer *multiLayerPlot(Table*, const QString&, int=1) /PyName=plot/;
%MethodCode
  QStringList l;
  l << *a1;
  sipRes = sipCpp->multilayerPlot(a0, l, a2);
%End

  MultiLayer *waterfallPlot(Table*, SIP_PYTUPLE);
%MethodCode
  if (a0 == 0) {
     sipIsErr = 1;
     PyErr_Format(PyExc_ValueError, "Invalid table in argument to waterfallPlot().");
  } else {
    QStringList l;
    int n = PyTuple_Size(a1);
    for (int i=0; i<n; i++) {
    PyObject *str = PyObject_Str(PyTuple_GET_ITEM(a1,i));
    if (str) {
      l << PyString_AsString(str);
      Py_DECREF(str);
     } else {
      sipIsErr = 1;
      break;
    }
    }
    sipRes = sipCpp->waterfallPlot(a0, l);
  }
%End

	Matrix* importImage(const QString&);
	MultiLayer* plotSpectrogram(Matrix*, Graph::CurveType = Graph::ColorMap) /PyName=plot/;

    Graph3D* newPlot3D();
    Graph3D* plotXYZ(Table* table, const QString&, int = 0) /PyName=plot3D/;
	Graph3D* plot3DMatrix(Matrix *, int style = 5) /PyName=plot3D/;
    Graph3D* plotSurface(const QString&, double xl, double xr, double yl, double yr,
			double zl, double zr, int=40, int=40) /PyName=plot3D/;
    Graph3D* plotParametricSurface(const QString&, const QString&, const QString&,
            double, double, double, double, int=40, int=40, bool=true, bool=true) /PyName=plot3D/;

  MDIWindowList windowsList() /PyName=windows/;

  // Required for setting the IPython console
  QDockWidget _getInterpreterDock();
%MethodCode
  sipRes = sipCpp->m_interpreterDock;
%End

  // folders
  Folder *activeFolder();
%MethodCode
  sipRes = sipCpp->currentFolder();
%End

  Folder* appendProject(const QString& file_name, Folder* parentFolder = 0);
  Folder* projectFolder() /PyName=rootFolder/;

  Folder* addFolder(QString name, Folder* parent = 0);
  bool deleteFolder(Folder *);
  bool changeFolder(Folder *, bool force = false);
  bool copyFolder(Folder *src, Folder *dest);

  void setWindowName(MdiSubWindow *, const QString &);
  void setPreferences(Graph*);

  void selectMultiPeak(MultiLayer *, bool = true, double = 0, double = 0);
  void disableTools();

  void saveProjectAs(const QString& fileName = QString(), bool = false);
  MdiSubWindow* clone(MdiSubWindow*);

  Matrix* tableToMatrix(Table* t);
  Table* matrixToTable(Matrix* m, MatrixToTableConversion = Direct);

  QString stemPlot(Table *t, const QString& colName, int power = 1001, int startRow = 0, int endRow = -1);

  //---- Mantid
  MantidUI* mantidUI;
  void addUserMenu(const QString &);
  void addUserMenuAction(const QString &, const QString &, const QString &);

  void setConfirmFolderClose(bool value);

  void setToolbarsVisible(bool visible);

  //--------------
private:
  ApplicationWindow(const ApplicationWindow&);
};

%ModuleCode
ApplicationWindow *sipqti_app()
{
  int iserr = 0;
  PyObject *me = PyImport_ImportModule("_qti");
  PyObject *mydict = PyModule_GetDict(me);
  PyObject *pyapp = PyDict_GetItemString(mydict,"app");
  Py_DECREF(me);
  if (sipCanConvertToInstance(pyapp, sipClass_ApplicationWindow, SIP_NOT_NONE))
    return (ApplicationWindow*) sipConvertToInstance(pyapp, sipClass_ApplicationWindow, NULL, SIP_NOT_NONE, NULL, &iserr);
  else
    return NULL;
}
%End
%ModuleHeaderCode
class ApplicationWindow;
ApplicationWindow *sipqti_app();
#define SIPQTI_APP(sipcppexpr)\
ApplicationWindow *app = sipqti_app();\
if (app) sipCpp = sipcppexpr;\
else { sipCpp = NULL; }
%End


// used for output redirection
class PythonScripting
{
%TypeHeaderCode
#include "src/PythonScripting.h"
%End
public:
  void write(const QString&);
  // 'Fake' methods needed for IPython import
  void flush();
  void set_parent(SIP_PYOBJECT);
private:
  PythonScripting(const PythonScripting&);
};
class PythonScript
{
%TypeHeaderCode
#include "src/PythonScript.h"
%End
public:
  void write(const QString&);
  // 'Fake' method needed for IPython import
  void flush();
  // Line number trace
  void lineNumberChanged(SIP_PYOBJECT,int);
private:
  PythonScript(const PythonScript&);
};

class Folder : QObject
{
%TypeHeaderCode
#include "src/Folder.h"
%End
public:
  MDIWindowList windowsList() /PyName=windows/;
// TODO: implement signal Folder::nameChanged and make it update the project explorer; adjust renaming from GUI accordingly
//  void setFolderName(const QString&) /PyName=setName/;
  QString name();
  QString path();

  QList<Folder*> folders();
  Folder *findSubfolder(const QString&, bool=true, bool=false)  /PyName=folder/;
  MdiSubWindow* findWindow(const QString&, bool=true, bool=true, bool=false, bool=true);

  MdiSubWindow *window(const QString &name, const char *cls="MdiSubWindow", bool recursive=false);
  Table *table(const QString &name, bool recursive=false);
  Matrix *matrix(const QString &name, bool recursive=false);
  MultiLayer *graph(const QString &name, bool recursive=false);

  Folder* rootFolder();

private:
  Folder(const Folder&);
};

//-------------------
// Mantid Extensions
//-------------------

class MantidMatrix: MdiSubWindow
{
%TypeHeaderCode
#include "src/Mantid/MantidMatrix.h"
%End
public:
  int numRows() const;
  int numCols() const;
  double dataX(int,int) const;
  double dataY(int,int) const;
  double dataE(int,int) const;
  //int indexX(int,double) const;
  Graph3D* plotGraph3D(int = 3);
  MultiLayer* plotGraph2D(Graph::CurveType = Graph::ColorMap);
  void goToTab(const QString &);
  void goTo(int,int) /PyName = selectCell/;
  void goToRow(int) /PyName = selectRow/;
  void goToColumn(int) /PyName = selectColumn/;
  void closeDependants();
private:
  MantidMatrix(const MantidMatrix&);
};

class InstrumentWindowTab: QFrame
{
%TypeHeaderCode
#include "src/Mantid/InstrumentWidget/InstrumentWindowTab.h"
%End
private:
  InstrumentWindowTab(const InstrumentWindowTab &);
};

class InstrumentWindow: MdiSubWindow
{
%TypeHeaderCode
#include "src/Mantid/InstrumentWidget/InstrumentWindow.h"
%End

%Docstring
    The InstrumentWindow contains functionality for exploring
    a 3D model of an instrument. It is a tabbed widget offering
    rendering, picking, masking and grouping functions. There is
    also a tree view of the instrument hierarchy.

    For further help see the functions defined on each of the following
        - InstrumentWindowRenderTab
        - InstrumentWindowPickTab
        - InstrumentWindowMaskTab
%End

public:
  enum SurfaceType { FULL3D, CYLINDRICAL_X, CYLINDRICAL_Y, CYLINDRICAL_Z,
                     SPHERICAL_X, SPHERICAL_Y, SPHERICAL_Z,
                     RENDERMODE_SIZE };
  enum Tab { RENDER, PICK, MASK, TREE };


  InstrumentWindowTab * getTab(const QString & title) const;
%Docstring
    Returns a handler to the requested tab

    Args:

        title The full title of a tab in the window

    Returns:
        a pointer to the requested tab widget

%End

  InstrumentWindowTab * getTab(const Tab tab) const;
%Docstring
    Returns a handler to the requested tab

    Args:
        tab One of the Tab enumeration types:
	    InstrumentWindow.RENDER,InstrumentWindow.PICK,
	    InstrumentWindow.MASK,InstrumentWindow.TREE

    Returns:
        a pointer to the requested tab widget

%End

  void setBinRange(double min_value, double max_value);
%Docstring
    Updates the integration range over which the colours
    are calculated

    Args:
        min_value The minimum value over which the data is integrated
        max_value The maximum value over which the data is integrated

%End

  bool overlay(const QString & wsName);
%Docstring
    Overlays a workspace onto an unwrapped surface on the instrument view.
    Raises a dialog box if the current surface is not unwrapped.

    Args:
        wsName The name of the workspace

    Returns:
        A boolean indicating if the overlay was successful or not

%End

  // -- Deprecated in favour of specific tab functions --
  // Deprecation is done in mantidplot.py module rather than using
  // sip /Deprecated/annotation so that the message can be customised
  void changeColormap();
  void changeColormap(const QString & file);
  void setColorMapMinValue(double);
  void setColorMapMaxValue(double);
  void setColorMapRange(double, double);
  void selectComponent(const QString &);
  void setScaleType(GraphOptions::ScaleType);
  void setViewType(const QString &);

private:
InstrumentWindow();
InstrumentWindow(const InstrumentWindow &);
};

class InstrumentWindowRenderTab: InstrumentWindowTab
{
%TypeHeaderCode
#include "src/Mantid/InstrumentWidget/InstrumentWindowRenderTab.h"
%End

public:

  GraphOptions::ScaleType getScaleType() const;
%Docstring
    Returns the current scale type. 0=Linear, 1=Log10
%End

  void setScaleType(GraphOptions::ScaleType type);
%Docstring
    Set the scale type for the colour bar.

    Args:
        type The new type Options are: GraphOptions.Linear, GraphOptions.Log10
%End

  void setAxis(const QString& axisName);
%Docstring
    Set the current viewing axis by name.

    Args:
       axisName The name of a view in a string: X+,X-,Y+,Y-,Z+,Z-
%End

  void setMinValue(double value, bool apply = true);
%Docstring
    Set the minimum value for the colour scale

    Args:
       value The new value for the minimum
%End

  void setMaxValue(double value, bool apply = true);
%Docstring
    Set the maximum value for the colour scale

    Args:
       value The new value for the maximum
%End

  void setRange(double minValue, double maxValue, bool apply = true);
%Docstring
    Set the range of the colour bar

    Args:
        minValue The new value for the minimum
        maxValue The new value for the maximum
%End

  void showAxes(bool on);
%Docstring
    Set the axes on/off

    Args:
        on True/False indicating whether the axes are visible
%End

  void setColorMapAutoscaling(bool on);
%Docstring
    Set whether the colour scale should auto scale when the data is updated

    Args:
        on True/False indicating whether auto scaling is active
%End

  void displayDetectorsOnly(bool yes);
%Docstring
    Set whether only detectors should be shown.

    Args:
        yes True/False indicating whether only detectors are shown
%End

  void enableGL(bool on);
%Docstring
    Toggle the use of OpenGL. This can only be used for the unwrapped views.

    Args:
        on True/False indicating whether OpenGL should be used.
%End

  void changeColorMap(const QString & filename = "");
%Docstring
    Change the color map for the given one. If blank then a dialog is raised

    Args:
        filename A filename giving the full path to a color map file
%End

  void setSurfaceType(int type);
%Docstring
    Set the surface type of the current window.

    Args:
        type A known suface type: FULL3D, CYLINDRICAL_X, CYLINDRICAL_Y,
                                  CYLINDRICAL_Z,SPHERICAL_X, SPHERICAL_Y,
                                  SPHERICAL_Z
%End

  void flipUnwrappedView(bool on);
%Docstring
    Set whether an unwrapped view is flipped

    Args:
        on True/False indicatingif to flip the view
%End

  void saveImage(QString filename = "");
%Docstring
    Save the current display buffer to an image file. The format is chosen by the filename extension and must
    be one of .bmp,.jpeg,.jpeg,.png,.ppm,.tiff,.xbm,.xpm

    Args:
        filename The filename for the saved image. An empty string raises a dialog box.
%End

private:
  InstrumentWindowRenderTab();
  InstrumentWindowRenderTab(const InstrumentWindowRenderTab &);
};

class InstrumentWindowPickTab: InstrumentWindowTab
{
%TypeHeaderCode
#include "src/Mantid/InstrumentWidget/InstrumentWindowPickTab.h"
%End

public:
  enum ToolType {Zoom, PixelSelect, TubeSelect, PeakSelect, PeakErase};

  void selectTool(const ToolType tool);
%Docstring
    Select the active tool.

    Args:
        tool One of InstrumentWindowPickTab.{Zoom, PixelSelect, TubeSelect, PeakSelect, PeakErase}
%End

private:
  InstrumentWindowPickTab();
  InstrumentWindowPickTab(const InstrumentWindowPickTab &);
};

class InstrumentWindowMaskTab: InstrumentWindowTab
{
%TypeHeaderCode
#include "src/Mantid/InstrumentWidget/InstrumentWindowMaskTab.h"
%End

public:
  enum Mode {Mask, Group};
  enum Activity {Move,Select,DrawEllipse,DrawRectangle,DrawEllipticalRing,DrawRectangularRing};

  void setMode(Mode mode);
%Docstring
  Set the mode to either group/mask

  Args:
      mode One of the Mode enumerations InstrumentWindowMaskTab.{Mask, Group}
%End

  void selectTool(Activity tool);
%Docstring
  Set the mode to either group/mask

  Args:
      tool One of the Activity enumerations InstrumentWindowMaskTab.{Move,Select,DrawEllipse,DrawRectangle,DrawEllipticalRing,DrawRectangularRing}
%End

private:
  InstrumentWindowMaskTab();
  InstrumentWindowMaskTab(const InstrumentWindowMaskTab &);
};

class InstrumentWindowTreeTab: InstrumentWindowTab
{
%TypeHeaderCode
#include "src/Mantid/InstrumentWidget/InstrumentWindowTreeTab.h"
%End

public:
  void selectComponentByName(const QString& name);
%Docstring
    Sets the tree tab to center the view on the named component

    Args:
        name The name of a component in the instrument
%End

private:
  InstrumentWindowTreeTab();
  InstrumentWindowTreeTab(const InstrumentWindowTreeTab &);
};

class MantidUI: QObject
{
%TypeHeaderCode
#include "src/Mantid/MantidUI.h"
%End
%ConvertToSubClassCode
// we have to do this to override casting in qt/qobject.sip (PyQt 3.16)
// Russell Taylor, 4/8/09: Change needed for mac build.
//   Taken from main qtiplot svn revision 1187.
sipClass=sipFindClass(sipCpp->className());
%End

public:
  // Plotting methods
  MultiLayer* plot1D(const QList<QString>& wsnames, const QList<int>&, bool spectrumPlot, bool errs=false,
    Graph::CurveType style = Graph::Unspecified, MultiLayer* window = NULL, bool clearWindow = false);

  MultiLayer* drawSingleColorFillPlot(const QString & name, Graph::CurveType style = Graph::ColorMap,
                                      MultiLayer* window = NULL);

  MultiLayer* plotMDList(const QList<QString>& wsNames, const int plotAxis,
    const Mantid::API::MDNormalization normalization, const bool showErrors, MultiLayer* window = NULL,
    bool clearWindow = false);

  MultiLayer* mergePlots(MultiLayer*, MultiLayer*);
  void convertToWaterfall(MultiLayer*);

  // Methods relating to creating or getting handles to GUI objects
  MantidMatrix* getMantidMatrix(const QString &);
  QString getSelectedWorkspaceName();
  InstrumentWindow* getInstrumentView(const QString &, int tab = -1);
%MethodCode
  sipRes = sipCpp->getInstrumentView(*a0,a1);
  if(sipRes) sipRes->hide();
%End
  MantidMatrix* importMatrixWorkspace(const QString&,int = -1, int = -1, bool = false, bool = false);
  Table* createTableFromSpectraList(const QString& tableName, const QString& workspaceName,
      QList<int> indexList, bool errs=true, bool binCentres=false) /PyName = workspaceToTable/;
  Table* importTableWorkspace(const QString&, bool = false, bool = false);

  void setIsRunning(bool running);
  bool createScriptInputDialog(const QString &, const QString &, const QString&, const QStringList&, const QStringList&);

   MantidQt::MantidWidgets::FitPropertyBrowser* fitFunctionBrowser();
private:
  MantidUI(const MantidUI &);
};
