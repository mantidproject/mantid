/***************************************************************************
    File                 : Graph3D.h
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2004-2007 by Ion Vasilief
    Email (use @ for *)  : ion_vasilief*yahoo.fr
    Description          : 3D graph widget

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
#ifndef GRAPH3D_H
#define GRAPH3D_H

#include <qwt3d_surfaceplot.h>
#include <qwt3d_function.h>
#include <qwt3d_parametricsurface.h>

#include <QTimer>
#include <QVector>
#include <QEvent>

#include "Table.h"
#include "Matrix.h"

using namespace Qwt3D;

class UserFunction;
class UserParametricSurface;
class UserHelperFunction;//Mantid

/**\brief 3D graph widget.
 *
 * This provides 3D plotting using Qwt3D.
 *
 * \section future Future Plans
 * If MultiLayer is extended to accept any QWidget, Graph3D wouldn't have to inherit from MdiSubWindow any more.
 * It could also make sense to unify the interface with other plot types; see documentation of Graph.
 * Big problem here: export to vector formats. Qwt3D's export filters write directly to a file, so they
 * can't be combined with output generated via QPrinter.
 */
class Graph3D: public MdiSubWindow
{
	Q_OBJECT

public:
	Graph3D (const QString& label, ApplicationWindow* parent, const char* name=0, Qt::WFlags f=0);
	~Graph3D();

	enum PlotType{Scatter = 0, Trajectory = 1, Bars = 2, Ribbon =  3};
	enum PointStyle{None = 0, Dots = 1, VerticalBars = 2, HairCross = 3, Cones = 4};

public slots:
	void copy(Graph3D* g);
	void initPlot();
	void initCoord();
	void addFunction(const QString& s, double xl, double xr, double yl,
						  double yr, double zl, double zr, int columns, int rows, 
                          UserHelperFunction* hfun = 0);//Manid
	void addParametricSurface(const QString& xFormula, const QString& yFormula,
						const QString& zFormula, double ul, double ur, double vl, double vr,
						int columns, int rows, bool uPeriodic, bool vPeriodic);
	void insertNewData(Table* table, const QString& colName);

	Matrix * matrix(){return d_matrix;};
	void addMatrixData(Matrix* m);//used to plot matrixes
	void addMatrixData(Matrix* m,double xl,double xr,double yl,double yr,double zl,double zr);
	void updateMatrixData(Matrix* m);

	void addData(Table* table,const QString& xColName,const QString& yColName);
	void addData(Table* table,const QString& xColName,const QString& yColName,
                double xl, double xr, double yl, double yr, double zl, double zr);
	void addData(Table* table, int xCol, int yCol, int zCol, int type = 0);
	void loadData(Table* table, int xCol, int yCol, int zCol,
                double xl=0.0, double xr=0.0, double yl=0.0, double yr=0.0, double zl=0.0, double zr=0.0);

	void clearData();
	bool hasData(){return sp->hasData();};

	void updateData(Table* table);
	void updateDataXY(Table* table, int xCol, int yCol);

	void changeDataColumn(Table* table, const QString& colName, int type = 0);

	//! \name User Functions
	//@{
	UserParametricSurface *parametricSurface(){return d_surface;};
	//@}

	//! \name User Functions
	//@{
	UserFunction* userFunction(){return d_func;};
	QString formula();
	//@}

	//! \name Event Handlers
	//@{
	bool eventFilter(QObject *object, QEvent *e);
	void resizeEvent (QResizeEvent *);
	void scaleFonts(double factor);
	void setIgnoreFonts(bool ok){ignoreFonts = ok;};
	//@}

	//! \name Axes
	//@{
	void setFramed();
	void setBoxed();
	void setNoAxes();
	bool isOrthogonal(){return sp->ortho();};
	void setOrthogonal(bool on = true){sp->setOrtho(on);};

	QStringList axesLabels(){return labels;};
	void setAxesLabels(const QStringList& lst);
	void resetAxesLabels();

    void setXAxisLabel(const QString&);
    void setYAxisLabel(const QString&);
	void setZAxisLabel(const QString&);

	QFont xAxisLabelFont();
	QFont yAxisLabelFont();
	QFont zAxisLabelFont();

	void setXAxisLabelFont(const QFont& fnt);
	void setYAxisLabelFont(const QFont& fnt);
	void setZAxisLabelFont(const QFont& fnt);

	void setXAxisLabelFont(const QStringList& lst);
	void setYAxisLabelFont(const QStringList& lst);
	void setZAxisLabelFont(const QStringList& lst);

