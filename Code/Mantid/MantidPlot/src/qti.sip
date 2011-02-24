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

%Module qti 0
%Import QtCore/QtCoremod.sip
%Import QtGui/QtGuimod.sip

class MdiSubWindow: QMdiSubWindow /PyName=MDIWindow/
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

  virtual SIP_PYOBJECT saveAsTemplate(const QString& );
%MethodCode
  sipRes = PyString_FromString(sipCpp->saveAsTemplate(*a0));
%End
  virtual void restore(const QStringList& );
  Folder* folder();

  void askOnCloseEvent(bool)/PyName=confirmClose/;

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
			sipRes = NULL;
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

class LegendWidget // : QWidget /PyName=Legend/
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

class Grid // : QwtPlotGrid
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
#include "src/GraphOptions.h"
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
			Box, VectXYAM, VerticalSteps, ColorMap, GrayScale, Contour, Function, ImagePlot};

  bool isPiePlot();
  SIP_PYOBJECT pieLegendText() /PyName=pieLegend/;
%MethodCode
  sipRes = PyString_FromString(sipCpp->pieLegendText());
%End

  bool insertCurve(Table*, const QString&, const QString&, int=1);
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
  void deleteFitCurves();
  int curves() /PyName=numCurves/;

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
		  bool through = true, bool minus = true, bool plus = true);

  void addHistogram(Matrix*);
  void addArrow(ArrowMarker*);

  ImageMarker* addImage(ImageMarker*);
  ImageMarker* addImage(const QString& fileName);

  void setTitle(const QString& t);
  void setTitleFont(const QFont &fnt);
  void setTitleColor(const QColor &c);
  void setTitleAlignment(int align);
  void removeTitle();

  LegendWidget* newLegend(const QString& = QString());
  void setLegend(const QString&) /NoDerived/;
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

private:
  MultiLayer(const MultiLayer&);
};

class Note: MdiSubWindow
{
%TypeHeaderCode
#include "src/Note.h"
%End
public:
  void execute() const;
  void executeAll();
  void evaluate();

  bool autoexec() const;
  void setAutoexec(bool = true);

  QString text();
  void setText(const QString &s);

  void exportPDF(const QString& fileName);
  QString exportASCII(const QString &file=QString::null);
  QString importASCII(const QString &file=QString::null);

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
  enum MatrixToTableConversion{Direct, XYZ, YXZ};
  Table* table(const QString&);
%MethodCode
  sipRes = sipCpp->current_folder->table(*a0, false);
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
  sipRes = sipCpp->current_folder->matrix(*a0, false);
  if(!sipRes)
    sipRes = sipCpp->projectFolder()->matrix(*a0, true);
%End
  Matrix* newMatrix();
  Matrix* newMatrix(const QString&, int=32, int=32);
  MultiLayer *plot(const QString&) /PyName=graph/;
%MethodCode
  sipRes = sipCpp->current_folder->graph(*a0, false);
  if(!sipRes)
    sipRes = sipCpp->projectFolder()->graph(*a0, true);
%End
  MultiLayer* newGraph();
  MultiLayer* multilayerPlot(const QString&, int, int, int)/PyName=newGraph/;

  Note *note(const QString&);
%MethodCode
  sipRes = sipCpp->current_folder->note(*a0, false);
  if(!sipRes)
    sipRes = sipCpp->projectFolder()->note(*a0, true);
%End
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

	Matrix* importImage(const QString&);
	MultiLayer* plotSpectrogram(Matrix*, Graph::CurveType = Graph::ColorMap) /PyName=plot/;
	MultiLayer* plot(const QString&, double, double, int = 100);
%MethodCode
  QStringList l;
  l << *a0;
  sipRes = sipCpp->newFunctionPlot(l, a1, a2, a3);
%End

    Graph3D* newPlot3D();
    Graph3D* plotXYZ(Table* table, const QString&, int = 0) /PyName=plot3D/;
	Graph3D* plot3DMatrix(Matrix *, int style = 5) /PyName=plot3D/;
    Graph3D* plotSurface(const QString&, double xl, double xr, double yl, double yr,
			double zl, double zr, int=40, int=40) /PyName=plot3D/;
    Graph3D* plotParametricSurface(const QString&, const QString&, const QString&,
            double, double, double, double, int=40, int=40, bool=true, bool=true) /PyName=plot3D/;

