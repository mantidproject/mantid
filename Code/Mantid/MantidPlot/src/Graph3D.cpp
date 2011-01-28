/***************************************************************************
    File                 : Graph3D.cpp
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
#include "Graph3D.h"
#include "Bar.h"
#include "Cone3D.h"
#include "MyParser.h"
#include "MatrixModel.h"
#include "UserFunction.h"//Mantid


#include <QApplication>
#include <QMessageBox>
#include <QFileDialog>
#include <QPrinter>
#include <QClipboard>
#include <QPixmap>
#include <QBitmap>
#include <QCursor>
#include <QImageWriter>

#include <qwt3d_io_gl2ps.h>
#include <qwt3d_coordsys.h>

#include <gsl/gsl_vector.h>
#include <fstream>
Mantid::Kernel::Logger& Graph3D::logObject=Mantid::Kernel::Logger::get("Graph3D");

UserParametricSurface::UserParametricSurface(const QString& xFormula, const QString& yFormula,
											 const QString& zFormula, SurfacePlot& pw)
: ParametricSurface(pw),
d_x_formula(xFormula),
d_y_formula(yFormula),
d_z_formula(zFormula)
{}

void UserParametricSurface::setDomain(double ul, double ur, double vl, double vr)
{
	ParametricSurface::setDomain(ul, ur, vl, vr);
	d_ul = ul;
	d_ur = ur;
	d_vl = vl;
	d_vr = vr;
}

void UserParametricSurface::setMesh (unsigned int columns, unsigned int rows)
{
	ParametricSurface::setMesh (columns, rows);
	d_columns = columns;
	d_rows = rows;
}

void UserParametricSurface::setPeriodic (bool u, bool v)
{
	ParametricSurface::setPeriodic (u, v);
	d_u_periodic = u;
	d_v_periodic = v;
}

Triple UserParametricSurface::operator()(double u, double v)
{
	if (d_x_formula.isEmpty() || d_y_formula.isEmpty() || d_z_formula.isEmpty())
		return Triple(0.0, 0.0, 0.0);

	double x = 0.0, y = 0.0, z = 0.0;
	MyParser parser;
	try{
		parser.DefineVar("u", &u);
		parser.DefineVar("v", &v);

		parser.SetExpr((const std::string)d_x_formula.ascii());
		x = parser.Eval();
		parser.SetExpr((const std::string)d_y_formula.ascii());
		y = parser.Eval();
		parser.SetExpr((const std::string)d_z_formula.ascii());
		z = parser.Eval();
	}
	catch(mu::ParserError &e){
		QMessageBox::critical(0, "MantidPlot - Input function error", QString::fromStdString(e.GetMsg()));
	}
	return Triple(x, y, z);
}

Graph3D::Graph3D(const QString& label, ApplicationWindow* parent, const char* name, Qt::WFlags f)
: MdiSubWindow(label, parent, name, f)
{
	initPlot();
}

void Graph3D::initPlot()
{
	d_table = 0;
	d_matrix = 0;
    plotAssociation = QString();

    color_map = QString::null;
    animation_redraw_wait = 50;
    d_timer = new QTimer(this);
    connect(d_timer, SIGNAL(timeout()), this, SLOT(rotate()));
	ignoreFonts = false;

    setGeometry(0, 0, 500, 400);
	sp = new SurfacePlot(this);
	sp->installEventFilter(this);
	sp->setRotation(30, 0, 15);
	sp->setScale(1, 1, 1);
	sp->setShift(0.15, 0, 0);
	sp->setZoom(0.9);
	sp->setOrtho(false);
	sp->setSmoothMesh(false);
	setWidget(sp);
	d_autoscale = true;

	title = QString();
	sp->setTitle(title);

	titleCol = QColor(Qt::black);
	sp->setTitleColor(Qt2GL(titleCol));

	titleFnt = QFont("Times New Roman", 14);
	titleFnt.setBold(true);

	sp->setTitleFont(titleFnt.family(), titleFnt.pointSize(),
			titleFnt.weight(), titleFnt.italic());

	axesCol=QColor(Qt::black);
	labelsCol=QColor(Qt::black);
	numCol=QColor(Qt::black);
	meshCol=QColor(Qt::black);
	gridCol=QColor(Qt::black);
	bgCol=QColor(255, 255, 255);
	fromColor=QColor(Qt::red);
	toColor=QColor(Qt::blue);

	col_ = 0;

	legendOn = false;
	legendMajorTicks = 5;
	sp->showColorLegend(legendOn);
	sp->legend()->setAutoScale(true);
	sp->legend()->setMajors(legendMajorTicks) ;

	labelsDist=0;

	scaleType=QVector<int>(3);
	for (int j=0;j<3;j++)
		scaleType[j]=0;

	pointStyle = None;
	d_func = 0;
	d_surface = 0;
	alpha = 1.0;
	barsRad = 0.007;
	d_point_size = 5; d_smooth_points = false;
	crossHairRad = 0.03, crossHairLineWidth = 2;
	crossHairSmooth = true, crossHairBoxed = false;
	conesQuality = 32; conesRad = 0.5;

	style_ = NOPLOT;
	initCoord();	
	
	connect(sp,SIGNAL(rotationChanged(double, double, double)),this,SLOT(rotationChanged(double, double, double)));
	connect(sp,SIGNAL(zoomChanged(double)),this,SLOT(zoomChanged(double)));
	connect(sp,SIGNAL(scaleChanged(double, double, double)),this,SLOT(scaleChanged(double, double, double)));
	connect(sp,SIGNAL(shiftChanged(double, double, double)),this,SLOT(shiftChanged(double, double, double)));

	m_zoomInScale=1;
	m_zoomOutScale=1;
	m_PreviousYpos=0;
	
	

}

void Graph3D::initCoord()
{
	sp->makeCurrent();
	for (unsigned i=0; i!=sp->coordinates()->axes.size(); ++i){
		sp->coordinates()->axes[i].setMajors(5);
		sp->coordinates()->axes[i].setMinors(5);
	}

	QString s = tr("X axis");
	sp->coordinates()->axes[X1].setLabelString(s);
	sp->coordinates()->axes[X2].setLabelString(s);
	sp->coordinates()->axes[X3].setLabelString(s);
	sp->coordinates()->axes[X4].setLabelString(s);
	labels<<s;

	s = tr("Y axis");
	sp->coordinates()->axes[Y1].setLabelString(s);
	sp->coordinates()->axes[Y2].setLabelString(s);
	sp->coordinates()->axes[Y3].setLabelString(s);
	sp->coordinates()->axes[Y4].setLabelString(s);
	labels<<s;

	s = tr("Z axis");
	sp->coordinates()->axes[Z1].setLabelString(s);
	sp->coordinates()->axes[Z2].setLabelString(s);
	sp->coordinates()->axes[Z3].setLabelString(s);
	sp->coordinates()->axes[Z4].setLabelString(s);
	labels<<s;

	sp->setCoordinateStyle(BOX);
	sp->coordinates()->setLineSmooth(false);
	sp->coordinates()->setAutoScale(false);
}

void Graph3D::addFunction(const QString& s, double xl, double xr, double yl,
						double yr, double zl, double zr, int columns, int rows, 
                        UserHelperFunction* hfun)//Mantid
{
	if (d_surface)
		delete d_surface;
	else if (d_func)
		delete d_func;

	sp->makeCurrent();
	sp->resize(this->size());

	d_func = new UserFunction(s, *sp);

    d_func->setHlpFun(hfun);//Mantid

	d_func->setMesh(columns, rows);
	d_func->setDomain(xl, xr, yl, yr);
	d_func->setMinZ(zl);
	d_func->setMaxZ(zr);
	d_func->create();

	sp->legend()->setLimits(zl, zr);

	if (sp->plotStyle() == NOPLOT){
		sp->setPlotStyle(FILLED);
		style_=FILLED;
		pointStyle = None;
	}
  	sp->createCoordinateSystem(Triple(xl, yl, zl), Triple(xr, yr, zr));
	findBestLayout();
	update();
}

void Graph3D::addParametricSurface(const QString& xFormula, const QString& yFormula,
						const QString& zFormula, double ul, double ur, double vl, double vr,
						int columns, int rows, bool uPeriodic, bool vPeriodic)
{
	if (d_surface)
		delete d_surface;
	else if (d_func)
		delete d_func;

	sp->makeCurrent();
	sp->resize(this->size());

	d_surface = new UserParametricSurface(xFormula, yFormula, zFormula, *sp);
	d_surface->setMesh(columns, rows);
	d_surface->setDomain(ul, ur, vl, vr);
	d_surface->setPeriodic(uPeriodic, vPeriodic);
	d_surface->create();

	double zl, zr;
	sp->coordinates()->axes[Z1].limits(zl, zr);
	sp->legend()->setLimits(zl, zr);

	if (sp->plotStyle() == NOPLOT){
		sp->setPlotStyle(FILLED);
		style_=FILLED;
		pointStyle = None;
	}
  	//sp->createCoordinateSystem(Triple(xl, yl, zl), Triple(xr, yr, zr));
	findBestLayout();
	update();
}

void Graph3D::addData(Table* table,const QString& xColName, const QString& yColName)
{
	if (!table)
		return;

	int xcol = table->colIndex(xColName);
	int ycol =  table->colIndex(yColName);

	if (xcol < 0 || ycol < 0)
		return;

	plotAssociation = xColName+"(X)," + yColName+"(Y)";
	d_table=table;
	int r=table->numRows();
	int i, xmesh=0, ymesh=2;
	for (i = 0; i < r; i++){
		if (!table->text(i,xcol).isEmpty() && !table->text(i,ycol).isEmpty())
			xmesh++;
	}

	if (xmesh == 0)
		xmesh++;

	double **data = Matrix::allocateMatrixData(xmesh, ymesh);
	gsl_vector * x = gsl_vector_alloc (xmesh);
	gsl_vector * y = gsl_vector_alloc (xmesh);

	for (int j = 0; j < ymesh; j++){
		int k=0;
		for (i = 0; i < r; i++){
			if (!table->text(i,xcol).isEmpty() && !table->text(i,ycol).isEmpty()){
				gsl_vector_set (x, k, table->cell(i, xcol));

				double yv = table->cell(i, ycol);
				gsl_vector_set (y, k, yv);
				data[k][j] = yv;
				k++;
			}
		}
	}

	double maxy=gsl_vector_max(y);
	double maxz=0.6*maxy;
	sp->makeCurrent();
	sp->legend()->setLimits(gsl_vector_min(y),maxy);
	sp->loadFromData(data, xmesh, ymesh, gsl_vector_min(x),gsl_vector_max(x),0,maxz);

	if (d_autoscale)
		findBestLayout();

	gsl_vector_free (x);
	gsl_vector_free (y);
	Matrix::freeMatrixData(data, xmesh);
}

void Graph3D::addData(Table* table,const QString& xColName,const QString& yColName,
		double xl, double xr, double yl, double yr, double zl, double zr)
{
	d_table=table;
	int r=table->numRows();
	int xcol=table->colIndex(xColName);
	int ycol=table->colIndex(yColName);

	plotAssociation = xColName + "(X)," + yColName + "(Y)";

	int i, j, xmesh=0, ymesh=2;
	double xv, yv;

	for (i = 0; i < r; i++){
		if (!table->text(i,xcol).isEmpty() && !table->text(i,ycol).isEmpty()){
			xv=table->cell(i, xcol);
			if (xv>=xl && xv <= xr)
				xmesh++;
		}
	}

	if (xmesh == 0)
		xmesh++;

	double **data = Matrix::allocateMatrixData(xmesh, ymesh);
	for ( j = 0; j < ymesh; j++){
		int k=0;
		for ( i = 0; i < r; i++){
			if (!table->text(i,xcol).isEmpty() && !table->text(i,ycol).isEmpty()){
				xv=table->cell(i,xcol);
				if (xv>=xl && xv <= xr){
					yv=table->cell(i,ycol);
					if (yv > zr)
						data[k][j] = zr;
					else if (yv < zl)
						data[k][j] = zl;
					else
						data[k][j] = yv;
					k++;
				}
			}
		}
	}
	sp->makeCurrent();
	sp->loadFromData(data, xmesh, ymesh, xl, xr, yl, yr);
	sp->createCoordinateSystem(Triple(xl, yl, zl), Triple(xr, yr, zr));
	sp->legend()->setLimits(zl, zr);
	sp->legend()->setMajors(legendMajorTicks);

	Matrix::freeMatrixData(data, xmesh);
}

void Graph3D::addMatrixData(Matrix* m)
{
	if (!m || d_matrix == m)
		return;

	bool first_time = false;
	if(!d_matrix)
		first_time = true;

	d_matrix = m;
	plotAssociation = "matrix<" + QString(m->objectName()) + ">";

	int cols = m->numCols();
	int rows = m->numRows();

	double **data_matrix = Matrix::allocateMatrixData(cols, rows);
	for (int i = 0; i < cols; i++ ){
		for (int j = 0; j < rows; j++)
			data_matrix[i][j] = m->cell(j, i);
	}

	sp->makeCurrent();
	sp->loadFromData(data_matrix, cols, rows, m->xStart(), m->xEnd(), m->yStart(), m->yEnd());

	double start, end;
	sp->coordinates()->axes[Z1].limits (start, end);
	sp->legend()->setLimits(start, end);
	sp->legend()->setMajors(legendMajorTicks);

	Matrix::freeMatrixData(data_matrix, cols);

	if (d_autoscale || first_time)
		findBestLayout();
	update();
}

void Graph3D::addMatrixData(Matrix* m, double xl, double xr,
		double yl, double yr, double zl, double zr)
{
	d_matrix = m;
	plotAssociation = "matrix<" + QString(m->objectName()) + ">";

	updateScalesFromMatrix(xl, xr, yl, yr, zl, zr);
}

void Graph3D::insertNewData(Table* table, const QString& colName)
{
	int zCol=table->colIndex(colName);
	int yCol=table->colY(zCol);
	int xCol=table->colX(zCol);

	addData(table, xCol, yCol, zCol, Trajectory);
	update();
}

void Graph3D::changeDataColumn(Table* table, const QString& colName, int type)
{
	if (!table)
		return;

	resetNonEmptyStyle();

	if (type == Ribbon) {
		int ycol = table->colIndex(colName);
		int xcol = table->colX(ycol);

		addData(table, table->colName(xcol), colName);
		setFilledMeshStyle();
	} else {
		int zCol=table->colIndex(colName);
		int yCol=table->colY(zCol);
		int xCol=table->colX(zCol);

		addData(table, xCol, yCol, zCol, type);
	}

    resetAxesLabels();
}

void Graph3D::addData(Table* table, int xCol, int yCol, int zCol, int type)
{
	loadData(table, xCol, yCol, zCol);

	if (d_autoscale)
		findBestLayout();

	if (type == Scatter)
		setDotStyle();
	else if (type == Trajectory)
		setWireframeStyle();
	else
		setBarStyle();
}

void Graph3D::loadData(Table* table, int xCol, int yCol, int zCol,
		double xl, double xr, double yl, double yr, double zl, double zr)
{
	if (!table || xCol < 0 || yCol < 0 || zCol < 0)
		return;

	d_table = table;

	plotAssociation = table->colName(xCol) + "(X),";
	plotAssociation += table->colName(yCol) + "(Y),";
	plotAssociation += table->colName(zCol) + "(Z)";

	bool check_limits = true;
	if (xl == xr && yl == yr && zl == zr)
		check_limits = false;

	Qwt3D::TripleField data;
	Qwt3D::CellField cells;
	int index = 0;
	for (int i = 0; i < table->numRows(); i++){
		if (!table->text(i, xCol).isEmpty() && !table->text(i, yCol).isEmpty() && !table->text(i, zCol).isEmpty()){
			double x = table->cell(i, xCol);
			double y = table->cell(i, yCol);
			double z = table->cell(i, zCol);

			if (check_limits && (x < xl || x > xr || y < yl || y > yr || z < zl || z > zr))
				continue;

			data.push_back (Triple(x, y, z));
			Qwt3D::Cell cell;
			cell.push_back(index);
			if (index > 0)
                cell.push_back(index-1);
			cells.push_back (cell);
			index ++;
		}
	}

	sp->loadFromData (data, cells);
	if (check_limits)
		sp->createCoordinateSystem(Triple(xl, yl, zl), Triple(xr, yr, zr));

	double start, end;
	sp->coordinates()->axes[Z1].limits (start, end);
	sp->legend()->setLimits(start, end);
	sp->legend()->setMajors(legendMajorTicks);
}

void Graph3D::updateData(Table* table)
{
	if (d_func)// function plot
		return;

	QString name = plotAssociation;
	int pos=name.find("_",0);
	int posX=name.find("(",pos);
	QString xColName=name.mid(pos+1,posX-pos-1);

	pos=name.find(",", posX);
	posX=name.find("(", pos);
	QString yColName=name.mid(pos+1, posX-pos-1);

	int xCol=table->colIndex(xColName);
	int yCol=table->colIndex(yColName);

	if (name.contains("(Z)", true)) {
		pos=name.find(",", posX);
		posX=name.find("(", pos);
		QString zColName=name.mid(pos+1, posX-pos-1);
		int zCol=table->colIndex(zColName);
		resetNonEmptyStyle();
		loadData(table, xCol, yCol, zCol);
	} else
		updateDataXY(table, xCol, yCol);

	if (d_autoscale)
		findBestLayout();
	update();
}

void Graph3D::updateDataXY(Table* table, int xCol, int yCol)
{
	int r=table->numRows();
	int i, j, xmesh=0, ymesh=2;

	for (i = 0; i < r; i++)
	{
		if (!table->text(i,xCol).isEmpty() && !table->text(i,yCol).isEmpty())
			xmesh++;
	}

	if (xmesh<2)
	{
		sp->setPlotStyle(NOPLOT);
		update();
		return;
	}

	double **data = Matrix::allocateMatrixData(xmesh, ymesh);
	gsl_vector * x = gsl_vector_alloc (xmesh);
	gsl_vector * y = gsl_vector_alloc (xmesh);

	for ( j = 0; j < ymesh; j++)
	{
		int k=0;
		for ( i = 0; i < r; i++)
		{
			if (!table->text(i,xCol).isEmpty() && !table->text(i,yCol).isEmpty())
			{
				double xv=table->cell(i,xCol);
				double yv=table->cell(i,yCol);

				gsl_vector_set (x, k, xv);
				gsl_vector_set (y, k, yv);

				data[k][j] =yv;
				k++;
			}
		}
	}

	double minx=gsl_vector_min (x);
	double maxx=gsl_vector_max(x);
	double minz=gsl_vector_min (y);
	double maxz=gsl_vector_max(y);
	double miny, maxy;

	sp->makeCurrent();
	resetNonEmptyStyle();
	sp->coordinates()->axes[Y1].limits (miny,maxy);	//actual Y scale limits
	sp->loadFromData(data, xmesh, ymesh, minx, maxx, miny, maxy);
	sp->legend()->setLimits(minz,maxz);
	sp->legend()->setMajors(legendMajorTicks);

	gsl_vector_free (x);gsl_vector_free (y);
	Matrix::freeMatrixData(data, xmesh);
}

void Graph3D::updateMatrixData(Matrix* m)
{
	int cols=m->numCols();
	int rows=m->numRows();

	double **data = Matrix::allocateMatrixData(cols, rows);
	for (int i = 0; i < cols; i++ ){
		for (int j = 0; j < rows; j++)
			data[i][j] = m->cell(j, i);
	}
	sp->loadFromData(data, cols, rows, m->xStart(), m->xEnd(), m->yStart(), m->yEnd());

	Qwt3D::Axis z_axis = sp->coordinates()->axes[Z1];
	double start, end;
	z_axis.limits (start, end);
	z_axis.setMajors(z_axis.majors());
	z_axis.setMajors(z_axis.minors());

	sp->legend()->setLimits(start, end);
	sp->legend()->setMajors(legendMajorTicks);

	Matrix::freeMatrixData(data, cols);
	if (d_autoscale)
		findBestLayout();
	update();
}

void Graph3D::resetNonEmptyStyle()
{
	if (sp->plotStyle() != Qwt3D::NOPLOT )
		return; // the plot was not previousely emptied

	if (style_== Qwt3D::USER)
	{// reseting the right user plot style
		switch (pointStyle)
		{
			case None:
				break;

			case Dots :
				sp->setPlotStyle(Dot(d_point_size, d_smooth_points));
				break;

			case VerticalBars :
				sp->setPlotStyle(Bar(barsRad));
				break;

			case HairCross :
				sp->setPlotStyle(CrossHair(crossHairRad, crossHairLineWidth, crossHairSmooth, crossHairBoxed));
				break;

			case Cones :
				sp->setPlotStyle(Cone3D(conesRad, conesQuality));
				break;
		}
	}
	else
		sp->setPlotStyle(style_);
}

void Graph3D::update()
{
	sp->makeCurrent();

	resetAxesLabels();

	sp->updateData();
	sp->updateGL();
}

void Graph3D::setLabelsDistance(int val)
{
	if (labelsDist != val){
		labelsDist = val;
		sp->coordinates()->adjustLabels(val);
		sp->makeCurrent();
		sp->updateGL();
        emit modified();
	}
}

QFont Graph3D::numbersFont()
{
	return sp->coordinates()->axes[X1].numberFont();
}

void Graph3D::setNumbersFont(const QFont& font)
{
	sp->coordinates()->setNumberFont (font);
	sp->makeCurrent();
	sp->updateGL();
}

void Graph3D::setNumbersFont(const QStringList& lst)
{
	QFont fnt=QFont(lst[1],lst[2].toInt(),lst[3].toInt(),lst[4].toInt());
	sp->coordinates()->setNumberFont(fnt);
}

void Graph3D::setXAxisLabelFont(const QFont& fnt)
{
	sp->coordinates()->axes[X1].setLabelFont(fnt);
	sp->coordinates()->axes[X2].setLabelFont(fnt);
	sp->coordinates()->axes[X3].setLabelFont(fnt);
	sp->coordinates()->axes[X4].setLabelFont(fnt);
}

void Graph3D::setYAxisLabelFont(const QFont& fnt)
{
	sp->coordinates()->axes[Y1].setLabelFont(fnt);
	sp->coordinates()->axes[Y2].setLabelFont(fnt);
	sp->coordinates()->axes[Y3].setLabelFont(fnt);
	sp->coordinates()->axes[Y4].setLabelFont(fnt);
}

void Graph3D::setZAxisLabelFont(const QFont& fnt)
{
	sp->coordinates()->axes[Z1].setLabelFont(fnt);
	sp->coordinates()->axes[Z2].setLabelFont(fnt);
	sp->coordinates()->axes[Z3].setLabelFont(fnt);
	sp->coordinates()->axes[Z4].setLabelFont(fnt);
}

void Graph3D::setXAxisLabelFont(const QStringList& lst)
{
	QFont fnt=QFont(lst[1],lst[2].toInt(),lst[3].toInt(),lst[4].toInt());
	sp->coordinates()->axes[X1].setLabelFont(fnt);
	sp->coordinates()->axes[X2].setLabelFont(fnt);
	sp->coordinates()->axes[X3].setLabelFont(fnt);
	sp->coordinates()->axes[X4].setLabelFont(fnt);
}

void Graph3D::setYAxisLabelFont(const QStringList& lst)
{
	QFont fnt=QFont(lst[1],lst[2].toInt(),lst[3].toInt(),lst[4].toInt());
	sp->coordinates()->axes[Y1].setLabelFont(fnt);
	sp->coordinates()->axes[Y2].setLabelFont(fnt);
	sp->coordinates()->axes[Y3].setLabelFont(fnt);
	sp->coordinates()->axes[Y4].setLabelFont(fnt);
}

void Graph3D::setZAxisLabelFont(const QStringList& lst)
{
	QFont fnt=QFont(lst[1],lst[2].toInt(),lst[3].toInt(),lst[4].toInt());
	sp->coordinates()->axes[Z1].setLabelFont(fnt);
	sp->coordinates()->axes[Z2].setLabelFont(fnt);
	sp->coordinates()->axes[Z3].setLabelFont(fnt);
	sp->coordinates()->axes[Z4].setLabelFont(fnt);
}

QStringList Graph3D::axisTickLengths()
{
	QStringList lst;
	double majorl,minorl;

	sp->coordinates()->axes[X1].ticLength (majorl,minorl);
	lst<<QString::number(majorl);
	lst<<QString::number(minorl);

	sp->coordinates()->axes[Y1].ticLength (majorl,minorl);
	lst<<QString::number(majorl);
	lst<<QString::number(minorl);

	sp->coordinates()->axes[Z1].ticLength (majorl,minorl);
	lst<<QString::number(majorl);
	lst<<QString::number(minorl);

	return lst;
}

void Graph3D::setTickLengths(const QStringList& lst)
{
	double majorl, minorl;
	QStringList tick_length = lst;
	if (int(lst.count()) > 6)
		tick_length.remove(tick_length.first());

	majorl=tick_length[0].toDouble();
	minorl=tick_length[1].toDouble();
	sp->coordinates()->axes[X1].setTicLength (majorl,minorl);
	sp->coordinates()->axes[X2].setTicLength (majorl,minorl);
	sp->coordinates()->axes[X3].setTicLength (majorl,minorl);
	sp->coordinates()->axes[X4].setTicLength (majorl,minorl);

	majorl=tick_length[2].toDouble();
	minorl=tick_length[3].toDouble();
	sp->coordinates()->axes[Y1].setTicLength (majorl,minorl);
	sp->coordinates()->axes[Y2].setTicLength (majorl,minorl);
	sp->coordinates()->axes[Y3].setTicLength (majorl,minorl);
	sp->coordinates()->axes[Y4].setTicLength (majorl,minorl);

	majorl=tick_length[4].toDouble();
	minorl=tick_length[5].toDouble();
	sp->coordinates()->axes[Z1].setTicLength (majorl,minorl);
	sp->coordinates()->axes[Z2].setTicLength (majorl,minorl);
	sp->coordinates()->axes[Z3].setTicLength (majorl,minorl);
	sp->coordinates()->axes[Z4].setTicLength (majorl,minorl);
}

void Graph3D::setXAxisTickLength(double majorLength, double minorLength)
{
	double majorl, minorl;
	sp->coordinates()->axes[X1].ticLength (majorl,minorl);
	if (majorl != majorLength || minorl != minorLength){
		sp->coordinates()->axes[X1].setTicLength (majorLength,minorLength);
		sp->coordinates()->axes[X2].setTicLength (majorLength,minorLength);
		sp->coordinates()->axes[X3].setTicLength (majorLength,minorLength);
		sp->coordinates()->axes[X4].setTicLength (majorLength,minorLength);
		}
}

void Graph3D::setYAxisTickLength(double majorLength, double minorLength)
{
	double majorl, minorl;
	sp->coordinates()->axes[Y1].ticLength (majorl,minorl);
	if (majorl != majorLength || minorl != minorLength){
		sp->coordinates()->axes[Y1].setTicLength (majorLength,minorLength);
		sp->coordinates()->axes[Y2].setTicLength (majorLength,minorLength);
		sp->coordinates()->axes[Y3].setTicLength (majorLength,minorLength);
		sp->coordinates()->axes[Y4].setTicLength (majorLength,minorLength);
	}
}

void Graph3D::setZAxisTickLength(double majorLength, double minorLength)
{
	double majorl, minorl;
	sp->coordinates()->axes[Z1].ticLength (majorl,minorl);
	if (majorl != majorLength || minorl != minorLength){
		sp->coordinates()->axes[Z1].setTicLength (majorLength,minorLength);
		sp->coordinates()->axes[Z2].setTicLength (majorLength,minorLength);
		sp->coordinates()->axes[Z3].setTicLength (majorLength,minorLength);
		sp->coordinates()->axes[Z4].setTicLength (majorLength,minorLength);
	}
}

void Graph3D::setAxisTickLength(int axis, double majorLength, double minorLength)
{
	double majorl, minorl;
	switch(axis)
	{
		case 0:
			sp->coordinates()->axes[X1].ticLength (majorl,minorl);
			if (majorl != majorLength || minorl != minorLength){
				sp->coordinates()->axes[X1].setTicLength (majorLength,minorLength);
				sp->coordinates()->axes[X2].setTicLength (majorLength,minorLength);
				sp->coordinates()->axes[X3].setTicLength (majorLength,minorLength);
				sp->coordinates()->axes[X4].setTicLength (majorLength,minorLength);
			}
			break;

		case 1:
			sp->coordinates()->axes[Y1].ticLength (majorl,minorl);
			if (majorl != majorLength || minorl != minorLength){
				sp->coordinates()->axes[Y1].setTicLength (majorLength,minorLength);
				sp->coordinates()->axes[Y2].setTicLength (majorLength,minorLength);
				sp->coordinates()->axes[Y3].setTicLength (majorLength,minorLength);
				sp->coordinates()->axes[Y4].setTicLength (majorLength,minorLength);
			}
			break;

		case 2:
			sp->coordinates()->axes[Z1].ticLength (majorl,minorl);
			if (majorl != majorLength || minorl != minorLength){
				sp->coordinates()->axes[Z1].setTicLength (majorLength,minorLength);
				sp->coordinates()->axes[Z2].setTicLength (majorLength,minorLength);
				sp->coordinates()->axes[Z3].setTicLength (majorLength,minorLength);
				sp->coordinates()->axes[Z4].setTicLength (majorLength,minorLength);
			}
			break;
	}
	sp->updateGL();
}

void Graph3D::rotationChanged(double, double, double)
{
	emit modified();
}

void Graph3D::scaleChanged(double, double , double )
{	emit modified();
}

void Graph3D::shiftChanged(double, double, double)
{	
	emit modified();
}

void Graph3D::zoomChanged(double)
{	
  	emit modified();
}

void Graph3D::resetAxesLabels()
{
	sp->coordinates()->axes[X1].setLabelString(labels[0]);
	sp->coordinates()->axes[X2].setLabelString(labels[0]);
	sp->coordinates()->axes[X3].setLabelString(labels[0]);
	sp->coordinates()->axes[X4].setLabelString(labels[0]);

	sp->coordinates()->axes[Y1].setLabelString(labels[1]);
	sp->coordinates()->axes[Y2].setLabelString(labels[1]);
	sp->coordinates()->axes[Y3].setLabelString(labels[1]);
	sp->coordinates()->axes[Y4].setLabelString(labels[1]);

	sp->coordinates()->axes[Z1].setLabelString(labels[2]);
	sp->coordinates()->axes[Z2].setLabelString(labels[2]);
	sp->coordinates()->axes[Z3].setLabelString(labels[2]);
	sp->coordinates()->axes[Z4].setLabelString(labels[2]);
}

void Graph3D::setAxesLabels(const QStringList& l)
{
	QString label = l[0];
	sp->coordinates()->axes[X1].setLabelString(label);
	sp->coordinates()->axes[X2].setLabelString(label);
	sp->coordinates()->axes[X3].setLabelString(label);
	sp->coordinates()->axes[X4].setLabelString(label);

	label = l[1];
	sp->coordinates()->axes[Y1].setLabelString(label);
	sp->coordinates()->axes[Y2].setLabelString(label);
	sp->coordinates()->axes[Y3].setLabelString(label);
	sp->coordinates()->axes[Y4].setLabelString(label);

	label = l[2];
	sp->coordinates()->axes[Z1].setLabelString(label);
	sp->coordinates()->axes[Z2].setLabelString(label);
	sp->coordinates()->axes[Z3].setLabelString(label);
	sp->coordinates()->axes[Z4].setLabelString(label);

	labels = l;
}

void Graph3D::setXAxisLabel(const QString& label)
{
    if (labels[0] != label){
        sp->coordinates()->axes[X1].setLabelString(label);
        sp->coordinates()->axes[X2].setLabelString(label);
        sp->coordinates()->axes[X3].setLabelString(label);
        sp->coordinates()->axes[X4].setLabelString(label);
        labels[0]=label;
    }

	sp->makeCurrent();
	sp->updateGL();
	emit modified();
}

void Graph3D::setYAxisLabel(const QString& label)
{
    if (labels[1] != label){
        sp->coordinates()->axes[Y1].setLabelString(label);
        sp->coordinates()->axes[Y2].setLabelString(label);
        sp->coordinates()->axes[Y3].setLabelString(label);
        sp->coordinates()->axes[Y4].setLabelString(label);
        labels[1]=label;
    }

	sp->makeCurrent();
	sp->updateGL();
	emit modified();
}

void Graph3D::setZAxisLabel(const QString& label)
{
    if (labels[2] != label){
        sp->coordinates()->axes[Z1].setLabelString(label);
        sp->coordinates()->axes[Z2].setLabelString(label);
        sp->coordinates()->axes[Z3].setLabelString(label);
        sp->coordinates()->axes[Z4].setLabelString(label);
        labels[2]=label;
    }

	sp->makeCurrent();
	sp->updateGL();
	emit modified();
}

QFont Graph3D::xAxisLabelFont()
{
	return sp->coordinates()->axes[X1].labelFont();
}

QFont Graph3D::yAxisLabelFont()
{
	return sp->coordinates()->axes[Y1].labelFont();
}

QFont Graph3D::zAxisLabelFont()
{
	return sp->coordinates()->axes[Z1].labelFont();
}

double Graph3D::xStart()
{
	double start,stop;
	sp->coordinates()->axes[X1].limits (start,stop);
	return start;
}

double Graph3D::xStop()
{
	double start,stop;
	sp->coordinates()->axes[X1].limits (start,stop);
	return stop;
}

double Graph3D::yStart()
{
	double start,stop;
	sp->coordinates()->axes[Y1].limits (start,stop);
	return start;
}

double Graph3D::yStop()
{
	double start,stop;
	sp->coordinates()->axes[Y1].limits (start,stop);
	return stop;
}

double Graph3D::zStart()
{
	double start,stop;
	sp->coordinates()->axes[Z1].limits (start,stop);
	return start;
}

double Graph3D::zStop()
{
	double start,stop;
	sp->coordinates()->axes[Z1].limits(start, stop);
	return stop;
}

QStringList Graph3D::scaleLimits()
{
	QStringList limits;
	double start, stop;
	int majors, minors;

	sp->coordinates()->axes[X1].limits (start,stop);
	majors=sp->coordinates()->axes[X1].majors();
	minors=sp->coordinates()->axes[X1].minors();

	limits<<QString::number(start);
	limits<<QString::number(stop);
	limits<<QString::number(majors);
	limits<<QString::number(minors);
	limits<<QString::number(scaleType[0]);

	sp->coordinates()->axes[Y1].limits (start,stop);
	majors=sp->coordinates()->axes[Y1].majors();
	minors=sp->coordinates()->axes[Y1].minors();

	limits<<QString::number(start);
	limits<<QString::number(stop);
	limits<<QString::number(majors);
	limits<<QString::number(minors);
	limits<<QString::number(scaleType[1]);

	sp->coordinates()->axes[Z1].limits (start,stop);
	majors=sp->coordinates()->axes[Z1].majors();
	minors=sp->coordinates()->axes[Z1].minors();

	limits<<QString::number(start);
	limits<<QString::number(stop);
	limits<<QString::number(majors);
	limits<<QString::number(minors);
	limits<<QString::number(scaleType[2]);

	return limits;
}

QStringList Graph3D::scaleTicks()
{
	QStringList limits;
	int majors,minors;

	majors=sp->coordinates()->axes[X1].majors();
	minors=sp->coordinates()->axes[X1].minors();
	limits<<QString::number(majors);
	limits<<QString::number(minors);

	majors=sp->coordinates()->axes[Y1].majors();
	minors=sp->coordinates()->axes[Y1].minors();
	limits<<QString::number(majors);
	limits<<QString::number(minors);

	majors=sp->coordinates()->axes[Z1].majors();
	minors=sp->coordinates()->axes[Z1].minors();
	limits<<QString::number(majors);
	limits<<QString::number(minors);

	return limits;
}

void Graph3D::updateScale(int axis, const QStringList& options)
{
	QString st = QString::number(scaleType[axis]);
	double start, stop, xl, xr, yl, yr;
	int majors, minors, newMaj, newMin;

	sp->makeCurrent();
	switch(axis)
	{
		case 0:
			majors=sp->coordinates()->axes[X1].majors ();
  	        minors=sp->coordinates()->axes[X1].minors ();
			sp->coordinates()->axes[X1].limits(xl,xr);
			if (xl !=options[0].toDouble() || xr != options[1].toDouble())
			{
				xl=options[0].toDouble();
				xr=options[1].toDouble();
				sp->coordinates()->axes[Y1].limits(yl,yr);
				sp->coordinates()->axes[Z1].limits(start,stop);

				if (d_func){
					d_func->setDomain(xl,xr,yl,yr);
					d_func->create ();
					sp->createCoordinateSystem(Triple(xl, yl, start), Triple(xr, yr, stop));
				} else if (d_surface){
					d_surface->restrictRange (ParallelEpiped(Triple(xl, yl, start), Triple(xr, yr, stop)));
					d_surface->create();
					sp->createCoordinateSystem(Triple(xl, yl, start), Triple(xr, yr, stop));
				} else
					setScales(xl, xr, yl, yr, start, stop);
			}

			if(st != options[4]){
				if (options[4]=="0"){
					sp->coordinates()->axes[X1].setScale (LINEARSCALE);
					scaleType[axis]=0;
				} else {
					sp->coordinates()->axes[X1].setScale (LOG10SCALE);
					scaleType[axis]=1;
				}
			}

			newMaj = options[2].toInt();
			if (majors != newMaj){
				sp->coordinates()->axes[X1].setMajors(newMaj);
				sp->coordinates()->axes[X2].setMajors(newMaj);
				sp->coordinates()->axes[X3].setMajors(newMaj);
				sp->coordinates()->axes[X4].setMajors(newMaj);
			}

			newMin = options[3].toInt();
			if (minors != newMin){
				sp->coordinates()->axes[X1].setMinors(newMin);
				sp->coordinates()->axes[X2].setMinors(newMin);
				sp->coordinates()->axes[X3].setMinors(newMin);
				sp->coordinates()->axes[X4].setMinors(newMin);
			}
			break;

		case 1:
			majors = sp->coordinates()->axes[Y1].majors ();
  	        minors = sp->coordinates()->axes[Y1].minors ();
			sp->coordinates()->axes[Y1].limits(yl, yr);
			if (yl != options[0].toDouble() || yr != options[1].toDouble()){
				yl = options[0].toDouble();
				yr = options[1].toDouble();
				sp->coordinates()->axes[X1].limits(xl, xr);
				sp->coordinates()->axes[Z1].limits(start, stop);

				if (d_func){
					d_func->setDomain(xl, xr, yl, yr);
					d_func->create();
					sp->createCoordinateSystem(Triple(xl, yl, start), Triple(xr, yr, stop));
				} else if (d_surface){
					d_surface->restrictRange (ParallelEpiped(Triple(xl, yl, start), Triple(xr, yr, stop)));
					d_surface->create();
					sp->createCoordinateSystem(Triple(xl, yl, start), Triple(xr, yr, stop));
				} else
					setScales(xl, xr, yl, yr, start, stop);
			}

			newMaj = options[2].toInt();
			if (majors != newMaj ){
				sp->coordinates()->axes[Y1].setMajors(newMaj);
				sp->coordinates()->axes[Y2].setMajors(newMaj);
				sp->coordinates()->axes[Y3].setMajors(newMaj);
				sp->coordinates()->axes[Y4].setMajors(newMaj);
			}

            newMin = options[3].toInt();
			if (minors != newMin){
				sp->coordinates()->axes[Y1].setMinors(newMin);
				sp->coordinates()->axes[Y2].setMinors(newMin);
				sp->coordinates()->axes[Y3].setMinors(newMin);
				sp->coordinates()->axes[Y4].setMinors(newMin);
			}

			if(st != options[4]){
				if (options[4]=="0"){
					sp->coordinates()->axes[Y1].setScale (LINEARSCALE);
					scaleType[axis]=0;
				} else {
					sp->coordinates()->axes[Y1].setScale (LOG10SCALE);
					scaleType[axis]=1;
				}
			}
			break;

		case 2:
			majors=sp->coordinates()->axes[Z1].majors();
			minors=sp->coordinates()->axes[Z1].minors();
			sp->coordinates()->axes[Z1].limits(start,stop);
			if (start != options[0].toDouble() || stop != options[1].toDouble())
			{
				start=options[0].toDouble();
				stop=options[1].toDouble();
				sp->coordinates()->axes[X1].limits(xl,xr);
  	            sp->coordinates()->axes[Y1].limits(yl,yr);
				if (d_func){
					d_func->setMinZ(start);
					d_func->setMaxZ(stop);
					d_func->create ();
					sp->createCoordinateSystem(Triple(xl, yl, start), Triple(xr, yr, stop));
				} else if (d_surface){
					d_surface->restrictRange (ParallelEpiped(Triple(xl, yl, start), Triple(xr, yr, stop)));
					d_surface->create();
					sp->createCoordinateSystem(Triple(xl, yl, start), Triple(xr, yr, stop));
				} else
					setScales(xl, xr, yl, yr, start, stop);
				sp->legend()->setLimits(start,stop);
			}

			newMaj= options[2].toInt();
			if (majors != newMaj ){
				sp->coordinates()->axes[Z1].setMajors(newMaj);
				sp->coordinates()->axes[Z2].setMajors(newMaj);
				sp->coordinates()->axes[Z3].setMajors(newMaj);
				sp->coordinates()->axes[Z4].setMajors(newMaj);
			}

			newMin = options[3].toInt();
			if (minors != newMin){
				sp->coordinates()->axes[Z1].setMinors(newMin);
				sp->coordinates()->axes[Z2].setMinors(newMin);
				sp->coordinates()->axes[Z3].setMinors(newMin);
				sp->coordinates()->axes[Z4].setMinors(newMin);
			}
			if(st != options[4]){
  	         	if (options[4]=="0"){
  	             	sp->coordinates()->axes[Z1].setScale (LINEARSCALE);
  	            	scaleType[axis]=0;
  	            } else {
  	            	sp->coordinates()->axes[Z1].setScale (LOG10SCALE);
  	                scaleType[axis]=1;
  	            }
  	        }
			break;
	}
	update();
	emit modified();
}

void Graph3D::setScales(double xl, double xr, double yl, double yr, double zl, double zr)
{
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	if (d_matrix)
		updateScalesFromMatrix(xl, xr, yl, yr, zl, zr);
    else if (d_func){
        d_func->setDomain(xl, xr, yl, yr);
        d_func->setMinZ(zl);
        d_func->setMaxZ(zr);
        d_func->create ();
        sp->createCoordinateSystem(Triple(xl, yl, zl), Triple(xr, yr, zr));
    } else if (d_table){
		QString name = plotAssociation;

		int pos = name.find("_", 0);
		int posX = name.find("(", pos);
		QString xColName = name.mid(pos+1, posX-pos-1);
		int xCol = d_table->colIndex(xColName);

		pos = name.find(",", posX);
		posX = name.find("(", pos);
		QString yColName = name.mid(pos+1, posX-pos-1);
		int yCol = d_table->colIndex(yColName);

		if (name.endsWith("(Z)",true)){
			pos = name.find(",",posX);
			posX = name.find("(",pos);
			QString zColName = name.mid(pos+1,posX-pos-1);
			int zCol = d_table->colIndex(zColName);

			loadData(d_table, xCol, yCol, zCol, xl, xr, yl, yr, zl, zr);
		} else if (name.endsWith("(Y)",true))
			updateScales(xl, xr, yl, yr, zl, zr, xCol, yCol);
	}
    resetAxesLabels();
	QApplication::restoreOverrideCursor();
}

void Graph3D::updateScalesFromMatrix(double xl, double xr, double yl,
		double yr, double zl, double zr)
{	double xStart = qMin(d_matrix->xStart(), d_matrix->xEnd());
	double xEnd = qMax(d_matrix->xStart(), d_matrix->xEnd());
	double yStart = qMin(d_matrix->yStart(), d_matrix->yEnd());
	double yEnd = qMax(d_matrix->yStart(), d_matrix->yEnd());

	double dx = fabs((xEnd - xStart)/double(d_matrix->numCols()-1));
	double dy = fabs((yEnd - yStart)/double(d_matrix->numRows()-1));

	int nc = int(fabs(xr - xl)/dx)+1;
	int nr = int(fabs(yr - yl)/dy)+1;

    double x_begin = qMin(xl, xr);
	double y_begin = qMin(yl, yr);

	double **data_matrix = Matrix::allocateMatrixData(nc, nr);
	for (int i = 0; i < nc; i++){
		double x = x_begin + i*dx;
        double dli,dlf;
        dlf = modf(abs((x - xStart)/dx),&dli);
        int l = dli; if (dlf > 0.5) l++;
		for (int j = 0; j < nr; j++){
			double y = y_begin + j*dy;
			if (x >= xStart && x <= xEnd && y >= yStart && y <= yEnd){
                double dki,dkf;
                dkf = modf(abs((y - yStart)/dy),&dki);
                int k = dki; if (dkf > 0.5) k++;
				double val = d_matrix->cell(k, l);
				if (val > zr)
					data_matrix[i][j] = zr;
				else if (val < zl)
					data_matrix[i][j] = zl;
				else
					data_matrix[i][j] = val;
			} else
				data_matrix[i][j] = 0.0;
		}
	}
	sp->loadFromData(data_matrix, nc, nr, xl, xr, yl, yr);
	Matrix::freeMatrixData(data_matrix, nc);

	sp->createCoordinateSystem(Triple(xl, yl, zl), Triple(xr, yr, zr));
	sp->legend()->setLimits(zl, zr);
	sp->legend()->setMajors(legendMajorTicks);
	update();
}

void Graph3D::updateScales(double xl, double xr, double yl, double yr,double zl, double zr,
		int xcol, int ycol)
{
	int r=d_table->numRows();
	int xmesh=0, ymesh=2;
	double xv, yv;

	for (int i = 0; i < r; i++){
		if (!d_table->text(i,xcol).isEmpty() && !d_table->text(i,ycol).isEmpty()){
			xv=d_table->cell(i,xcol);
			if (xv >= xl && xv <= xr)
				xmesh++;
		}
	}

	if (xmesh == 0)
		xmesh++;

	double **data = Matrix::allocateMatrixData(xmesh, ymesh);

	for (int j = 0; j < ymesh; j++){
		int k=0;
		for (int i = 0; i < r; i++){
			if (!d_table->text(i,xcol).isEmpty() && !d_table->text(i,ycol).isEmpty()){
				xv=d_table->cell(i,xcol);
				if (xv >= xl && xv <= xr){
					yv=d_table->cell(i,ycol);
					if (yv > zr)
						data[k][j] = zr;
					else if (yv < zl)
						data[k][j] = zl;
					else
						data[k][j] = yv;
					k++;
				}
			}
		}
	}

	sp->loadFromData(data, xmesh, ymesh, xl, xr, yl, yr);
	sp->createCoordinateSystem(Triple(xl, yl, zl), Triple(xr, yr, zr));
	Matrix::freeMatrixData(data, xmesh);
}

void Graph3D::setTicks(const QStringList& options)
{
	int min,maj;
	if (int(options.count()) == 6)
	{
		maj=options[0].toInt();
		sp->coordinates()->axes[X1].setMajors(maj);
		sp->coordinates()->axes[X2].setMajors(maj);
		sp->coordinates()->axes[X3].setMajors(maj);
		sp->coordinates()->axes[X4].setMajors(maj);

		min=options[1].toInt();
		sp->coordinates()->axes[X1].setMinors(min);
		sp->coordinates()->axes[X2].setMinors(min);
		sp->coordinates()->axes[X3].setMinors(min);
		sp->coordinates()->axes[X4].setMinors(min);

		maj=options[2].toInt();
		sp->coordinates()->axes[Y1].setMajors(maj);
		sp->coordinates()->axes[Y2].setMajors(maj);
		sp->coordinates()->axes[Y3].setMajors(maj);
		sp->coordinates()->axes[Y4].setMajors(maj);

		min=options[3].toInt();
		sp->coordinates()->axes[Y1].setMinors(min);
		sp->coordinates()->axes[Y2].setMinors(min);
		sp->coordinates()->axes[Y3].setMinors(min);
		sp->coordinates()->axes[Y4].setMinors(min);

		maj=options[4].toInt();
		sp->coordinates()->axes[Z1].setMajors(maj);
		sp->coordinates()->axes[Z2].setMajors(maj);
		sp->coordinates()->axes[Z3].setMajors(maj);
		sp->coordinates()->axes[Z4].setMajors(maj);

		min=options[5].toInt();
		sp->coordinates()->axes[Z1].setMinors(min);
		sp->coordinates()->axes[Z2].setMinors(min);
		sp->coordinates()->axes[Z3].setMinors(min);
		sp->coordinates()->axes[Z4].setMinors(min);
	}
	else
	{
		maj=options[1].toInt();
		sp->coordinates()->axes[X1].setMajors(maj);
		sp->coordinates()->axes[X2].setMajors(maj);
		sp->coordinates()->axes[X3].setMajors(maj);
		sp->coordinates()->axes[X4].setMajors(maj);

		min=options[2].toInt();
		sp->coordinates()->axes[X1].setMinors(min);
		sp->coordinates()->axes[X2].setMinors(min);
		sp->coordinates()->axes[X3].setMinors(min);
		sp->coordinates()->axes[X4].setMinors(min);

		maj=options[3].toInt();
		sp->coordinates()->axes[Y1].setMajors(maj);
		sp->coordinates()->axes[Y2].setMajors(maj);
		sp->coordinates()->axes[Y3].setMajors(maj);
		sp->coordinates()->axes[Y4].setMajors(maj);

		min=options[4].toInt();
		sp->coordinates()->axes[Y1].setMinors(min);
		sp->coordinates()->axes[Y2].setMinors(min);
		sp->coordinates()->axes[Y3].setMinors(min);
		sp->coordinates()->axes[Y4].setMinors(min);

		maj=options[5].toInt();
		sp->coordinates()->axes[Z1].setMajors(maj);
		sp->coordinates()->axes[Z2].setMajors(maj);
		sp->coordinates()->axes[Z3].setMajors(maj);
		sp->coordinates()->axes[Z4].setMajors(maj);

		min=options[6].toInt();
		sp->coordinates()->axes[Z1].setMinors(min);
		sp->coordinates()->axes[Z2].setMinors(min);
		sp->coordinates()->axes[Z3].setMinors(min);
		sp->coordinates()->axes[Z4].setMinors(min);
	}
}

void Graph3D::setMeshColor(const QColor& meshColor)
{
	if (meshCol != meshColor){
		sp->setMeshColor(Qt2GL(meshColor));
		meshCol=meshColor;
	}
}

void Graph3D::setAxesColor(const QColor& axesColor)
{
	if(axesCol != axesColor){
		sp->coordinates()->setAxesColor(Qt2GL(axesColor));
		axesCol=axesColor;
	}
}

void Graph3D::setNumbersColor(const QColor& numColor)
{
	if(numCol !=numColor){
		sp->coordinates()->setNumberColor(Qt2GL(numColor));
		numCol=numColor;
	}
}

void Graph3D::setLabelsColor(const QColor& labelColor)
{
	if(labelsCol !=labelColor){
		sp->coordinates()->setLabelColor(Qt2GL(labelColor));
		labelsCol=labelColor;
	}
}

void Graph3D::setBackgroundColor(const QColor& bgColor)
{
	if(bgCol !=bgColor){
		sp->setBackgroundColor(Qt2GL(bgColor));
		bgCol=bgColor;
	}
}

void Graph3D::setGridColor(const QColor& gridColor)
{
	if(gridCol !=gridColor){
		sp->coordinates()->setGridLinesColor(Qt2GL(gridColor));
		gridCol=gridColor;
	}
}

void Graph3D::setColors(const QStringList& colors)
{
	meshCol=QColor(colors[1]);
	sp->setMeshColor(Qt2GL(meshCol));

	axesCol=QColor(colors[2]);
	sp->coordinates()->setAxesColor(Qt2GL(axesCol));

	numCol=QColor(colors[3]);
	sp->coordinates()->setNumberColor(Qt2GL(numCol));

	labelsCol=QColor(colors[4]);
	sp->coordinates()->setLabelColor(Qt2GL(labelsCol));

	bgCol=QColor(colors[5]);
	sp->setBackgroundColor(Qt2GL(bgCol));

	gridCol=QColor(colors[6]);
	sp->coordinates()->setGridLinesColor(Qt2GL(gridCol));

	if ((int)colors.count()>7){
		QColor min=QColor(colors[7]);
		QColor max=QColor(colors[8]);
		alpha = colors[9].toDouble();
		if ((int)colors.count() == 11)
            setDataColorMap(colors[10]);
        else
		    setDataColors(min,max);
	}
}

void Graph3D::scaleFonts(double factor)
{
	QFont font = sp->coordinates()->axes[X1].numberFont();
	font.setPointSizeFloat(font.pointSizeFloat()*factor);
	sp->coordinates()->setNumberFont (font);

	titleFnt.setPointSizeFloat(factor*titleFnt.pointSizeFloat());
	sp->setTitleFont(titleFnt.family(),titleFnt.pointSize(),titleFnt.weight(),titleFnt.italic());

	font = xAxisLabelFont();
	font.setPointSizeFloat(factor*font.pointSizeFloat());
	setXAxisLabelFont(font);

	font = yAxisLabelFont();
	font.setPointSizeFloat(factor*font.pointSizeFloat());
	setYAxisLabelFont(font);

	font = zAxisLabelFont();
	font.setPointSizeFloat(factor*font.pointSizeFloat());
	setZAxisLabelFont(font);
}

void Graph3D::resizeEvent(QResizeEvent *e)
{
	if (!ignoreFonts && this->isVisible()){
		double ratio=(double)e->size().height()/(double)e->oldSize().height();
		scaleFonts(ratio);
	}
	emit resizedWindow(this);
	emit modified();
	QMdiSubWindow::resizeEvent(e);
}

void Graph3D::setFramed()
{
	if (sp->coordinates()->style() == FRAME)
		return;

	sp->makeCurrent();
	sp->setCoordinateStyle(FRAME);
}

void Graph3D::setBoxed()
{
	if (sp->coordinates()->style() == BOX)
		return;

	sp->makeCurrent();
	sp->setCoordinateStyle(BOX);
}

void Graph3D::setNoAxes()
{
	if (sp->coordinates()->style() == NOCOORD)
		return;

	sp->makeCurrent();
	sp->setCoordinateStyle(NOCOORD);
}

void Graph3D::setPolygonStyle()
{
	if (sp->plotStyle() == FILLED)
		return;

	sp->makeCurrent();
	sp->setPlotStyle(FILLED);
	sp->updateData();
	sp->updateGL();

	style_=FILLED;
	pointStyle = None;
}

void Graph3D::setFilledMeshStyle()
{
	if (sp->plotStyle() == FILLEDMESH)
		return;

	sp->makeCurrent();
	sp->setPlotStyle(FILLEDMESH);
	sp->updateData();
	sp->updateGL();

	style_=FILLEDMESH;
	pointStyle = None;
}

void Graph3D::setHiddenLineStyle()
{
	if (sp->plotStyle() == HIDDENLINE)
		return;

	sp->makeCurrent();
	sp->setPlotStyle(HIDDENLINE);
	sp->showColorLegend(false);
	sp->updateData();
	sp->updateGL();

	style_=HIDDENLINE;
	pointStyle = None;
	legendOn=false;
}

void Graph3D::setWireframeStyle()
{
	if (sp->plotStyle() == WIREFRAME)
		return;

	sp->makeCurrent();
	sp->setPlotStyle(WIREFRAME);
	sp->showColorLegend(false);
	sp->updateData();
	sp->updateGL();

	pointStyle = None;
	style_=WIREFRAME;
	legendOn=false;
}

void Graph3D::setDotStyle()
{
	pointStyle=Dots;
	style_=Qwt3D::USER;

	sp->makeCurrent();
	sp->setPlotStyle(Dot(d_point_size, d_smooth_points));
	sp->updateData();
	sp->updateGL();
}

void Graph3D::setConeStyle()
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	pointStyle=Cones;
	style_=Qwt3D::USER;

	sp->makeCurrent();
	sp->setPlotStyle(Cone3D(conesRad, conesQuality));
	sp->updateData();
	sp->updateGL();

	QApplication::restoreOverrideCursor();
}

void Graph3D::setCrossStyle()
{
	pointStyle=HairCross;
	style_=Qwt3D::USER;

	sp->makeCurrent();
	sp->setPlotStyle(CrossHair(crossHairRad, crossHairLineWidth,crossHairSmooth,crossHairBoxed));
	sp->updateData();
	sp->updateGL();
}

void Graph3D::clearData()
{
	if (d_matrix)
		d_matrix = 0;
	else if (d_table)
		d_table = 0;
	else if (d_func){
		delete d_func;
		d_func = 0;
	}
	plotAssociation = QString();
	sp->makeCurrent();
	sp->loadFromData (0, 0, 0, false, false);
	sp->updateData();
	sp->updateGL();
}

void Graph3D::setBarStyle()
{
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	pointStyle=VerticalBars;
	style_=Qwt3D::USER;

	sp->makeCurrent();
	sp->setPlotStyle(Bar(barsRad));
	sp->updateData();
	sp->updateGL();
	QApplication::restoreOverrideCursor();
}

void Graph3D::setFloorData()
{
	if (sp->floorStyle() == FLOORDATA)
		return;

	sp->makeCurrent();
	sp->setFloorStyle(FLOORDATA);
	sp->updateData();
	sp->updateGL();
}

void Graph3D::setFloorIsolines()
{
	if (sp->floorStyle() == FLOORISO)
		return;

	sp->makeCurrent();
	sp->setFloorStyle(FLOORISO);
	sp->updateData();
	sp->updateGL();
}

void Graph3D::setEmptyFloor()
{
	if (sp->floorStyle() == NOFLOOR)
		return;

	sp->makeCurrent();
	sp->setFloorStyle(NOFLOOR);
	sp->updateData();
	sp->updateGL();
}

void Graph3D::setMeshLineWidth(double lw)
{
	if (sp->meshLineWidth() == lw)
		return;

	sp->makeCurrent();
	sp->setMeshLineWidth(lw);
	sp->updateData();
	sp->updateGL();
}

int Graph3D::grids()
{
	return sp->coordinates()->grids();
}

void Graph3D::setGrid(Qwt3D::SIDE s, bool b)
{
	if (!sp)
		return;

	int sum = sp->coordinates()->grids();

	if (b)
		sum |= s;
	else
		sum &= ~s;

	sp->coordinates()->setGridLines(sum!=Qwt3D::NOSIDEGRID, false, sum);
	sp->updateGL();
	emit modified();
}

void Graph3D::setGrid(int grids)
{
	if (!sp)
		return;

	sp->coordinates()->setGridLines(true, false, grids);
}

void Graph3D::setLeftGrid(bool b)
{
	setGrid(Qwt3D::LEFT,b);
}
void Graph3D::setRightGrid(bool b)
{
	setGrid(Qwt3D::RIGHT,b);
}
void Graph3D::setCeilGrid(bool b)
{
	setGrid(Qwt3D::CEIL,b);
}
void Graph3D::setFloorGrid(bool b)
{
	setGrid(Qwt3D::FLOOR,b);
}
void Graph3D::setFrontGrid(bool b)
{
	setGrid(Qwt3D::FRONT,b);
}
void Graph3D::setBackGrid(bool b)
{
	setGrid(Qwt3D::BACK,b);
}

void Graph3D::print()
{
	QPrinter printer;
	if (width() > height())
        printer.setOrientation(QPrinter::Landscape);
    else
        printer.setOrientation(QPrinter::Portrait);
	printer.setColorMode (QPrinter::Color);
	printer.setFullPage(false);
	if (printer.setup()){
        QImage im = sp->grabFrameBuffer(true);
        QPainter paint(&printer);
        paint.drawImage(printer.pageRect(), im);
        paint.end();
	}
}

void Graph3D::copyImage()
{
    QApplication::clipboard()->setPixmap(sp->renderPixmap(), QClipboard::Clipboard);
    sp->updateData();
}

void Graph3D::exportImage(const QString& fileName, int quality, bool transparent)
{
	if (transparent){
        QPixmap pic = sp->renderPixmap();
        sp->updateData();

        QBitmap mask(pic.size());
		mask.fill(Qt::color1);
		QPainter p;
		p.begin(&mask);
		p.setPen(Qt::color0);

		QColor background = QColor (Qt::white);
		QRgb backgroundPixel = background.rgb ();
		QImage image = pic.convertToImage();
		for (int y=0; y<image.height(); y++){
			for ( int x=0; x<image.width(); x++ ){
				QRgb rgb = image.pixel(x, y);
				if (rgb == backgroundPixel) // we want the frame transparent
					p.drawPoint( x, y );
			}
		}
		p.end();
		pic.setMask(mask);
		pic.save(fileName, 0, quality);
	} else {
        QImage im = sp->grabFrameBuffer(true);
        QImageWriter iw(fileName);
        iw.setQuality(quality);
        iw.write(im);
    }
}

void Graph3D::exportPDF(const QString& fileName)
{
	exportVector(fileName);
}

void Graph3D::exportVector(const QString& fileName)
{
	if ( fileName.isEmpty() ){
		QMessageBox::critical(this, tr("MantidPlot - Error"), tr("Please provide a valid file name!"));
        return;
	}

    QString format = "PDF";
    if (fileName.endsWith(".eps", Qt::CaseInsensitive))
        format = "EPS";
    else if (fileName.endsWith(".ps", Qt::CaseInsensitive))
        format = "PS";
	else if (fileName.endsWith(".svg", Qt::CaseInsensitive))
        format = "SVG";

    VectorWriter * gl2ps = (VectorWriter*)IO::outputHandler(format);
    if (gl2ps)
      gl2ps->setTextMode(VectorWriter::NATIVE);

    IO::save(sp, fileName, format);
}

void Graph3D::exportToFile(const QString& fileName)
{
	if ( fileName.isEmpty() ){
		QMessageBox::critical(this, tr("MantidPlot - Error"), tr("Please provide a valid file name!"));
        return;
	}

	if (fileName.contains(".eps") || fileName.contains(".pdf") ||
		fileName.contains(".ps") || fileName.contains(".svg")){
		exportVector(fileName);
		return;
	} else {
		QList<QByteArray> list = QImageWriter::supportedImageFormats();
    	for(int i=0 ; i<list.count() ; i++){
			if (fileName.contains( "." + list[i].toLower())){
				exportImage(fileName);
				return;
			}
		}
    	QMessageBox::critical(this, tr("MantidPlot - Error"), tr("File format not handled, operation aborted!"));
	}
}

bool Graph3D::eventFilter(QObject *object, QEvent *e)
{
	if (e->type() == QEvent::MouseButtonDblClick && object == (QObject *)this->sp)
	{
		emit showOptionsDialog();
		return TRUE;
	}
    // Ticket #623
	//below code is added for  zoom in/zoom out the 3D graph 
	else if (e->type() == QEvent::MouseMove)
	{	
		QInputEvent *keyevent=dynamic_cast<QInputEvent*>(e);
		if(keyevent)
		{   double zoom=sp->zoom();
		    //ALT  key and mouse left button movement for zoomIn
			if(keyevent->modifiers ()==Qt::AltModifier )
			{	
				QMouseEvent* mouseEvent=dynamic_cast<QMouseEvent*>(e);
				if(mouseEvent)
				{
					//current y position of mouse
					int yPos=mouseEvent->globalY();
					if(m_PreviousYpos==0)
					{
						setZoom(zoom*1);
					}
					//if mouse is moved up zoomIn
					else if(yPos<m_PreviousYpos)
					{
						m_zoomOutScale=1;
						m_zoomInScale+=0.003125; 
						{
							setZoom(zoom*m_zoomInScale);
						}
					}
					else //ZoomOut if mouse is moved backwards
					{
						m_zoomInScale=1;
						m_zoomOutScale+=0.003125;
						//if(zoom >=(m_originalzoom/zoomMax))
							setZoom(zoom/m_zoomOutScale);
					}
					//storing the previous position
					m_PreviousYpos=yPos;
					return true;
				}//end of if mouseevent loop
			}//end of keyevent->modifiers() loop
					
		}//end of keyevent loop
		
	}
	// zooming on mouse wheel  rotation
	else if(e->type()== QEvent::Wheel)
	{
		QWheelEvent *wheelevent=dynamic_cast<QWheelEvent*>(e);
		if(wheelevent)
		{
			if (wheelevent->orientation() == Qt::Vertical) {

				double zoom=sp->zoom();
				int delta=wheelevent->delta() ;
				//zoom in on mouse wheel forward rotation  
				if(delta>0)
				{	m_zoomOutScale=1;
				m_zoomInScale+=0.003125; 
				setZoom(zoom*m_zoomInScale);
				}
				else 
				{	//zoom out on mouse wheel backward rotation  
					m_zoomInScale=1;
					m_zoomOutScale+=0.003125;
					setZoom(zoom/m_zoomOutScale);
				}
				return true;
			}
			
		}
	}
	
	return MdiSubWindow::eventFilter(object, e);
}

double Graph3D::barsRadius()
{
	if (sp->plotStyle() == Qwt3D::USER && sp->plotStyle() != Qwt3D::POINTS)
		return barsRad;
	else
		return 0.0;
}

void Graph3D::setBarRadius(double rad)
{
	if (barsRad == rad)
		return;

	barsRad = rad;
}

void Graph3D::setDotOptions(double size, bool smooth)
{
	d_point_size = size;
	d_smooth_points = smooth;
}

void Graph3D::setConeOptions(double rad, int quality)
{
	conesRad = rad;
	conesQuality = quality;
}

void Graph3D::setCrossOptions(double rad, double linewidth, bool smooth, bool boxed)
{
	crossHairRad = rad;
	crossHairLineWidth=linewidth;
	crossHairSmooth = smooth;
	crossHairBoxed = boxed;
}

void Graph3D::setStyle(const QStringList& st)
{
	if (st[1] =="nocoord")
		sp->setCoordinateStyle(NOCOORD);
	else if (st[1] =="frame")
		sp->setCoordinateStyle(FRAME);
	else if (st[1] =="box")
		sp->setCoordinateStyle(BOX);

	if (st[2] =="nofloor")
		sp->setFloorStyle(NOFLOOR);
	else if (st[2] =="flooriso")
		sp->setFloorStyle(FLOORISO);
	else if (st[2] =="floordata")
		sp->setFloorStyle(FLOORDATA);

	if (st[3] =="filledmesh")
		setFilledMeshStyle();
	else if (st[3] =="filled")
		setPolygonStyle();
	else if (st[3] =="points") {
		d_point_size = st[4].toDouble();
		d_smooth_points = false;
		if (st[5] == "1")
			d_smooth_points = true;
		setDotStyle();
	} else if (st[3] =="wireframe")
		setWireframeStyle();
	else if (st[3] =="hiddenline")
		setHiddenLineStyle();
	else if (st[3] =="bars") {
		barsRad = (st[4]).toDouble();
		setBarStyle();
	} else if (st[3] =="cones") {
		conesRad = (st[4]).toDouble();
		conesQuality = (st[5]).toInt();
		setConeStyle();
	} else if (st[3] =="cross") {
		crossHairRad = (st[4]).toDouble();
		crossHairLineWidth = (st[5]).toDouble();
		crossHairSmooth=false;
		if (st[6] == "1")
			crossHairSmooth=true;
		crossHairBoxed=false;
		if (st[7] == "1")
			crossHairBoxed=true;
		setCrossStyle();
	}
}

void Graph3D::customPlotStyle(int style)
{
	sp->makeCurrent();
	if (sp->plotStyle() == style)
		return;

	switch (style)
	{
		case WIREFRAME  :
			{
				sp->setPlotStyle(WIREFRAME);
				style_= WIREFRAME ;
				pointStyle = None;

				legendOn = false;
				sp->showColorLegend(legendOn);
				break;
			}

		case FILLED :
			{
				sp->setPlotStyle(FILLED );
				style_= FILLED;
				pointStyle = None;
				break;
			}

		case FILLEDMESH  :
			{
				sp->setPlotStyle(FILLEDMESH);
				style_= FILLEDMESH;
				pointStyle = None;
				break;
			}

		case HIDDENLINE:
			{
				sp->setPlotStyle(HIDDENLINE);
				style_= HIDDENLINE;
				pointStyle = None;
				legendOn = false;
				sp->showColorLegend(legendOn);
				break;
			}

		case Qwt3D::POINTS:
			{
				d_point_size = 5;
				d_smooth_points = true;
				pointStyle=Dots;
				style_ = Qwt3D::USER;

				Dot d(d_point_size, d_smooth_points);
				sp->setPlotStyle(d);
				break;
			}

		case Qwt3D::USER:
			{
				pointStyle = VerticalBars;
				style_ = Qwt3D::USER;
				sp->setPlotStyle(Bar(barsRad));
				break;
			}
	}

	sp->updateData();
	sp->updateGL();
}

void Graph3D::setRotation(double xVal, double yVal, double zVal)
{
	sp->setRotation(xVal, yVal, zVal);
}

void Graph3D::setZoom(double val)
{
	
	if (sp->zoom() == val)
		return;

    sp->setZoom(val);
}

void Graph3D::setScale(double  xVal, double  yVal, double  zVal)
{
    if (sp->xScale() == xVal && sp->yScale() == yVal && sp->zScale() == zVal)
        return;

	sp->setScale(xVal, yVal, zVal);
}

void Graph3D::setShift(double  xVal,double  yVal,double  zVal)
{
	sp->setShift(xVal, yVal, zVal);
}

Qwt3D::PLOTSTYLE Graph3D::plotStyle()
{
	return sp->plotStyle();
}

Qwt3D::FLOORSTYLE Graph3D::floorStyle()
{
	return sp->floorStyle();
}

Qwt3D::COORDSTYLE Graph3D::coordStyle()
{
	return sp->coordinates()->style();
}

QString Graph3D::formula()
{
	if (d_func)
		return d_func->function();
	else
		return plotAssociation;
}

QString Graph3D::saveToString(const QString& geometry, bool)
{
	QString s="<SurfacePlot>\n";
	s+= QString(name())+"\t";
	s+= birthDate() + "\n";
	s+= geometry;
	s+= "SurfaceFunction\t";

	sp->makeCurrent();
	if (d_func)
	{ s+="mantidMatrix3D\t";	s += d_func->function() + ";" + QString::number(d_func->columns()) + ";" + QString::number(d_func->rows()) + "\t";
	}
	else if (d_surface){
		s += d_surface->xFormula() + "," + d_surface->yFormula() + "," + d_surface->zFormula() + ",";
		s += QString::number(d_surface->uStart(), 'e', 15) + ",";
		s += QString::number(d_surface->uEnd(), 'e', 15) + ",";
		s += QString::number(d_surface->vStart(), 'e', 15) + ",";
		s += QString::number(d_surface->vEnd(), 'e', 15) + ",";
		s += QString::number(d_surface->columns()) + ",";
		s += QString::number(d_surface->rows()) + ",";
		s += QString::number(d_surface->uPeriodic()) + ",";
		s += QString::number(d_surface->vPeriodic());
	} else { 
		s += plotAssociation;
		s += "\t";
	}
	double start,stop;
	sp->coordinates()->axes[X1].limits(start,stop);
	s+=QString::number(start)+"\t";
	s+=QString::number(stop)+"\t";
	sp->coordinates()->axes[Y1].limits(start,stop);
	s+=QString::number(start)+"\t";
	s+=QString::number(stop)+"\t";
	sp->coordinates()->axes[Z1].limits(start,stop);
	s+=QString::number(start)+"\t";
	s+=QString::number(stop)+"\n";
    
	QString st;
	if (sp->coordinates()->style() == Qwt3D::NOCOORD)
		st="nocoord";
	else if (sp->coordinates()->style() == Qwt3D::BOX)
		st="box";
	else
		st="frame";
	QString style;
	style.setNum(style_);
	s+="Style\t"+style+"\t"+st+"\t";

	switch(sp->floorStyle ())
	{
		case NOFLOOR:
			st="nofloor";
			break;

		case FLOORISO:
			st="flooriso";
			break;

		case FLOORDATA:
			st="floordata";
			break;
	}
	s+=st+"\t";

	switch(sp->plotStyle())
	{
		case USER:
			if (pointStyle == VerticalBars)
				st="bars\t"+QString::number(barsRad);
			else if (pointStyle == Dots){
				st="points\t"+QString::number(d_point_size);
				st+="\t"+QString::number(d_smooth_points);
			} else if (pointStyle == Cones) {
				st="cones\t"+QString::number(conesRad);
				st+="\t"+QString::number(conesQuality);
			} else if (pointStyle == HairCross) {
				st="cross\t"+QString::number(crossHairRad);
				st+="\t"+QString::number(crossHairLineWidth);
				st+="\t"+QString::number(crossHairSmooth);
				st+="\t"+QString::number(crossHairBoxed);
			}
			break;

		case WIREFRAME:
			st="wireframe";
			break;

		case HIDDENLINE:
			st="hiddenline";
			break;

		case FILLED:
			st="filled";
			break;

		case FILLEDMESH:
			st="filledmesh";
			break;

		default:
			;
	}
	s+=st+"\n";

	s+="grids\t";
	s+=QString::number(sp->coordinates()->grids())+"\n";

	s+="title\t";
	s+=title+"\t";
	s+=titleCol.name()+"\t";
	s+=titleFnt.family()+"\t";
	s+=QString::number(titleFnt.pointSize())+"\t";
	s+=QString::number(titleFnt.weight())+"\t";
	s+=QString::number(titleFnt.italic())+"\n";

	s+="colors\t";
	s+=meshCol.name()+"\t";
	s+=axesCol.name()+"\t";
	s+=numCol.name()+"\t";
	s+=labelsCol.name()+"\t";
	s+=bgCol.name()+"\t";
	s+=gridCol.name()+"\t";
	s+=fromColor.name()+"\t";
	s+=toColor.name()+"\t";
	s+=QString::number(alpha) + "\t" + color_map + "\n";

	s+="axesLabels\t";
	s+=labels.join("\t")+"\n";

	s+="tics\t";
	QStringList tl=scaleTicks();
	s+=tl.join("\t")+"\n";

	s+="tickLengths\t";
	tl=axisTickLengths();
	s+=tl.join("\t")+"\n";

	s+="options\t";
	s+=QString::number(legendOn)+"\t";
	s+=QString::number(sp->resolution())+"\t";
	s+=QString::number(labelsDist)+"\n";

	s+="numbersFont\t";
	QFont fnt=sp->coordinates()->axes[X1].numberFont();
	s+=fnt.family()+"\t";
	s+=QString::number(fnt.pointSize())+"\t";
	s+=QString::number(fnt.weight())+"\t";
	s+=QString::number(fnt.italic())+"\n";

	s+="xAxisLabelFont\t";
	fnt=sp->coordinates()->axes[X1].labelFont();
	s+=fnt.family()+"\t";
	s+=QString::number(fnt.pointSize())+"\t";
	s+=QString::number(fnt.weight())+"\t";
	s+=QString::number(fnt.italic())+"\n";

	s+="yAxisLabelFont\t";
	fnt=sp->coordinates()->axes[Y1].labelFont();
	s+=fnt.family()+"\t";
	s+=QString::number(fnt.pointSize())+"\t";
	s+=QString::number(fnt.weight())+"\t";
	s+=QString::number(fnt.italic())+"\n";

	s+="zAxisLabelFont\t";
	fnt=sp->coordinates()->axes[Z1].labelFont();
	s+=fnt.family()+"\t";
	s+=QString::number(fnt.pointSize())+"\t";
	s+=QString::number(fnt.weight())+"\t";
	s+=QString::number(fnt.italic())+"\n";

	s+="rotation\t";
	s+=QString::number(sp->xRotation())+"\t";
	s+=QString::number(sp->yRotation())+"\t";
	s+=QString::number(sp->zRotation())+"\n";

	s+="zoom\t";
	s+=QString::number(sp->zoom())+"\n";

	s+="scaling\t";
	s+=QString::number(sp->xScale())+"\t";
	s+=QString::number(sp->yScale())+"\t";
	s+=QString::number(sp->zScale())+"\n";

	s+="shift\t";
	s+=QString::number(sp->xShift())+"\t";
	s+=QString::number(sp->yShift())+"\t";
	s+=QString::number(sp->zShift())+"\n";

	s+="LineWidth\t";
	s+=QString::number(sp->meshLineWidth())+"\n";
	s+="WindowLabel\t" + windowLabel() + "\t" + QString::number(captionPolicy()) + "\n";
	s+="Orthogonal\t" + QString::number(sp->ortho())+"\n";
	s+="</SurfacePlot>\n";
	return s;
}

QString Graph3D::saveAsTemplate(const QString& geometryInfo)
{
	QString s = saveToString(geometryInfo);
	QStringList lst = s.split("\n", QString::SkipEmptyParts);
	QStringList l = lst[3].split("\t");
	l[1] = QString();
	lst[3] = l.join("\t");
	return lst.join("\n");
}

void Graph3D::showColorLegend(bool show)
{
	if (legendOn == show)
		return;

	sp->makeCurrent();
	sp->showColorLegend(show);

	legendOn=show;
	sp->updateGL();
	emit modified();
}

void Graph3D::setResolution(int r)
{
	if (sp->resolution() == r)
		return;

	sp->makeCurrent();
	sp->setResolution(r);
	sp->updateData();
	sp->updateGL();
	emit modified();
}

void Graph3D::setTitle(const QStringList& lst)
{
	title=lst[1];
	sp->setTitle(title);

	titleCol=QColor(lst[2]);
	sp->setTitleColor(Qt2GL(titleCol));

	titleFnt=QFont(lst[3],lst[4].toInt(),lst[5].toInt(),lst[6].toInt());
	sp->setTitleFont(titleFnt.family(),titleFnt.pointSize(),titleFnt.weight(),titleFnt.italic());
}

void Graph3D::setTitle(const QString& s,const QColor& color,const QFont& font)
{
	if (title != s){
		title=s;
		sp->setTitle(title);
	}

	titleCol=color;
	sp->setTitleColor(Qt2GL(color));

	if (titleFnt != font){
		titleFnt=font;
		sp->setTitleFont(font.family(),font.pointSize(),font.weight(),font.italic());
	}
}

void Graph3D::setTitleFont(const QFont& font)
{
	if (titleFnt != font)
	{
		titleFnt=font;
		sp->setTitleFont(font.family(),font.pointSize(),font.weight(),font.italic());
	}
}

void Graph3D::setOptions(const QStringList& lst)
{
	legendOn=false;
	if (lst[1].toInt() == 1)
		legendOn=true;
	sp->showColorLegend(legendOn);
	sp->setResolution(lst[2].toInt());
	setLabelsDistance(lst[3].toInt());
}


void Graph3D::setOptions(bool legend, int r, int dist)
{
	sp->showColorLegend(legend);
	legendOn = legend;
	sp->setResolution(r);
	setLabelsDistance(dist);
}

QColor Graph3D::minDataColor()
{
	return fromColor;
}

QColor Graph3D::maxDataColor()
{
	return toColor;
}

void Graph3D::setDataColors(const QColor& cMin, const QColor& cMax)
{
	if (cMin == fromColor && cMax == toColor)
		return;

	fromColor=cMin;
	toColor=cMax;

	Qwt3D::ColorVector cv;

	int size=255;
	double dsize = size;

	double r1=cMax.red()/dsize;
	double r2=cMin.red()/dsize;

	double stepR = (r1-r2)/dsize;

	double g1=cMax.green()/dsize;
	double g2=cMin.green()/dsize;

	double stepG = (g1-g2)/dsize;

	double b1=cMax.blue()/dsize;
	double b2=cMin.blue()/dsize;

	double stepB = (b1-b2)/dsize;

	RGBA rgb;
	for (int i=0; i<size; i++) {
		rgb.r = r1-i*stepR;
		rgb.g = g1-i*stepG;
		rgb.b = b1-i*stepB;
		rgb.a = alpha;

		cv.push_back(rgb);
	}

	col_ = new StandardColor(sp);
	col_->setColorVector(cv);
	sp->setDataColor(col_);

	if (legendOn) {
		sp->showColorLegend(false);
		sp->showColorLegend(legendOn);
	}
}

void Graph3D::changeTransparency(double t)
{
	if (alpha == t)
		return;

	alpha = t;

	Qwt3D::StandardColor* color=(StandardColor*) sp->dataColor ();
	color->setAlpha(t);

    sp->showColorLegend(legendOn);
	sp->updateData();
	sp->updateGL();
	emit modified();
}

void Graph3D::setTransparency(double t)
{
	if (alpha == t)
		return;

	alpha = t;

	Qwt3D::StandardColor* color=(StandardColor*) sp->dataColor ();
	color->setAlpha(t);
}

void Graph3D::showWorksheet()
{
	if (d_table)
		d_table->showMaximized();
	else if (d_matrix)
		d_matrix->showMaximized();
}

void Graph3D::setAntialiasing(bool smooth)
{
    sp->makeCurrent();
	sp->setSmoothMesh(smooth);
	sp->coordinates()->setLineSmooth(smooth);
	sp->updateData();
	sp->updateGL();
}

/**
Turns 3D animation on or off
*/
void Graph3D::animate(bool on)
{
if ( on )
    d_timer->start( animation_redraw_wait ); // Wait this many msecs before redraw
else
    d_timer->stop();
}