	QFont numbersFont();
	void setNumbersFont(const QFont& font);
	void setNumbersFont(const QStringList& lst);

	double xStart();
	double xStop();
	double yStart();
	double yStop();
	double zStart();
	double zStop();
	QStringList scaleLimits();
	void updateScale(int axis, const QStringList& options);
	void setScales(double xl, double xr, double yl, double yr, double zl, double zr);
	void updateScales(double xl, double xr, double yl, double yr,
				  		double zl, double zr, int xcol, int ycol);
	void updateScalesFromMatrix(double xl,double xr,double yl,double yr,double zl,double zr);

	QStringList scaleTicks();
	void setTicks(const QStringList& options);

	void setXAxisTickLength(double majorLength, double minorLength);
    void setYAxisTickLength(double majorLength, double minorLength);
	void setZAxisTickLength(double majorLength, double minorLength);

	void setAxisTickLength(int axis, double majorLength, double minorLength);
	void setLabelsDistance(int val);
	int labelsDistance(){return labelsDist;};

	QStringList axisTickLengths();
	void setTickLengths(const QStringList& lst);
	//@}

	//! \name Mesh
	//@{
	void setPolygonStyle();
	void setHiddenLineStyle();
	void setWireframeStyle();
	void setFilledMeshStyle();
	void setDotStyle();
	void setBarStyle();
	void setFloorData();
	void setFloorIsolines();
	void setEmptyFloor();

	void setMeshLineWidth(double lw);
	double meshLineWidth(){return sp->meshLineWidth();};
	//@}

	//! \name Grid
	//@{
	int grids();
	void setGrid(Qwt3D::SIDE s, bool b);
	void setGrid(int grids);

	void setLeftGrid(bool b = true);
	void setRightGrid(bool b = true);
	void setCeilGrid(bool b = true);
	void setFloorGrid(bool b = true);
	void setFrontGrid(bool b = true);
	void setBackGrid(bool b = true);
	//@}

	void setStyle(const QStringList& st);
	void customPlotStyle(int style);
	void resetNonEmptyStyle();

	void setRotation(double  xVal,double  yVal,double  zVal);
	void setScale(double  xVal,double  yVal,double  zVal);
	void setShift(double  xVal,double  yVal,double  zVal);

	double xRotation(){return sp->xRotation();};
	double yRotation(){return sp->yRotation();};
	double zRotation(){return sp->zRotation();};

	double xScale(){return sp->xScale();};
	double yScale(){return sp->yScale();};
	double zScale(){return sp->zScale();};

	double xShift(){return sp->xShift();};
	double yShift(){return sp->yShift();};
	double zShift(){return sp->zShift();};

	double zoom(){return sp->zoom();};
	void setZoom(double val);

	Qwt3D::PLOTSTYLE plotStyle();
	Qwt3D::FLOORSTYLE floorStyle();
	Qwt3D::COORDSTYLE coordStyle();

	void print();
	void copyImage();
	void exportImage(const QString& fileName, int quality = 100, bool transparent = false);
    void exportPDF(const QString& fileName);
    void exportVector(const QString& fileName);
    void exportToFile(const QString& fileName);

	QString saveToString(const QString& geometry, bool = false);
	QString saveAsTemplate(const QString& geometryInfo);

	void zoomChanged(double);
	void rotationChanged(double, double, double);
	void scaleChanged(double, double, double);
	void shiftChanged(double, double, double);

	//! \name Colors
	//@{
	void setDataColors(const QColor& cMax, const QColor& cMin);

	void changeTransparency(double t);
	void setTransparency(double t);
	double transparency(){return alpha;};

	QColor minDataColor();
	QColor maxDataColor();
	QColor meshColor(){return meshCol;};
	QColor axesColor(){return axesCol;};
	QColor labelColor(){return labelsCol;};
	QColor numColor(){return numCol;};
	QColor bgColor(){return bgCol;};
	QColor gridColor(){return gridCol;};

	QString colorMap(){return color_map;};
	void setDataColorMap(const QString& fileName);
	bool openColorMap(ColorVector& cv, QString fname);

	void setMeshColor(const QColor&);
	void setAxesColor(const QColor&);
	void setNumbersColor(const QColor&);
	void setLabelsColor(const QColor&);
	void setBackgroundColor(const QColor&);
	void setGridColor(const QColor&);

	void setColors(const QStringList& colors);
	//@}

	//! \name Title
	//@{
	QFont titleFont(){return titleFnt;};
	void setTitleFont(const QFont& font);
	QString plotTitle(){return title;};
	QColor titleColor(){return titleCol;};
	void setTitle(const QStringList& lst);
	void setTitle(const QString& s, const QColor& color = QColor(Qt::black), const QFont& font = QFont());
	//@}