  MDIWindowList windowsList() /PyName=windows/;

  // folders
  Folder *activeFolder() /NoDerived/;
%MethodCode
  sipRes = sipCpp->current_folder;
%End

  Folder* appendProject(const QString& file_name, Folder* parentFolder = 0);
  void saveFolder(Folder *folder, const QString& fn, bool=false);
  Folder* projectFolder() /PyName=rootFolder/;

  Folder* addFolder(QString name, Folder* parent = 0);
  bool deleteFolder(Folder *);
  bool changeFolder(Folder *, bool force = false);
  bool copyFolder(Folder *src, Folder *dest);

  MdiSubWindow* openTemplate(const QString&);
  void saveAsTemplate(MdiSubWindow*, const QString&);

  void setWindowName(MdiSubWindow *, const QString &);
  void setPreferences(Graph*);

  void saveProjectAs(const QString& fileName = QString(), bool = false);
  MdiSubWindow* clone(MdiSubWindow*);

  Matrix* tableToMatrix(Table* t);
  Table* matrixToTable(Matrix* m, MatrixToTableConversion = Direct);

  //---- Mantid
  MantidUI* mantidUI;
  void addUserMenu(const QString &);
  void addUserMenuAction(const QString &, const QString &, const QString &);
  //--------------
private:
  ApplicationWindow(const ApplicationWindow&);
};

class Fit : Filter
{
%TypeHeaderCode
#include "src/Fit.h"
%End
public:
  enum Algorithm{ScaledLevenbergMarquardt, UnscaledLevenbergMarquardt, NelderMeadSimplex};
  enum WeightingMethod{NoWeighting, Instrumental, Statistical, Dataset};

  Fit(ApplicationWindow* /TransferThis/, Graph*=0, const char*=0);
  ~Fit();

  virtual void fit();
  virtual bool run();

  bool setWeightingData(WeightingMethod, const QString&=QString::null);

  bool setDataFromCurve(const QString&, Graph*=0);
  bool setDataFromCurve(const QString&, double, double, Graph*=0);
  bool setDataFromTable(Table *, const QString&, const QString&, int = 1, int = -1);

  void setInterval(double from, double to);

  QString formula();
  QString resultFormula();
  int numParameters();

  void setInitialGuess(int, double) /PyName=setInitialValue/;
  void setInitialGuesses(...) /PyName=setInitialValues/;
%MethodCode
int n = PyTuple_GET_SIZE(a0);
double *values = new double[n];
for (int i=0; i<n; i++) {
  PyObject *item = PyTuple_GET_ITEM(a0, i);
  if (PyNumber_Check(item)) {
    item=PyNumber_Float(item);
    if (!item) {
      sipIsErr=1;
      break;
    }
    values[i] = PyFloat_AS_DOUBLE(item);
    Py_DECREF(item);
  } else
    values[i] = 0;
}
sipCpp->setInitialGuesses(values);
delete values;
%End

  virtual void guessInitialValues();
  void setParameterRange(int, double, double);

  void setAlgorithm(Algorithm);
  void setOutputPrecision(int);
  void generateFunction(bool, int=100);

  void showLegend();
  virtual QString legendInfo();

  void scaleErrors(bool yes = true);

  SIP_PYTUPLE results();
%MethodCode
double *results = sipCpp->results();
int size=sipCpp->numParameters();
sipRes = PyTuple_New(size);
if(sipRes)
{
  for(int i=0; i<size; i++)
    PyTuple_SET_ITEM(sipRes, i, PyFloat_FromDouble(results[i]));
} else
  sipIsErr = 1;
%End

  SIP_PYTUPLE errors();
%MethodCode
double *errors = sipCpp->errors();
int size=sipCpp->numParameters();
sipRes = PyTuple_New(size);
if(sipRes)
{
  for(int i=0; i<size; i++)
    PyTuple_SET_ITEM(sipRes, i, PyFloat_FromDouble(errors[i]));
} else
  sipIsErr = 1;
%End

  //! Returns the sum of squares of the residuals from the best-fit line
  double chiSquare();

  Table* parametersTable(const QString&);
  Matrix* covarianceMatrix(const QString&);
};