void Graph3D::rotate()
{
if (!sp)
   return;

sp->setRotation(int(sp->xRotation() + 1) % 360, int(sp->yRotation() + 1) % 360, int(sp->zRotation() + 1) % 360);
}

void Graph3D::setDataColorMap(const QString& fileName)
{
if (color_map == fileName)
   return;

ColorVector cv;
if (!openColorMap(cv, fileName))
   return;

color_map = fileName;

col_ = new StandardColor(sp);
col_->setColorVector(cv);

sp->setDataColor(col_);
sp->updateData();
sp->showColorLegend(legendOn);
sp->updateGL();
}

bool Graph3D::openColorMap(ColorVector& cv, QString fname)
{
if (fname.isEmpty())
   return false;

using std::ifstream;
ifstream file(QWT3DLOCAL8BIT(fname));
if (!file)
   return false;

RGBA rgb;
cv.clear();

while ( file ) {
      file >> rgb.r >> rgb.g >> rgb.b;
      file.ignore(10000,'\n');
      if (!file.good())
         break;
      else {
          rgb.a = 1;
          rgb.r /= 255;
          rgb.g /= 255;
          rgb.b /= 255;
          cv.push_back(rgb);
          }
      }
return true;
}

void Graph3D::findBestLayout()
{
  	double start, end;
  	sp->coordinates()->axes[X1].limits (start, end);
  	double xScale = 1/fabs(end-start);

  	sp->coordinates()->axes[Y1].limits (start, end);
  	double yScale = 1/fabs(end-start);

  	sp->coordinates()->axes[Z1].limits (start, end);
  	double zScale = 1/fabs(end-start);

  	double d = (sp->hull().maxVertex-sp->hull().minVertex).length();
  	sp->setScale(xScale, yScale, zScale);
  	sp->setZoom(d/sqrt(3.0));

  	double majl = 0.1/yScale;
  	setAxisTickLength(0, majl, 0.6*majl);
  	majl = 0.1/xScale;
  	setAxisTickLength(1, majl, 0.6*majl);
  	setAxisTickLength(2, majl, 0.6*majl);
}