	//! \name Resolution
	//@{
	void setResolution(int r);
	int resolution(){return sp->resolution();};
	//@}

	//! \name Legend
	//@{
	void showColorLegend(bool show = true);
	bool isLegendOn(){return legendOn;};
	//@}

	void setOptions(bool legend, int r, int dist);
	void setOptions(const QStringList& lst);
	void update();

	//! \name Bars
	//@{
	double barsRadius();
	void setBarRadius(double rad);
	//@}

	//! \name Scatter Plots
	//@{
	double pointsSize(){return d_point_size;};
	bool smoothPoints(){return d_smooth_points;};
	void setDotOptions(double size, bool smooth);

	bool smoothCrossHair(){return crossHairSmooth;};
	bool boxedCrossHair(){return crossHairBoxed;};
	double crossHairRadius(){return crossHairRad;};
	double crossHairLinewidth(){return crossHairLineWidth;};
	void setCrossOptions(double rad, double linewidth, bool smooth, bool boxed);
	void setCrossStyle();

	double coneRadius(){return conesRad;};
	int coneQuality(){return conesQuality;};
	void setConeOptions(double rad, int quality);
	void setConeStyle();

	PointStyle pointType(){return pointStyle;};
	//@}

	Table* table(){return d_table;};
	void showWorksheet();
	void setPlotAssociation(const QString& s){plotAssociation = s;};

	void setAntialiasing(bool smooth = true);
	bool antialiasing(){return sp->smoothDataMesh();};

	//! Used for the animation: rotates the scene with 1/360 degrees
	void rotate();
	void animate(bool on = true);
	bool isAnimated(){return d_timer->isActive();};

    void findBestLayout();
	bool autoscale(){return d_autoscale;};
	//! Enables/Disables autoscaling using findBestLayout().
	void setAutoscale(bool on = true){d_autoscale = on;};

signals:
	void showOptionsDialog();
	void modified();

private:
	//! Wait this many msecs before redraw 3D plot (used for animations)
  	int animation_redraw_wait;
	//! File name of the color map used for the data (if any)
  	QString color_map;

	QTimer *d_timer;
	QString title, plotAssociation;
	QStringList labels;
	QFont titleFnt;
	bool legendOn, d_autoscale;
	QVector<int> scaleType;
	QColor axesCol,labelsCol,titleCol,meshCol,bgCol,numCol,gridCol;
	//! Custom data colors.
	QColor fromColor, toColor;
	int labelsDist, legendMajorTicks;
	bool ignoreFonts;
	Qwt3D::StandardColor* col_;
	double barsRad, alpha, d_point_size, crossHairRad, crossHairLineWidth, conesRad;
	//! Draw 3D points with smoothed angles.
	bool d_smooth_points;
	bool crossHairSmooth, crossHairBoxed;
	int conesQuality;
	PointStyle pointStyle;
	Table *d_table;
	Matrix *d_matrix;
    Qwt3D::SurfacePlot* sp;
	UserFunction *d_func;
	UserParametricSurface *d_surface;
	Qwt3D::PLOTSTYLE style_;
	
	//scaling factor for zoom in the 3d graph
	double m_zoomInScale;
	//scaling factor for zoom out the 3d graph
	double m_zoomOutScale;
	int m_PreviousYpos;
};

//! Class for user defined parametric surfaces
class UserParametricSurface : public ParametricSurface
{
public:
    UserParametricSurface(const QString& xFormula, const QString& yFormula,
						  const QString& zFormula, SurfacePlot& pw);
    Triple operator()(double u, double v);

	unsigned int rows(){return d_rows;};
	unsigned int columns(){return d_columns;};
	void setMesh (unsigned int columns, unsigned int rows);

	bool uPeriodic(){return d_u_periodic;};
	bool vPeriodic(){return d_v_periodic;};
	void setPeriodic (bool u, bool v);

	double uStart(){return d_ul;};
	double uEnd(){return d_ur;};
	double vStart(){return d_vl;};
	double vEnd(){return d_vr;};
	void setDomain(double ul, double ur, double vl, double vr);

	QString xFormula(){return d_x_formula;};
	QString yFormula(){return d_y_formula;};
	QString zFormula(){return d_z_formula;};

private:
	QString d_x_formula, d_y_formula, d_z_formula;
	unsigned int d_rows, d_columns;
	bool d_u_periodic, d_v_periodic;
	double d_ul, d_ur, d_vl, d_vr;
};

#endif // Plot3D_H