%ModuleCode
ApplicationWindow *sipqti_app()
{
  int iserr = 0;
  PyObject *me = PyImport_ImportModule("qti");
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

class ExponentialFit : Fit
{
%TypeHeaderCode
#include "src/ExponentialFit.h"
%End
public:
  ExponentialFit(ApplicationWindow * /TransferThis/, Table *, const QString&, const QString&, int, int, bool);
  ExponentialFit(Table *, const QString&, const QString&, int = 1, int = -1, bool = false) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipExponentialFit(app, a0, *a1, *a2, a3, a4, a5))
%End

  ExponentialFit(ApplicationWindow * /TransferThis/, Graph *, const QString&, bool=false);
  ExponentialFit(ApplicationWindow * /TransferThis/, Graph *, const QString&, double, double, bool=false);
  ExponentialFit(Graph *, const QString&, bool=false) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipExponentialFit(app, a0, *a1, a2))
%End
  ExponentialFit(Graph *, const QString&, double, double, bool=false) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipExponentialFit(app, a0, *a1, a2, a3, a4))
%End
};

class TwoExpFit : Fit
{
%TypeHeaderCode
#include "src/ExponentialFit.h"
%End
public:
  TwoExpFit(ApplicationWindow * /TransferThis/, Table *, const QString&, const QString&, int, int);
  TwoExpFit(Table *, const QString&, const QString&, int = 1, int = -1) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipTwoExpFit(app, a0, *a1, *a2, a3, a4))
%End

  TwoExpFit(ApplicationWindow * /TransferThis/, Graph *, const QString&);
  TwoExpFit(ApplicationWindow * /TransferThis/, Graph *, const QString&, double, double);
  TwoExpFit(Graph *, const QString&) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipTwoExpFit(app, a0, *a1))
%End
  TwoExpFit(Graph *, const QString&, double, double) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipTwoExpFit(app, a0, *a1, a2, a3))
%End
};

class ThreeExpFit : Fit
{
%TypeHeaderCode
#include "src/ExponentialFit.h"
%End
public:
  ThreeExpFit(ApplicationWindow * /TransferThis/, Table *, const QString&, const QString&, int, int);
  ThreeExpFit(Table *, const QString&, const QString&, int = 1, int = -1) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipThreeExpFit(app, a0, *a1, *a2, a3, a4))
%End

  ThreeExpFit(ApplicationWindow * /TransferThis/, Graph *, const QString&);
  ThreeExpFit(ApplicationWindow * /TransferThis/, Graph *, const QString&, double, double);
  ThreeExpFit(Graph *, const QString&) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipThreeExpFit(app, a0, *a1))
%End
  ThreeExpFit(Graph *, const QString&, double, double) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipThreeExpFit(app, a0, *a1, a2, a3))
%End
};

class SigmoidalFit : Fit
{
%TypeHeaderCode
#include "src/SigmoidalFit.h"
%End
public:
  SigmoidalFit(ApplicationWindow * /TransferThis/, Table *, const QString&, const QString&, int, int);
  SigmoidalFit(Table *, const QString&, const QString&, int = 1, int = -1) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipSigmoidalFit(app, a0, *a1, *a2, a3, a4))
%End

  SigmoidalFit(ApplicationWindow * /TransferThis/, Graph *, const QString&);
  SigmoidalFit(ApplicationWindow * /TransferThis/, Graph *, const QString&, double, double);
  SigmoidalFit(Graph *, const QString&) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipSigmoidalFit(app, a0, *a1))
%End
  SigmoidalFit(Graph *, const QString&, double, double) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipSigmoidalFit(app, a0, *a1, a2, a3))
%End

  void guessInitialValues();
};

class LogisticFit : Fit
{
%TypeHeaderCode
#include "src/LogisticFit.h"
%End
public:
  LogisticFit(ApplicationWindow * /TransferThis/, Table *, const QString&, const QString&, int, int);
  LogisticFit(Table *, const QString&, const QString&, int = 1, int = -1) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipLogisticFit(app, a0, *a1, *a2, a3, a4))
%End

  LogisticFit(ApplicationWindow * /TransferThis/, Graph *, const QString&);
  LogisticFit(ApplicationWindow * /TransferThis/, Graph *, const QString&, double, double);
  LogisticFit(Graph *, const QString&) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipLogisticFit(app, a0, *a1))
%End
  LogisticFit(Graph *, const QString&, double, double) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipLogisticFit(app, a0, *a1, a2, a3))
%End

  void guessInitialValues();
};