void Graph3D::copy(Graph3D* g)
{
	if (!g)
        return;

	pointStyle = g->pointType();
	style_ = g->plotStyle();
	if (g->plotStyle() == Qwt3D::USER ){
		switch (pointStyle){
			case None :
				sp->setPlotStyle(g->plotStyle());
			break;

			case Dots :
				d_point_size = g->pointsSize();
				d_smooth_points = g->smoothPoints();
				sp->setPlotStyle(Dot(d_point_size, d_smooth_points));
			break;

			case VerticalBars :
				setBarRadius(g->barsRadius());
				sp->setPlotStyle(Bar(barsRad));
				break;

			case HairCross :
				setCrossOptions(g->crossHairRadius(), g->crossHairLinewidth(), g->smoothCrossHair(), g->boxedCrossHair());
				sp->setPlotStyle(CrossHair(crossHairRad, crossHairLineWidth, crossHairSmooth, crossHairBoxed));
				break;

			case Cones :
				setConeOptions(g->coneRadius(), g->coneQuality());
				sp->setPlotStyle(Cone3D(conesRad, conesQuality));
				break;
		}
	} else
		customPlotStyle(style_);

	sp->setCoordinateStyle(g->coordStyle());
	sp->setFloorStyle(g->floorStyle());

	setGrid(g->grids());
	setTitle(g->plotTitle(),g->titleColor(),g->titleFont());
	setTransparency(g->transparency());
	if (!g->colorMap().isEmpty())
		setDataColorMap(g->colorMap());
	else
		setDataColors(g->minDataColor(),g->maxDataColor());

	setMeshColor(g->meshColor());
	setAxesColor(g->axesColor());
	setNumbersColor(g->numColor());
	setLabelsColor(g->labelColor());
	setBackgroundColor(g->bgColor());
	setGridColor(g->gridColor());

	setAxesLabels(g->axesLabels());
	setTicks(g->scaleTicks());
	setTickLengths(g->axisTickLengths());
	setOptions(g->isLegendOn(), g->resolution(), g->labelsDistance());
	setNumbersFont(g->numbersFont());
	setXAxisLabelFont(g->xAxisLabelFont());
	setYAxisLabelFont(g->yAxisLabelFont());
	setZAxisLabelFont(g->zAxisLabelFont());
	setRotation(g->xRotation(),g->yRotation(),g->zRotation());
	setZoom(g->zoom());
	setScale(g->xScale(),g->yScale(),g->zScale());
	setShift(g->xShift(),g->yShift(),g->zShift());
	setMeshLineWidth(g->meshLineWidth());

	bool smooth = g->antialiasing();
    sp->setSmoothMesh(smooth);
	sp->coordinates()->setLineSmooth(smooth);

	setOrthogonal(g->isOrthogonal());

	sp->updateData();
	sp->updateGL();
	animate(g->isAnimated());
}

Graph3D::~Graph3D()
{
	if (d_surface)
		delete d_surface;
	if (d_func)
		delete d_func;

	delete sp;
}