class GaussAmpFit : Fit
{
%TypeHeaderCode
#include "src/MultiPeakFit.h"
%End
public:
  GaussAmpFit(ApplicationWindow * /TransferThis/, Table *, const QString&, const QString&, int, int);
  GaussAmpFit(Table *, const QString&, const QString&, int = 1, int = -1) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipGaussAmpFit(app, a0, *a1, *a2, a3, a4))
%End

  GaussAmpFit(ApplicationWindow * /TransferThis/, Graph *, const QString&);
  GaussAmpFit(ApplicationWindow * /TransferThis/, Graph *, const QString&, double, double);
  GaussAmpFit(Graph *, const QString&) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipGaussAmpFit(app, a0, *a1))
%End
  GaussAmpFit(Graph *, const QString&, double, double) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipGaussAmpFit(app, a0, *a1, a2, a3))
%End

  void guessInitialValues();
};

class NonLinearFit : Fit
{
%TypeHeaderCode
#include "src/NonLinearFit.h"
%End
public:
  NonLinearFit(ApplicationWindow * /TransferThis/, Table *, const QString&, const QString&, int, int);
  NonLinearFit(Table *, const QString&, const QString&, int = 1, int = -1) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipNonLinearFit(app, a0, *a1, *a2, a3, a4))
%End

  NonLinearFit(ApplicationWindow * /TransferThis/, Graph *, const QString&);
  NonLinearFit(ApplicationWindow * /TransferThis/, Graph *, const QString&, double, double);
  NonLinearFit(Graph *, const QString&) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipNonLinearFit(app, a0, *a1))
%End
  NonLinearFit(Graph *, const QString&, double, double) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipNonLinearFit(app, a0, *a1, a2, a3))
%End
  void setParametersList(...) /PyName=setParameters/;
%MethodCode
  QStringList l;
  char *item;
  for (int i=0; i<PyTuple_GET_SIZE(a0); i++)
    if (item = PyString_AsString(PyTuple_GET_ITEM(a0, i)))
      l << item;
    else
      sipIsErr = 1;
  sipCpp->setParametersList(l);
%End
  // TODO: make it accept Python callables
  void setFormula(const QString&);

  bool save(const QString&);
  bool load(const QString&);
};

class PluginFit : Fit
{
%TypeHeaderCode
#include "src/PluginFit.h"
%End
public:
  PluginFit(ApplicationWindow * /TransferThis/, Table *, const QString&, const QString&, int, int);
  PluginFit(Table *, const QString&, const QString&, int = 1, int = -1) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipPluginFit(app, a0, *a1, *a2, a3, a4))
%End

  PluginFit(ApplicationWindow * /TransferThis/, Graph *, const QString&);
  PluginFit(ApplicationWindow * /TransferThis/, Graph *, const QString&, double, double);
  PluginFit(Graph *, const QString&) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipPluginFit(app, a0, *a1))
%End
  PluginFit(Graph *, const QString&, double, double) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipPluginFit(app, a0, *a1, a2, a3))
%End
  bool load(const QString&);
};

class MultiPeakFit : Fit
{
%TypeHeaderCode
#include "src/MultiPeakFit.h"
%End
public:
  enum PeakProfile{Gauss, Lorentz};
  MultiPeakFit(ApplicationWindow * /TransferThis/, Graph *, PeakProfile=Gauss, int=1);

  int peaks() /PyName=numPeaks/;
  void setNumPeaks(int);

  void enablePeakCurves(bool);
  void setPeakCurvesColor(int);

  static QString generateFormula(int, PeakProfile);
  static QStringList generateParameterList(int);

  void guessInitialValues();
};

class LorentzFit : MultiPeakFit
{
%TypeHeaderCode
#include "src/MultiPeakFit.h"
%End
public:
  LorentzFit(ApplicationWindow * /TransferThis/, Table *, const QString&, const QString&, int, int);
  LorentzFit(Table *, const QString&, const QString&, int = 1, int = -1) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipLorentzFit(app, a0, *a1, *a2, a3, a4))
%End

  LorentzFit(ApplicationWindow * /TransferThis/, Graph *, const QString&);
  LorentzFit(ApplicationWindow * /TransferThis/, Graph *, const QString&, double, double);
  LorentzFit(Graph *, const QString&) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipLorentzFit(app, a0, *a1))
%End
  LorentzFit(Graph *, const QString&, int, int) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipLorentzFit(app, a0, *a1, a2, a3))
%End
};

class GaussFit : MultiPeakFit
{
%TypeHeaderCode
#include "src/MultiPeakFit.h"
%End
public:
  GaussFit(ApplicationWindow * /TransferThis/, Table *, const QString&, const QString&, int, int);
  GaussFit(Table *, const QString&, const QString&, int = 1, int = -1) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipGaussFit(app, a0, *a1, *a2, a3, a4))
%End

  GaussFit(ApplicationWindow * /TransferThis/, Graph *, const QString&);
  GaussFit(ApplicationWindow * /TransferThis/, Graph *, const QString&, double, double);
  GaussFit(Graph *, const QString&) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipGaussFit(app, a0, *a1))
%End
  GaussFit(Graph *, const QString&, double, double) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipGaussFit(app, a0, *a1, a2, a3))
%End
};

class PolynomialFit : Fit
{
%TypeHeaderCode
#include "src/PolynomialFit.h"
%End
public:
  PolynomialFit(ApplicationWindow * /TransferThis/, Table *, const QString&, const QString&, int, int, int, bool);
  PolynomialFit(Table *, const QString&, const QString&, int = 1, int = -1, int = 2, bool = false) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipPolynomialFit(app, a0, *a1, *a2, a3, a4, a5, a6))
%End

  PolynomialFit(ApplicationWindow * /TransferThis/, Graph *, QString&, int=2, bool=false);
  PolynomialFit(ApplicationWindow * /TransferThis/, Graph *, QString&, int, int, int=2, bool=false);
  PolynomialFit(Graph *, QString&, int=2, bool=false) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipPolynomialFit(app, a0, *a1, a2, a3))
%End
  PolynomialFit(Graph *, QString&, int, int, int=2, bool=false) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipPolynomialFit(app, a0, *a1, a2, a3, a4, a5))
%End

  virtual QString legendInfo();
  void fit();

  static QString generateFormula(int);
  static QStringList generateParameterList(int);
};

class LinearFit : Fit
{
%TypeHeaderCode
#include "src/PolynomialFit.h"
%End
public:
  LinearFit(ApplicationWindow * /TransferThis/, Table *, const QString&, const QString&, int, int);
  LinearFit(Table *, const QString&, const QString&, int = 1, int = -1) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipLinearFit(app, a0, *a1, *a2, a3, a4))
%End

  LinearFit(ApplicationWindow * /TransferThis/, Graph *, const QString&);
  LinearFit(ApplicationWindow * /TransferThis/, Graph *, const QString&, double, double);
  LinearFit(Graph *, const QString&) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipLinearFit(app, a0, *a1))
%End
  LinearFit(Graph *, const QString&, double, double) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipLinearFit(app, a0, *a1, a2, a3))
%End
  void fit();
};

class Filter : QObject
{
%TypeHeaderCode
#include "src/Filter.h"
%End
public:
  Filter(ApplicationWindow* /TransferThis/, Graph*=0, const char*=0);
  ~Filter();

  void setOutputPoints(int);
  void setTolerance(double);
  void setMaximumIterations(int);

  void setColor(int);
  void setColor(const QString&);

  bool setDataFromCurve(const QString& curveTitle, double from, double to, Graph *g = 0);
  virtual bool setDataFromTable(Table *t, const QString& xColName, const QString& yColName, int from = 1, int to = -1);

  Table *resultTable();

  virtual void enableGraphicsDisplay(bool on = true, Graph *g = 0);
  virtual bool run();
};

class Differentiation : Filter
{
%TypeHeaderCode
#include "src/Differentiation.h"
%End
public:
  Differentiation(ApplicationWindow * /TransferThis/, Table *, const QString&, const QString&, int, int);
  Differentiation(Table *, const QString&, const QString&, int = 1, int = -1) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipDifferentiation(app, a0, *a1, *a2, a3, a4))
%End

  Differentiation(ApplicationWindow * /TransferThis/, Graph *, const QString&);
  Differentiation(ApplicationWindow * /TransferThis/, Graph *, const QString&, double, double);
  Differentiation(Graph *, const QString&) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipDifferentiation(app, a0, *a1))
%End
  Differentiation(Graph *, const QString&, double, double) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipDifferentiation(app, a0, *a1, a2, a3))
%End
  bool run();
};

class Integration : Filter
{
%TypeHeaderCode
#include "src/Integration.h"
%End
public:
  Integration(ApplicationWindow * /TransferThis/, Table *, const QString&, const QString&, int, int);
  Integration(Table *, const QString&, const QString&, int = 1, int = -1) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipIntegration(app, a0, *a1, *a2, a3, a4))
%End

  Integration(ApplicationWindow * /TransferThis/, Graph *, const QString&);
  Integration(ApplicationWindow * /TransferThis/, Graph *, const QString&, double, double);
  Integration(Graph *, const QString&) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipIntegration(app, a0, *a1))
%End
  Integration(Graph *, const QString&, double, double) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipIntegration(app, a0, *a1, a2, a3))
%End

  void enableGraphicsDisplay(bool on = true, Graph *g = 0);
  void setMethodOrder(int n);
  bool run();

  double area();
};

class Interpolation : Filter
{
%TypeHeaderCode
#include "src/Interpolation.h"
%End
public:
  enum InterpolationMethod{Linear, Cubic, Akima};

  Interpolation(ApplicationWindow * /TransferThis/, Table *, const QString&, const QString&, int, int, int);
  Interpolation(Table *, const QString&, const QString&, int = 1, int = -1, int = 0) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipInterpolation(app, a0, *a1, *a2, a3, a4, a5))
%End

  Interpolation(ApplicationWindow * /TransferThis/, Graph *, const QString&, int=0);
  Interpolation(ApplicationWindow * /TransferThis/, Graph *, const QString&, double, double, int=0);
  Interpolation(Graph *, const QString&, int=0) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipInterpolation(app, a0, *a1, a2))
%End
  Interpolation(Graph *, const QString&, double, double, int=0) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipInterpolation(app, a0, *a1, a2, a3, a4))
%End

  void setMethod(int n);
  bool run();
};

class SmoothFilter : Filter
{
%TypeHeaderCode
#include "src/SmoothFilter.h"
%End
public:
  enum SmoothMethod{SavitzkyGolay = 1, FFT = 2, Average = 3};

  SmoothFilter(ApplicationWindow * /TransferThis/, Table *, const QString&, const QString&, int, int, int);
  SmoothFilter(Table *, const QString&, const QString&, int = 1, int = -1, int = 3) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipSmoothFilter(app, a0, *a1, *a2, a3, a4, a5))
%End

  SmoothFilter(ApplicationWindow * /TransferThis/, Graph *, const QString&, int=3);
  SmoothFilter(ApplicationWindow * /TransferThis/, Graph *, const QString&, double, double, int=3);
  SmoothFilter(Graph *, const QString&, int=3) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipSmoothFilter(app, a0, *a1, a2))
%End
  SmoothFilter(Graph *, const QString&, double, double, int=3) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipSmoothFilter(app, a0, *a1, a2, a3, a4))
%End

  void setMethod(int);
  void setSmoothPoints(int, int = 0);
  void setPolynomOrder(int);
  bool run();
};

class FFTFilter : Filter
{
%TypeHeaderCode
#include "src/FFTFilter.h"
%End
public:
  enum FilterType{LowPass = 1, HighPass = 2, BandPass = 3, BandBlock = 4};

  FFTFilter(ApplicationWindow * /TransferThis/, Table *, const QString&, const QString&, int, int, int);
  FFTFilter(Table *, const QString&, const QString&, int = 1, int = -1, int = 1) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipFFTFilter(app, a0, *a1, *a2, a3, a4, a5))
%End

  FFTFilter(ApplicationWindow * /TransferThis/, Graph *, const QString&, int=1);
  FFTFilter(ApplicationWindow * /TransferThis/, Graph *, const QString&, double, double, int=1);
  FFTFilter(Graph *, const QString&, int=1) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipFFTFilter(app, a0, *a1, a2))
%End
  FFTFilter(Graph *, const QString&, double, double, int=1) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipFFTFilter(app, a0, *a1, a2, a3, a4))
%End

  void setFilterType(int);
  void setCutoff(double);
  void setBand(double, double);
  void enableOffset(bool=true);

  bool run();
};

class FFT : Filter
{
%TypeHeaderCode
#include "src/FFT.h"
%End
public:

  FFT(ApplicationWindow * /TransferThis/, Table *, const QString&, const QString& = QString(), int = 1, int = -1);
  FFT(ApplicationWindow * /TransferThis/, Graph *, const QString&, double, double);
  FFT(ApplicationWindow * /TransferThis/, Graph *, const QString&);

  FFT(Table *, const QString&, const QString& = QString(), int = 1, int = -1) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipFFT(app, a0, *a1, *a2, a3, a4))
%End
  FFT(Graph *, const QString&) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipFFT(app, a0, *a1))
%End
  FFT(Graph *, const QString&, double, double) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipFFT(app, a0, *a1, a2, a3))
%End

  void setInverseFFT(bool=true);
  void setSampling(double);
  void normalizeAmplitudes(bool=true);
  void shiftFrequencies(bool=true);

  bool run();
};

class Correlation : Filter
{
%TypeHeaderCode
#include "src/Correlation.h"
%End
public:

  Correlation(ApplicationWindow * /TransferThis/, Table *, const QString&, const QString&, int, int);
  Correlation(Table *, const QString&, const QString&, int = 1, int = -1) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipCorrelation(app, a0, *a1, *a2, a3, a4))
%End

  bool setDataFromTable(Table *, const QString&, const QString&, int = 1, int = -1);
  bool run();
};

class Convolution : Filter
{
%TypeHeaderCode
#include "src/Convolution.h"
%End
public:

  Convolution(ApplicationWindow * /TransferThis/, Table *, const QString&, const QString&);
  Convolution(Table *, const QString&, const QString&) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipConvolution(app, a0, *a1, *a2))
%End

  void setDataFromTable(Table *, const QString&, const QString&);
  bool run();
};

class Deconvolution : Filter
{
%TypeHeaderCode
#include "src/Convolution.h"
%End
public:

  Deconvolution(ApplicationWindow * /TransferThis/, Table *, const QString&, const QString&);
  Deconvolution(Table *, const QString&, const QString&) /NoDerived/;
%MethodCode
  SIPQTI_APP(new sipDeconvolution(app, a0, *a1, *a2))
%End

  void setDataFromTable(Table *, const QString&, const QString&);
  bool run();
};

// used for output redirection
class PythonScripting
{
%TypeHeaderCode
#include "src/PythonScripting.h"
%End
public:
  void write(const QString&);
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
  int indexX(int,double) const;
  Graph3D* plotGraph3D(int = 3);
  MultiLayer* plotGraph2D(Graph::CurveType = Graph::ColorMap);
  void goToTab(const QString &);	 
  void goTo(int,int) /PyName = selectCell/;	     
  void goToRow(int) /PyName = selectRow/;
  void goToColumn(int) /PyName = selectColumn/;	     
private:
  MantidMatrix(const MantidMatrix&);
};

class InstrumentWindow: MdiSubWindow
{
%TypeHeaderCode
#include "src/Mantid/InstrumentWidget/InstrumentWindow.h"
%End
public:
  void show() /PyName = showWindow/;
  void changeColormap() /PyName = changeColorMap/;
  void changeColormap(const QString & file) /PyName = changeColorMap/;
  void setColorMapMinValue(double);
  void setColorMapMaxValue(double);
  void setColorMapRange(double, double);
  void setDataMappingIntegral(double, double, bool) /PyName = setBinRange/;
  void selectComponent(const QString &);
  void setScaleType(GraphOptions::ScaleType);
private:
InstrumentWindow(const InstrumentWindow &);
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
  MultiLayer* pyPlotSpectraList(const QList<QString>& wsnames, const QList<int>&, bool errs=false);
  MultiLayer* mergePlots(MultiLayer*, MultiLayer*);
  void setIsRunning(bool);
  MantidMatrix* getMantidMatrix(const QString &);
  MultiLayer* plotBin(const QString&,int,bool = false);  
  MantidMatrix* importMatrixWorkspace(const QString&,int = -1, int = -1, bool = false, bool = false);
  Table* importTableWorkspace(const QString&, bool = false, bool = false);	
  bool runAlgorithmAsync_PyCallback(const QString &);
  bool createPropertyInputDialog(const QString &, const QString &, const QString&, const QString&);
  QString getSelectedWorkspaceName();
  InstrumentWindow* getInstrumentView(const QString &);
  
private:
  MantidUI(const MantidUI &);
};
