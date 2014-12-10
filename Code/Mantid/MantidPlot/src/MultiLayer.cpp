/***************************************************************************
    File                 : MultiLayer.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief, Tilman Hoener zu Siederdissen
    Email (use @ for *)  : ion_vasilief*yahoo.fr, thzs*gmx.net
    Description          : Multi layer widget

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
#include <QVector>
#include <QWidgetList>
#include <QPrinter>
#include <QPrintDialog>
#include <QApplication>
#include <QMessageBox>
#include <QBitmap>
#include <QImageWriter>
#include <QPainter>
#include <QPicture>
#include <QClipboard>
#include <QCheckBox>
#include <QGroupBox>
#include <QSpinBox>
#include <QSize>

#include <set>

#if QT_VERSION >= 0x040300
	#include <QSvgGenerator>
#endif

#include <qwt_plot.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_layout.h>
#include <qwt_scale_widget.h>
#include <qwt_text_label.h>

#include "MultiLayer.h"
#include "Plot.h"
#include "LegendWidget.h"
#include "SelectionMoveResizer.h"
#include "ApplicationWindow.h"
#include <ColorButton.h>

#include "Mantid/MantidDock.h"
#include "Mantid/MantidMatrixCurve.h"
#include "Mantid/MantidMDCurve.h"

#include <gsl/gsl_vector.h>
#include "Mantid/MantidMDCurveDialog.h"
#include "MantidQtSliceViewer/LinePlotOptions.h"

#include "TSVSerialiser.h"

namespace
{
  /// static logger
  Mantid::Kernel::Logger g_log("MultiLayer");
}


LayerButton::LayerButton(const QString& text, QWidget* parent)
: QPushButton(text, parent)
{
	int btn_size = 20;

	setToggleButton(true);
	setOn(true);
	setMaximumWidth(btn_size);
	setMaximumHeight(btn_size);
}

void LayerButton::mousePressEvent( QMouseEvent *event )
{
	if (event->button() == Qt::LeftButton && !isOn())
		emit clicked(this);
}

void LayerButton::mouseDoubleClickEvent ( QMouseEvent * )
{
	emit showCurvesDialog();
}

MultiLayer::MultiLayer(ApplicationWindow* parent, int layers, int rows, int cols, 
                       const QString& label, const char* name, Qt::WFlags f)
                         : MdiSubWindow(parent, label, name, f),
                         active_graph(NULL),
                         d_cols(cols),
                         d_rows(rows),
                         graph_width(500),
                         graph_height(400),
                         colsSpace(5),
                         rowsSpace(5),
                         left_margin(5),
                         right_margin(5),
                         top_margin(5),
                         bottom_margin(5),
                         l_canvas_width(400),
                         l_canvas_height(300),
                         hor_align(HCenter),
                         vert_align(VCenter),
                         d_scale_on_print(true),
                         d_print_cropmarks(false),
                         d_is_waterfall_plot(false),
                         d_waterfall_fill_color(/*Invalid color*/)
{
	layerButtonsBox = new QHBoxLayout();
	waterfallBox = new QHBoxLayout();
	buttonsLine = new QHBoxLayout();
	buttonsLine->addLayout(layerButtonsBox);
	buttonsLine->addStretch();
	buttonsLine->addLayout(waterfallBox);

	canvas = new QWidget();

	QWidget *mainWidget = new QWidget();
	mainWidget->setAutoFillBackground(true);
	mainWidget->setBackgroundRole(QPalette::Window);

	//setAutoFillBackground(true);
	//setBackgroundRole(QPalette::Window);

	QVBoxLayout* layout = new QVBoxLayout(mainWidget);
	//QVBoxLayout* layout = new QVBoxLayout(this);

	layout->addLayout(buttonsLine);
	layout->addWidget(canvas, 1);
	layout->setMargin(0);
	layout->setSpacing(0);
	setWidget(mainWidget);

  int canvas_width = graph_width + left_margin + right_margin;
  int canvas_height = graph_height + top_margin + bottom_margin;
  setGeometry(QRect(0, 0, canvas_width, canvas_height + LayerButton::btnSize()));

  canvas->resize(canvas_width, canvas_height);
	canvas->installEventFilter(this);

	QPalette pal = palette();
	pal.setColor(QPalette::Window, QColor(Qt::white));
	setPalette(pal);

	for (int i = 0; i < layers; i++)
		addLayer();

	//setFocusPolicy(Qt::StrongFocus);
	//setFocus();

  setAcceptDrops(true);
}

MultiLayer::~MultiLayer()
{
}

QSize MultiLayer::minimumSizeHint() const
{
  return QSize(200, 200);
}

Graph *MultiLayer::layer(int num)
{
    int index = num - 1;
    if (index < 0 || index >= graphsList.count())
        return 0;

    return static_cast<Graph*>(graphsList.at(index));
}

void MultiLayer::insertCurve(MultiLayer* ml, int i)
{
  if( ml== this ) return;
  Graph *current = activeGraph();
  if( !current ) return;
  
  current->insertCurve(ml->activeGraph(), i);
  current->updatePlot();
}

LayerButton* MultiLayer::addLayerButton()
{
	foreach(LayerButton *btn, buttonsList)
		btn->setOn(false);

	LayerButton *button = new LayerButton(QString::number(graphsList.size() + 1));
	connect (button, SIGNAL(clicked(LayerButton*)), this, SLOT(activateGraph(LayerButton*)));
	connect (button, SIGNAL(showCurvesDialog()), this, SIGNAL(showCurvesDialog()));

	buttonsList.append(button);
    layerButtonsBox->addWidget(button);
	return button;
}

Graph* MultiLayer::addLayer(int x, int y, int width, int height)
{
	addLayerButton();
	if (!width && !height){		
		width =	canvas->width() - left_margin - right_margin - (d_cols - 1)*colsSpace; 
		height = canvas->height() - top_margin - left_margin - (d_rows - 1)*rowsSpace;
		
		int layers = graphsList.size();
		x = left_margin + (layers % d_cols)*(width + colsSpace);
	    y = top_margin + (layers / d_cols)*(height + rowsSpace);
	}

	Graph* g = new Graph(x, y, width, height, canvas);
    g->show();
	graphsList.append(g);

	active_graph = g;
	connectLayer(g);
	return g;
}

void MultiLayer::adjustSize()
{
    canvas->resize(size().width(), size().height() - LayerButton::btnSize());
}

void MultiLayer::activateGraph(LayerButton* button)
{
	for (int i=0;i<buttonsList.count();i++)
	{
        LayerButton *btn=static_cast<LayerButton*>(buttonsList.at(i));
		if (btn->isOn())
			btn->setOn(false);

		if (btn == button)
		{
            active_graph = static_cast<Graph*>(graphsList.at(i));
			//active_graph->setFocus();
			active_graph->raise();//raise layer on top of the layers stack
			button->setOn(true);
      if( d_layers_selector )
      {
        removeLayerSelectionFrame();
      }
		}
	}
}

void MultiLayer::setActiveGraph(Graph* g)
{
	if (!g || active_graph == g)
		return;

  // Make sure the passed in Graph belongs to this object
  bool found = false;
  foreach(Graph *gr, graphsList)
  {
    if (gr == g) found = true;
  }
  if (!found) return;

	active_graph = g;
	//active_graph->setFocus();

	if (d_layers_selector)
  {
    removeLayerSelectionFrame();
  }
	active_graph->raise();//raise layer on top of the layers stack

	for(int i=0; i<graphsList.count(); i++){
        Graph *gr = static_cast<Graph *>(graphsList.at(i));
		gr->deselect();

        LayerButton *btn = static_cast<LayerButton *>(buttonsList.at(i));
		if (gr == g)
			btn->setOn(true);
		else
			btn->setOn(false);
	}
}

void MultiLayer::resizeLayers (QResizeEvent *re)
{
	if (applicationWindow()->d_opening_file)
		return;

	QSize oldSize = re->oldSize();
	QSize size = re->size();

	bool scaleLayerFonts = false;
	if(!oldSize.isValid()){// The old size is invalid when maximizing a window (why?)
        oldSize = QSize(canvas->childrenRect().width() + left_margin + right_margin,
                        canvas->childrenRect().height() + top_margin + bottom_margin);
		scaleLayerFonts = true;
	}

	QApplication::setOverrideCursor(Qt::waitCursor);

	double w_ratio = (double)size.width()/(double)oldSize.width();
	double h_ratio = (double)(size.height())/(double)(oldSize.height());

	foreach (Graph *g, graphsList){
		if (!g->ignoresResizeEvents()){
			QObjectList lst = g->plotWidget()->children();
			foreach(QObject *o, lst){
				if (o->isA("LegendWidget"))
                    static_cast<LegendWidget *>(o)->setFixedCoordinatesMode();
			}

			int gx = qRound(g->x()*w_ratio);
			int gy = qRound(g->y()*h_ratio);
			int gw = qRound(g->width()*w_ratio);
			int gh = qRound(g->height()*h_ratio);
			g->setGeometry(QRect(gx, gy, gw, gh));
			g->plotWidget()->resize(QSize(gw, gh));

            if (scaleLayerFonts && g->autoscaleFonts())
                g->scaleFonts(h_ratio);
		}
	}

	if (d_is_waterfall_plot)
		updateWaterfalls();

	//emit modifiedPlot();
	//repaint();
	QApplication::restoreOverrideCursor();
}

void MultiLayer::confirmRemoveLayer()
{
	if (graphsList.size() > 1){
		switch(QMessageBox::information(this,
					tr("MantidPlot - Guess best layout?"),
					tr("Do you want MantidPlot to rearrange the remaining layers?"),
					tr("&Yes"), tr("&No"), tr("&Cancel"),
					0, 2) )
		{
			case 0:
				removeLayer();
				arrangeLayers(true, false);
				break;

			case 1:
				removeLayer();
				break;

			case 2:
				return;
				break;
		}
	} else
		removeLayer();
}

void MultiLayer::removeLayer()
{
	//remove corresponding button
	foreach(LayerButton* btn, buttonsList){
		if (btn->isOn()){
			buttonsList.removeAll(btn);
			btn->close(true);
			break;
		}
	}

	int i = 0;
	foreach(LayerButton* btn, buttonsList)//update the texts of the buttons
		btn->setText(QString::number(i + 1));

	if (active_graph->zoomOn() || active_graph->activeTool())
		emit setPointerCursor();

	int index = graphsList.indexOf(active_graph);
	graphsList.removeAt(index);
	active_graph->setAttribute(Qt::WA_DeleteOnClose, false);
	active_graph->close();
	delete active_graph;
	if(index >= graphsList.count())
		index--;

	if (graphsList.count() == 0){
		active_graph = 0;
		return;
	}

    active_graph=static_cast<Graph*>( graphsList.at(index));

	for (i=0;i<(int)graphsList.count();i++){
        Graph *gr=static_cast<Graph *>(graphsList.at(i));
		if (gr == active_graph){
            LayerButton *button = static_cast<LayerButton *>(buttonsList.at(i));
			button->setOn(TRUE);
			break;
		}
	}

	emit modifiedPlot();
}

void MultiLayer::setGraphGeometry(int x, int y, int w, int h)
{
	if (active_graph->pos() == QPoint (x,y) &&
		active_graph->size() == QSize (w,h))
		return;

	active_graph->setGeometry(QRect(QPoint(x,y),QSize(w,h)));
    active_graph->plotWidget()->resize(QSize(w, h));
	emit modifiedPlot();
}

QSize MultiLayer::arrangeLayers(bool userSize)
{
	int layers = graphsList.size();
	const QRect rect = canvas->geometry();

	gsl_vector *xTopR = gsl_vector_calloc (layers);//ratio between top axis + title and canvas height
	gsl_vector *xBottomR = gsl_vector_calloc (layers); //ratio between bottom axis and canvas height
	gsl_vector *yLeftR = gsl_vector_calloc (layers);
	gsl_vector *yRightR = gsl_vector_calloc (layers);
	gsl_vector *maxXTopHeight = gsl_vector_calloc (d_rows);//maximum top axis + title height in a row
	gsl_vector *maxXBottomHeight = gsl_vector_calloc (d_rows);//maximum bottom axis height in a row
	gsl_vector *maxYLeftWidth = gsl_vector_calloc (d_cols);//maximum left axis width in a column
	gsl_vector *maxYRightWidth = gsl_vector_calloc (d_cols);//maximum right axis width in a column
	gsl_vector *Y = gsl_vector_calloc (d_rows);
	gsl_vector *X = gsl_vector_calloc (d_cols);

	for (int i=0; i<layers; i++)
	{//calculate scales/canvas dimensions reports for each layer and stores them in the above vectors
        Graph *gr = static_cast<Graph *>(graphsList.at(i));
		QwtPlot *plot = gr->plotWidget();
		QwtPlotLayout *plotLayout=plot->plotLayout();
		QRect cRect=plotLayout->canvasRect();
		double ch = (double) cRect.height();
		double cw = (double) cRect.width();

		QRect tRect=plotLayout->titleRect ();
		QwtScaleWidget *scale=(QwtScaleWidget *) plot->axisWidget (QwtPlot::xTop);

		int topHeight = 0;
		if (!tRect.isNull())
			topHeight += tRect.height() + plotLayout->spacing();
		if (scale){
			QRect sRect=plotLayout->scaleRect (QwtPlot::xTop);
			topHeight += sRect.height();
		}
		gsl_vector_set (xTopR, i, double(topHeight)/ch);

		scale=(QwtScaleWidget *) plot->axisWidget (QwtPlot::xBottom);
		if (scale){
			QRect sRect = plotLayout->scaleRect (QwtPlot::xBottom);
			gsl_vector_set (xBottomR, i, double(sRect.height())/ch);
		}

		scale=(QwtScaleWidget *) plot->axisWidget (QwtPlot::yLeft);
		if (scale){
			QRect sRect = plotLayout->scaleRect (QwtPlot::yLeft);
			gsl_vector_set (yLeftR, i, double(sRect.width())/cw);
		}

		scale=(QwtScaleWidget *) plot->axisWidget (QwtPlot::yRight);
		if (scale){
			QRect sRect = plotLayout->scaleRect (QwtPlot::yRight);
			gsl_vector_set (yRightR, i, double(sRect.width())/cw);
		}

		//calculate max scales/canvas dimensions ratio for each line and column and stores them to vectors
		int row = i / d_cols;
		if (row >= d_rows )
			row = d_rows - 1;

		int col = i % d_cols;

		double aux = gsl_vector_get(xTopR, i);
		double old_max = gsl_vector_get(maxXTopHeight, row);
		if (aux >= old_max)
			gsl_vector_set(maxXTopHeight, row,  aux);

		aux = gsl_vector_get(xBottomR, i) ;
		if (aux >= gsl_vector_get(maxXBottomHeight, row))
			gsl_vector_set(maxXBottomHeight, row,  aux);

		aux = gsl_vector_get(yLeftR, i);
		if (aux >= gsl_vector_get(maxYLeftWidth, col))
			gsl_vector_set(maxYLeftWidth, col, aux);

		aux = gsl_vector_get(yRightR, i);
		if (aux >= gsl_vector_get(maxYRightWidth, col))
			gsl_vector_set(maxYRightWidth, col, aux);
	}

	double c_heights = 0.0;
	for (int i=0; i<d_rows; i++){
		gsl_vector_set (Y, i, c_heights);
		c_heights += 1 + gsl_vector_get(maxXTopHeight, i) + gsl_vector_get(maxXBottomHeight, i);
	}

	double c_widths = 0.0;
	for (int i=0; i<d_cols; i++){
		gsl_vector_set (X, i, c_widths);
		c_widths += 1 + gsl_vector_get(maxYLeftWidth, i) + gsl_vector_get(maxYRightWidth, i);
	}

	if (!userSize){
		l_canvas_width = static_cast<int>((rect.width()-(d_cols-1)*colsSpace - right_margin - left_margin)/c_widths);
		l_canvas_height = static_cast<int>((rect.height()-(d_rows-1)*rowsSpace - top_margin - bottom_margin)/c_heights);
	}

	QSize size = QSize(l_canvas_width, l_canvas_height);

	for (int i=0; i<layers; i++){
		int row = i / d_cols;
		if (row >= d_rows )
			row = d_rows - 1;

		int col = i % d_cols;

		//calculate sizes and positions for layers
		const int w = int (l_canvas_width*(1 + gsl_vector_get(yLeftR, i) + gsl_vector_get(yRightR, i)));
		const int h = int (l_canvas_height*(1 + gsl_vector_get(xTopR, i) + gsl_vector_get(xBottomR, i)));

		int x = left_margin + col*colsSpace;
		if (hor_align == HCenter)
			x += int (l_canvas_width*(gsl_vector_get(X, col) + gsl_vector_get(maxYLeftWidth, col) - gsl_vector_get(yLeftR, i)));
		else if (hor_align == Left)
			x += static_cast<int>(l_canvas_width*gsl_vector_get(X, col));
		else if (hor_align == Right)
			x += static_cast<int>(l_canvas_width*(gsl_vector_get(X, col) + gsl_vector_get(maxYLeftWidth, col) - gsl_vector_get(yLeftR, i)+
						gsl_vector_get(maxYRightWidth, col) - gsl_vector_get(yRightR, i)));

		int y = top_margin + row*rowsSpace;
		if (vert_align == VCenter)
			y += static_cast<int>(l_canvas_height*(gsl_vector_get(Y, row) + gsl_vector_get(maxXTopHeight, row) - gsl_vector_get(xTopR, i)));
		else if (vert_align == Top)
			y += static_cast<int>(l_canvas_height*gsl_vector_get(Y, row));
		else if (vert_align == Bottom)
			y += static_cast<int>(l_canvas_height*(gsl_vector_get(Y, row) + gsl_vector_get(maxXTopHeight, row) - gsl_vector_get(xTopR, i)+
						+ gsl_vector_get(maxXBottomHeight, row) - gsl_vector_get(xBottomR, i)));

		//resizes and moves layers
        Graph *gr = static_cast<Graph *>(graphsList.at(i));
		bool autoscaleFonts = false;
		if (!userSize){//When the user specifies the layer canvas size, the window is resized
			//and the fonts must be scaled accordingly. If the size is calculated
			//automatically we don't rescale the fonts in order to prevent problems
			//with too small fonts when the user adds new layers or when removing layers

			autoscaleFonts = gr->autoscaleFonts();//save user settings
			gr->setAutoscaleFonts(false);
		}

		gr->setGeometry(QRect(x, y, w, h));
		gr->plotWidget()->resize(QSize(w, h));

		if (!userSize)
			gr->setAutoscaleFonts(autoscaleFonts);//restore user settings
	}

	//free memory
	gsl_vector_free (maxXTopHeight); gsl_vector_free (maxXBottomHeight);
	gsl_vector_free (maxYLeftWidth); gsl_vector_free (maxYRightWidth);
	gsl_vector_free (xTopR); gsl_vector_free (xBottomR);
	gsl_vector_free (yLeftR); gsl_vector_free (yRightR);
	gsl_vector_free (X); gsl_vector_free (Y);
	return size;
}

void MultiLayer::findBestLayout(int &d_rows, int &d_cols)
{
	int NumGraph = graphsList.size();
	if(NumGraph%2==0) // NumGraph is an even number
	{
		if(NumGraph<=2)
			d_cols=NumGraph/2+1;
		else if(NumGraph>2)
			d_cols=NumGraph/2;

		if(NumGraph<8)
			d_rows=NumGraph/4+1;
		if(NumGraph>=8)
			d_rows=NumGraph/4;
	}
	else if(NumGraph%2!=0) // NumGraph is an odd number
	{
		int Num=NumGraph+1;

		if(Num<=2)
			d_cols=1;
		else if(Num>2)
			d_cols=Num/2;

		if(Num<8)
			d_rows=Num/4+1;
		if(Num>=8)
			d_rows=Num/4;
	}
}

void MultiLayer::arrangeLayers(bool fit, bool userSize)
{
	if (graphsList.size() == 0)
		return;

	QApplication::setOverrideCursor(Qt::waitCursor);

	if(d_layers_selector)
  {
    removeLayerSelectionFrame();
  }

	if (fit)
		findBestLayout(d_rows, d_cols);

	//the canvas sizes of all layers become equal only after several
	//resize iterations, due to the way Qwt handles the plot layout
	int iterations = 0;
	QSize size = arrangeLayers(userSize);
	QSize canvas_size = QSize(1,1);
	while (canvas_size != size && iterations < 10){
		iterations++;
		canvas_size = size;
		size = arrangeLayers(userSize);
	}

	if (userSize){//resize window
		bool ignoreResize = active_graph->ignoresResizeEvents();
		foreach (Graph *gr, graphsList)
			gr->setIgnoreResizeEvents(true);

		this->showNormal();
		QSize size = canvas->childrenRect().size();
		this->resize(canvas->x() + size.width() + left_margin + 2*right_margin, 
					canvas->y() + size.height() + bottom_margin + 2*LayerButton::btnSize());
		
		foreach (Graph *gr, graphsList)
			gr->setIgnoreResizeEvents(ignoreResize);
	}

	emit modifiedPlot();
	QApplication::restoreOverrideCursor();
}

void MultiLayer::setCols(int c)
{
	if (d_cols != c)
		d_cols = c;
}

void MultiLayer::setRows(int r)
{
	if (d_rows != r)
		d_rows = r;
}

QPixmap MultiLayer::canvasPixmap()
{
    QPixmap pic(canvas->size());
    pic.fill();
    QPainter p(&pic);
	foreach (Graph *g, graphsList){
        Plot *plot = static_cast<Plot *>(g->plotWidget());
		plot->print(&p, QRect(g->pos(), plot->size()));
	}
	p.end();
	return pic;
}

void MultiLayer::exportToFile(const QString& fileName)
{
	if ( fileName.isEmpty() ){
		QMessageBox::critical(0, tr("MantidPlot - Error"), tr("Please provide a valid file name!"));
        return;
	}

	if (fileName.contains(".eps") || fileName.contains(".pdf") || fileName.contains(".ps")){
		exportVector(fileName);
		return;
	} else if(fileName.contains(".svg")){
		exportSVG(fileName);
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

void MultiLayer::exportImage(const QString& fileName, int quality, bool transparent)
{
	QPixmap pic = canvasPixmap();
	if (transparent)
	{
		QBitmap mask(pic.size());
		mask.fill(Qt::color1);
		QPainter p;
		p.begin(&mask);
		p.setPen(Qt::color0);

		QColor background = QColor (Qt::white);
		QRgb backgroundPixel = background.rgb ();

		QImage image = pic.convertToImage();
		for (int y=0; y<image.height(); y++)
		{
			for ( int x=0; x<image.width(); x++ )
			{
				QRgb rgb = image.pixel(x, y);
				if (rgb == backgroundPixel) // we want the frame transparent
					p.drawPoint( x, y );
			}
		}
		p.end();
		pic.setMask(mask);
	}
	pic.save(fileName, 0, quality);
}

void MultiLayer::exportPDF(const QString& fname)
{
	exportVector(fname);
}

void MultiLayer::exportVector(const QString& fileName, int res, bool color, bool keepAspect, QPrinter::PageSize pageSize)
{
  (void) res; //avoid compiler warning

	if ( fileName.isEmpty() ){
		QMessageBox::critical(this, tr("MantidPlot - Error"),
		tr("Please provide a valid file name!"));
        return;
	}

	QPrinter printer;
    printer.setDocName (objectName());
    printer.setCreator("MantidPlot");
	printer.setFullPage(true);
	printer.setOutputFileName(fileName);
    if (fileName.contains(".eps"))
    	printer.setOutputFormat(QPrinter::PostScriptFormat);

	//if (res) //only printing with screen resolution works correctly for the moment
		//printer.setResolution(res);

	QRect canvasRect = canvas->rect();
    if (pageSize == QPrinter::Custom)
        printer.setPageSize(Graph::minPageSize(printer, canvasRect));
    else
        printer.setPageSize(pageSize);

	double canvas_aspect = double(canvasRect.width())/double(canvasRect.height());
	if (canvas_aspect < 1)
		printer.setOrientation(QPrinter::Portrait);
	else
		printer.setOrientation(QPrinter::Landscape);

	if (color)
		printer.setColorMode(QPrinter::Color);
	else
		printer.setColorMode(QPrinter::GrayScale);

    double x_margin = 0, y_margin = 0, width = 0, height = 0;
    if (keepAspect){// export should preserve plot aspect ratio
        double page_aspect = double(printer.width())/double(printer.height());
        if (page_aspect > canvas_aspect){
            y_margin = (0.1/2.54)*printer.logicalDpiY(); // 1 mm margins
            height = printer.height() - 2*y_margin;
            width = height*canvas_aspect;
            x_margin = 0.5*(printer.width() - width);
        } else {
            x_margin = (0.1/2.54)*printer.logicalDpiX(); // 1 mm margins
            width = printer.width() - 2*x_margin;
            height = width/canvas_aspect;
            y_margin = 0.5*(printer.height() - height);
        }
	} else {
	    x_margin = (0.1/2.54)*printer.logicalDpiX(); // 1 mm margins
        y_margin = (0.1/2.54)*printer.logicalDpiY(); // 1 mm margins
        width = printer.width() - 2*x_margin;
        height = printer.height() - 2*y_margin;
	}

    double scaleFactorX = width/(double)canvasRect.width();
    double scaleFactorY = height/(double)canvasRect.height();

    QPainter paint(&printer);
	foreach (Graph *g, graphsList){
        Plot *plot = static_cast<Plot *>(g->plotWidget());

        QPoint pos = g->pos();
        pos = QPoint(qRound(x_margin + pos.x()*scaleFactorX), qRound(y_margin + pos.y()*scaleFactorY));

        int layer_width = qRound(plot->frameGeometry().width()*scaleFactorX);
        int layer_height = qRound(plot->frameGeometry().height()*scaleFactorY);

        plot->print(&paint, QRect(pos, QSize(layer_width, layer_height)));
    }
}

void MultiLayer::exportSVG(const QString& fname)
{
	QSvgGenerator generator;
	generator.setFileName(fname);
	generator.setSize(canvas->size());

	QPainter p(&generator);
	foreach (Graph *g, graphsList){
        Plot *plot = static_cast<Plot *>(g->plotWidget());
		plot->print(&p, QRect(g->pos(), plot->size()));
	}
	p.end();
}

void MultiLayer::copyAllLayers()
{
	bool selectionOn = false;
	if (d_layers_selector){
		d_layers_selector->hide();
		selectionOn = true;
	}

	foreach (QWidget* g, graphsList)
        static_cast<Graph *>(g)->deselectMarker();

	QPixmap pic = canvasPixmap();
	QImage image = pic.convertToImage();
	QApplication::clipboard()->setImage(image);

	if (selectionOn)
		d_layers_selector->show();
}

void MultiLayer::printActiveLayer()
{
	if (active_graph){
		active_graph->setScaleOnPrint(d_scale_on_print);
		active_graph->printCropmarks(d_print_cropmarks);
		active_graph->print();
	}
}

void MultiLayer::print()
{
	QPrinter printer;
	printer.setColorMode (QPrinter::Color);
	printer.setFullPage(true);
    QRect canvasRect = canvas->rect();
    double aspect = double(canvasRect.width())/double(canvasRect.height());
    if (aspect < 1)
        printer.setOrientation(QPrinter::Portrait);
    else
        printer.setOrientation(QPrinter::Landscape);

    QPrintDialog printDialog(&printer);
    if (printDialog.exec() == QDialog::Accepted)
	{
		QPainter paint(&printer);
		printAllLayers(&paint);
		paint.end();
	}
}

void MultiLayer::printAllLayers(QPainter *painter)
{
	if (!painter)
		return;

	QPrinter *printer = (QPrinter *)painter->device();
	QRect paperRect = ((QPrinter *)painter->device())->paperRect();
	QRect canvasRect = canvas->rect();
	QRect pageRect = printer->pageRect();
	QRect cr = canvasRect; // cropmarks rectangle
	Qt::WindowStates qtstates=windowState();
	if(qtstates==(Qt::WindowMaximized | Qt::WindowActive))
	   d_scale_on_print=true;
	else if (qtstates==Qt::WindowActive)
		d_scale_on_print=false;
	if (d_scale_on_print)
	{
		int margin = (int)((1/2.54)*printer->logicalDpiY()); // 1 cm margins
		double scaleFactorX=(double)(paperRect.width()-2*margin)/(double)canvasRect.width();
		double scaleFactorY=(double)(paperRect.height()-2*margin)/(double)canvasRect.height();
	     if (d_print_cropmarks)
        {
			cr.moveTo(QPoint(margin + static_cast<int>(cr.x()*scaleFactorX),
							 margin + static_cast<int>(cr.y()*scaleFactorY)));
			cr.setWidth(static_cast<int>(cr.width()*scaleFactorX));
			cr.setHeight(static_cast<int>(cr.height()*scaleFactorX));
        }

		for (int i=0; i<(int)graphsList.count(); i++)
		{
            Graph *gr=static_cast<Graph *>(graphsList.at(i));
			Plot *myPlot= gr->plotWidget();
			QPoint pos=gr->pos();
			pos=QPoint(margin + static_cast<int>(pos.x()*scaleFactorX), margin + static_cast<int>(pos.y()*scaleFactorY));
			int width=static_cast<int>(myPlot->frameGeometry().width()*scaleFactorX);
			int height=static_cast<int>(myPlot->frameGeometry().height()*scaleFactorY);
			myPlot->print(painter, QRect(pos, QSize(width,height)));
		}
	}
	else
	{	
	   	int x_margin = (pageRect.width() - canvasRect.width())/2;
	   	int y_margin = (pageRect.height() - canvasRect.height())/2;
		if (d_print_cropmarks)
            cr.moveTo(x_margin, y_margin);
		int margin = (int)((1/2.54)*printer->logicalDpiY()); // 1 cm margins
		double scaleFactorX=(double)(paperRect.width()-4*margin)/(double)canvasRect.width();
		double scaleFactorY=(double)(paperRect.height()-4*margin)/(double)canvasRect.height();
		
		for (int i=0; i<(int)graphsList.count(); i++)
		{
            Graph *gr = static_cast<Graph *>(graphsList.at(i));
            Plot *myPlot = static_cast<Plot *>(gr->plotWidget());

			QPoint pos = gr->pos();
			pos = QPoint(margin + pos.x(), margin + pos.y());
			QSize size=	myPlot->size();
			int width=int(size.width()*scaleFactorX);
			int height=int(size.height()*scaleFactorY);
			myPlot->print(painter, QRect(pos, QSize(width,height)));
		
		}
	}
	if (d_print_cropmarks)
    {
		cr.addCoords(-1, -1, 2, 2);
    	painter->save();
		painter->setPen(QPen(QColor(Qt::black), 0.5, Qt::DashLine));
		painter->drawLine(paperRect.left(), cr.top(), paperRect.right(), cr.top());
		painter->drawLine(paperRect.left(), cr.bottom(), paperRect.right(), cr.bottom());
		painter->drawLine(cr.left(), paperRect.top(), cr.left(), paperRect.bottom());
		painter->drawLine(cr.right(), paperRect.top(), cr.right(), paperRect.bottom());
		painter->restore();
	}

}

void MultiLayer::setFonts(const QFont& titleFnt, const QFont& scaleFnt,
		const QFont& numbersFnt, const QFont& legendFnt)
{
	for (int i=0;i<(int)graphsList.count();i++){
        Graph *gr=static_cast<Graph *>(graphsList.at(i));
		QwtPlot *plot=gr->plotWidget();

		QwtText text = plot->title();
  	    text.setFont(titleFnt);
  	    plot->setTitle(text);
		for (int j= 0;j<QwtPlot::axisCnt;j++){
			plot->setAxisFont (j,numbersFnt);

			text = plot->axisTitle(j );
  	        text.setFont(scaleFnt);
  	        plot->setAxisTitle(j, text);
		}

		QList <LegendWidget *> texts = gr->textsList();
		foreach (LegendWidget *l, texts)
			l->setFont(legendFnt);

		plot->replot();
	}
	emit modifiedPlot();
}

void MultiLayer::connectLayer(Graph *g)
{
	connect (g,SIGNAL(drawLineEnded(bool)), this, SIGNAL(drawLineEnded(bool)));
	connect (g,SIGNAL(showPlotDialog(int)),this,SIGNAL(showPlotDialog(int)));
	connect (g,SIGNAL(createTable(const QString&,int,int,const QString&)),
			this,SIGNAL(createTable(const QString&,int,int,const QString&)));
	connect (g,SIGNAL(viewLineDialog()),this,SIGNAL(showLineDialog()));
	connect (g,SIGNAL(showContextMenu()),this,SIGNAL(showGraphContextMenu()));
	connect (g,SIGNAL(showAxisDialog(int)),this,SIGNAL(showAxisDialog(int)));
	connect (g,SIGNAL(axisDblClicked(int)),this,SIGNAL(showScaleDialog(int)));
	connect (g,SIGNAL(showAxisTitleDialog()),this,SIGNAL(showAxisTitleDialog()));
	connect (g,SIGNAL(showMarkerPopupMenu()),this,SIGNAL(showMarkerPopupMenu()));
	connect (g,SIGNAL(showCurveContextMenu(int)),this,SIGNAL(showCurveContextMenu(int)));
	connect (g,SIGNAL(cursorInfo(const QString&)),this,SIGNAL(cursorInfo(const QString&)));
	connect (g,SIGNAL(viewImageDialog()),this,SIGNAL(showImageDialog()));
	connect (g,SIGNAL(viewTitleDialog()),this,SIGNAL(viewTitleDialog()));
	connect (g,SIGNAL(modifiedGraph()),this,SIGNAL(modifiedPlot()));
	connect (g,SIGNAL(selectedGraph(Graph*)),this, SLOT(setActiveGraph(Graph*)));
	connect (g,SIGNAL(viewTextDialog()),this,SIGNAL(showTextDialog()));
	connect (g,SIGNAL(currentFontChanged(const QFont&)), this, SIGNAL(currentFontChanged(const QFont&)));
  connect (g,SIGNAL(enableTextEditor(Graph *)), this, SIGNAL(enableTextEditor(Graph *)));
  connect (g,SIGNAL(dragMousePress(QPoint)), this, SIGNAL(dragMousePress(QPoint)));
  connect (g,SIGNAL(dragMouseRelease(QPoint)), this, SIGNAL(dragMouseRelease(QPoint)));
  connect (g,SIGNAL(dragMouseMove(QPoint)), this, SIGNAL(dragMouseMove(QPoint)));
}

bool MultiLayer::eventFilter(QObject *object, QEvent *e)
{
  if(e->type() == QEvent::Resize && object == (QObject *)canvas)
  {
    resizeLayers((QResizeEvent *)e);
    return true;
  }
  else if (e->type() == QEvent::MouseButtonPress )
  {
    if( object == (QObject *)canvas)
    {
      const QMouseEvent *me = (const QMouseEvent *)e;
      if (me->button() == Qt::RightButton)
        return MdiSubWindowParent_t::eventFilter(object, e);

      QPoint pos = canvas->mapFromParent(me->pos());
      // iterate backwards, so layers on top are preferred for selection
      QList<Graph*>::iterator i = graphsList.end();
      while (i != graphsList.begin()) {
        --i;
        Graph *g = *i;
        if (g->selectedText() || g->titleSelected() || g->selectedScale()){
          g->deselect();
          return true;
        }

        QRect igeo = (*i)->frameGeometry();
        if (igeo.contains(pos)) {
          if (me->modifiers() & Qt::ShiftModifier) {
            if (d_layers_selector)
              d_layers_selector->add(*i);
            else {
              d_layers_selector = new SelectionMoveResizer(*i);
              connect(d_layers_selector, SIGNAL(targetsChanged()), this, SIGNAL(modifiedPlot()));
            }
          }
          return true;
        }
      }
    }
    if (d_layers_selector)
    {
      removeLayerSelectionFrame();
    }
  }

	return MdiSubWindow::eventFilter(object, e);
}

void MultiLayer::keyPressEvent(QKeyEvent * e)
{
	if (e->key() == Qt::Key_F12){
    removeLayerSelectionFrame();
    int index = graphsList.indexOf(active_graph) + 1;
		if (index >= graphsList.size())
			index = 0;
        Graph *g = static_cast<Graph *>(graphsList.at(index));
		if (g)
			setActiveGraph(g);
		return;
	}

	if (e->key() == Qt::Key_F10){
    removeLayerSelectionFrame();
		int index = graphsList.indexOf(active_graph) - 1;
		if (index < 0)
			index = graphsList.size() - 1;
        Graph *g = static_cast<Graph *>(graphsList.at(index));
		if (g)
			setActiveGraph(g);
		return;
	}

	if (e->key() == Qt::Key_F11){
		emit showContextMenu();
		return;
	}
}

/**
 * Ensures all layers are removed promptly
 */
void MultiLayer::closeEvent(QCloseEvent* e)
{
  MdiSubWindow::closeEvent(e);
  if( e->isAccepted() )
  {
    const int nlayers = layers();
    for(int i = 0; i < nlayers; ++i)
    {
      removeLayer();
    }
  }
}

void MultiLayer::wheelEvent ( QWheelEvent * e )
{
	QApplication::setOverrideCursor(Qt::waitCursor);

	bool resize=false;
	QPoint aux;
	QSize intSize;
	Graph *resize_graph = 0;
	// Get the position of the mouse
	int xMouse=e->x();
	int yMouse=e->y();
	for (int i=0;i<(int)graphsList.count();i++){
        Graph *gr=static_cast<Graph *>(graphsList.at(i));
		intSize=gr->plotWidget()->size();
		aux=gr->pos();
		if(xMouse>aux.x() && xMouse<(aux.x()+intSize.width())){
			if(yMouse>aux.y() && yMouse<(aux.y()+intSize.height())){
				resize_graph=gr;
				resize=TRUE;
			}
		}
	}
	if(resize && (e->state()==Qt::AltButton || e->state()==Qt::ControlButton || e->state()==Qt::ShiftButton))
	{
		intSize = resize_graph->plotWidget()->size();
		if(e->state()==Qt::AltButton){// If alt is pressed then change the width
			if(e->delta()>0)
				intSize.rwidth()+=5;
			else if(e->delta()<0)
				intSize.rwidth()-=5;
		} else if(e->state()==Qt::ControlButton){// If crt is pressed then changed the height
			if(e->delta()>0)
				intSize.rheight()+=5;
			else if(e->delta()<0)
				intSize.rheight()-=5;
		} else if(e->state()==Qt::ShiftButton){// If shift is pressed then resize
			if(e->delta()>0){
				intSize.rwidth()+=5;
				intSize.rheight()+=5;
			} else if(e->delta()<0){
				intSize.rwidth()-=5;
				intSize.rheight()-=5;
			}
		}

		aux = resize_graph->pos();
		resize_graph->setGeometry(QRect(QPoint(aux.x(),aux.y()),intSize));
		resize_graph->plotWidget()->resize(intSize);

		emit modifiedPlot();
	}
	QApplication::restoreOverrideCursor();
}

bool MultiLayer::isEmpty ()
{
	if (graphsList.count() <= 0)
		return true;
	else
		return false;
}

void MultiLayer::setMargins (int lm, int rm, int tm, int bm)
{
	if (left_margin != lm)
		left_margin = lm;
	if (right_margin != rm)
		right_margin = rm;
	if (top_margin != tm)
		top_margin = tm;
	if (bottom_margin != bm)
		bottom_margin = bm;
}

void MultiLayer::setSpacing (int rgap, int cgap)
{
	if (rowsSpace != rgap)
		rowsSpace = rgap;
	if (colsSpace != cgap)
		colsSpace = cgap;
}

void MultiLayer::setLayerCanvasSize (int w, int h)
{
	if (l_canvas_width != w)
		l_canvas_width = w;
	if (l_canvas_height != h)
		l_canvas_height = h;
}

void MultiLayer::setAlignement (int ha, int va)
{
	if (hor_align != ha)
		hor_align = ha;

	if (vert_align != va)
		vert_align = va;
}

void MultiLayer::setLayersNumber(int n)
{
	if (graphsList.size() == n)
		return;

	int dn = graphsList.size() - n;
	if (dn > 0){
		for (int i = 0; i < dn; i++){//remove layer buttons
            LayerButton *btn=static_cast<LayerButton*>(buttonsList.last());
			if (btn){
				btn->close();
				buttonsList.removeLast();
			}

            Graph *g = static_cast<Graph *>(graphsList.last());
			if (g){//remove layers
				if (g->zoomOn() || g->activeTool())
					setPointerCursor();

				g->close();
				graphsList.removeLast();
			}
		}
		if (graphsList.size() <= 0){
			active_graph = 0;
			return;
		}

		// check whether the active Graph.has been deleted
		if(graphsList.indexOf(active_graph) == -1)
			active_graph=static_cast<Graph*>( graphsList.last());
        for (int j=0;j<static_cast<int>(graphsList.count());j++){
            Graph *gr=static_cast<Graph *>(graphsList.at(j));
			if (gr == active_graph){
                LayerButton *button=static_cast<LayerButton *>(buttonsList.at(j));
				button->setOn(TRUE);
				break;
			}
		}
	}else{
		for (int i = 0; i < abs(dn); i++)
			addLayer();
	}

	emit modifiedPlot();
}

void MultiLayer::copy(MultiLayer* ml)
{
	hide();//FIXME: find a better way to avoid a resize event
    resize(ml->size());

	setSpacing(ml->rowsSpacing(), ml->colsSpacing());
	setAlignement(ml->horizontalAlignement(), ml->verticalAlignement());
	setMargins(ml->leftMargin(), ml->rightMargin(), ml->topMargin(), ml->bottomMargin());

	QList<Graph *> layers = ml->layersList();
	foreach(Graph *g, layers){
		Graph* g2 = addLayer(g->pos().x(), g->pos().y(), g->width(), g->height());
		g2->copy(g);
		g2->setIgnoreResizeEvents(g->ignoresResizeEvents());
		g2->setAutoscaleFonts(g->autoscaleFonts());
	}

	if (ml->isWaterfallPlot())
		setWaterfallLayout(true);

	show();
}

bool MultiLayer::focusNextPrevChild ( bool next )
{
	if (!active_graph)
		return true;

	return active_graph->focusNextPrevChild(next);
}

void MultiLayer::dragEnterEvent( QDragEnterEvent * event )
{
  QObject * workspaceTree = applicationWindow()->findChild<QObject*>("WorkspaceTree");
  if ( event->source() == workspaceTree)
  {
    event->acceptProposedAction();
  }
}

/// Accepts drops from the Workspace browser and adds a curve from the selected workspace(s)
void MultiLayer::dropEvent( QDropEvent * event )
{
  MantidTreeWidget * tree = dynamic_cast<MantidTreeWidget*>(event->source());
  
  Graph *g = this->activeGraph();
  if (!g) return; // (shouldn't happen either)
  
  if(g->curves() > 0)
  {
    //Do some capability queries on the base curve.
    MantidMatrixCurve * asMatrixCurve = dynamic_cast<MantidMatrixCurve*>(g->curve(0));
    MantidMDCurve* asMDCurve = dynamic_cast<MantidMDCurve*>(g->curve(0));

    if(NULL == asMatrixCurve && NULL != asMDCurve)
    {
      //Treat as a MDCurve
      dropOntoMDCurve(g, asMDCurve, tree);
    }
    else
    {
      //Anything else we treat as a MantidMatrixCurve.
      dropOntoMatrixCurve(g, asMatrixCurve, tree);
    }
  }
}

/** Drop a workspace onto an exisiting MantidMDCurve (plot of a MDWorkspace)
@param g : Graph object
@param originalCurve : the original MantidMDCurve onto which the new workspace(s) are to be dropped
@param tree : Mantid Tree widget
*/
void MultiLayer::dropOntoMDCurve(Graph *g, MantidMDCurve* originalCurve, MantidTreeWidget * tree)
{
  UNUSED_ARG(originalCurve);
  using namespace Mantid::API;
  QList<QString> allWsNames = tree->getSelectedWorkspaceNames();

  if (allWsNames.size() <= 0) return;
  // Create a dialog to ask for options. Use the first workspace to choose the dimensions
  MantidMDCurveDialog * dlg = new MantidMDCurveDialog(g, allWsNames[0]);
  int result = dlg->exec();
  if (result == QDialog::Rejected)
    return;
  // Extract the settings from the dialog opened earlier
  bool showErrors = dlg->showErrorBars();
  LinePlotOptions * opts = dlg->getLineOptionsWidget();

  // Loop through all selected workspaces create curves and put them onto the graph
  for (int i=0; i<allWsNames.size(); i++)
  {
    //Capability query the candidate workspaces
    Workspace_sptr ws = AnalysisDataService::Instance().retrieve(allWsNames[i].toStdString());
    IMDWorkspace_sptr imdWS = boost::dynamic_pointer_cast<IMDWorkspace>(ws);
    //Only process IMDWorkspaces
    if(imdWS)
    {
      QString currentName(imdWS->name().c_str());
      try
      {
        MantidMDCurve* curve = new MantidMDCurve(currentName, g, showErrors);
        MantidQwtIMDWorkspaceData * data = curve->mantidData();
        // Apply the settings
        data->setPreviewMode(false);
        data->setPlotAxisChoice(opts->getPlotAxis());
        data->setNormalization(opts->getNormalization());

      }
      catch(std::invalid_argument& ex)
      {
        //Handle case when workspace does not have only one non-integrated dimension.
        g_log.warning() << ex.what() << std::endl;
      }
    }
  }
}

/*
Drop a workspace onto an exisiting matrix curve
@param g : Graph object
@param originalCurve : the original MantidMatrixCurve onto which the new workspace(s) are to be dropped
@param tree : Mantid Tree widget
*/
void MultiLayer::dropOntoMatrixCurve(Graph *g, MantidMatrixCurve* originalCurve, MantidTreeWidget * tree)
{
  bool errorBars;
  if(NULL != originalCurve)
  {
    errorBars = originalCurve->hasErrorBars();
  }
  else
  {
    // Else we'll just have no error bars.
    errorBars = false;
  }
  
  if ( tree == NULL ) return; // (shouldn't happen)
  QMultiMap<QString,std::set<int> > toPlot = tree->chooseSpectrumFromSelected();

  // Iterate through the selected workspaces adding a set of curves from each
  for(QMultiMap<QString,std::set<int> >::const_iterator it=toPlot.begin();it!=toPlot.end();++it)
  {
    std::set<int>::iterator setIt = it.value().begin();

    for( ; setIt != it.value().end(); ++setIt)
    {
      try {
        // If the current curve is plotted as a distribution then do so also here
        new MantidMatrixCurve(it.key(),g,(*setIt),MantidMatrixCurve::Spectrum, errorBars,
                              originalCurve->isDistribution()); // The graph takes ownership
      } catch (Mantid::Kernel::Exception::NotFoundError &) {
        // Get here if workspace name is invalid - shouldn't be possible, but just in case
      } catch (std::invalid_argument&) {
        // Get here if invalid spectrum number given - shouldn't be possible, but just in case
      }
    }
  }
  // Update the plot
  g->replot();

  if (d_is_waterfall_plot) updateWaterfalls();
}

/**
* Mark the layer selector for deletion and set the pointer to NULL
*/
void MultiLayer::removeLayerSelectionFrame()
{
  d_layers_selector->deleteLater();
  d_layers_selector = NULL;
}


bool MultiLayer::swapLayers(int src, int dest)
{
	Graph *layerSrc = layer(src);
	Graph *layerDest = layer(dest);
	if (!layerSrc || !layerDest)
		return false;

	QRect rectSrc = layerSrc->geometry();
	QRect rectDest = layerDest->geometry();

	layerSrc->setGeometry(rectDest);
    layerSrc->plotWidget()->resize(rectDest.size());

	layerDest->setGeometry(rectSrc);
    layerDest->plotWidget()->resize(rectSrc.size());

	graphsList[src-1] = layerDest;
	graphsList[dest-1] = layerSrc;

	emit modifiedPlot();
	return true;
}

/** Do something when the graphs are modified, e.g. it can close itself
 *  if it becomes empty.
 */
void MultiLayer::maybeNeedToClose()
{
  if (d_close_on_empty)
  {
    bool shouldClose = true;
    for(int i=1;i<=layers();i++)
    {
      Graph* g = layer(i);
      if (g && g->curves() > 0)
      {
        shouldClose = false;
        break;
      }
    }
    if (shouldClose) close();
  }
}

void MultiLayer::setWaterfallLayout(bool on)
{
  if (graphsList.isEmpty())
    return;

  d_is_waterfall_plot = on;

  if (on){
    createWaterfallBox();
    updateWaterfalls();
  } else {
    for (int i = 0; i < waterfallBox->count(); i++){
      QLayoutItem *item = waterfallBox->itemAt(i);
      if (item){
        waterfallBox->removeItem(item);
        delete item;
      }
    }
  }
}

void MultiLayer::createWaterfallBox()
{
  if (waterfallBox->count() > 0)
    return;

  QPushButton *btn = new QPushButton(tr("Offset Amount..."));
  connect (btn, SIGNAL(clicked()), this, SLOT(showWaterfallOffsetDialog()));

  waterfallBox->addWidget(btn);
  btn = new QPushButton(tr("Reverse Order"));
  connect (btn, SIGNAL(clicked()), this, SLOT(reverseWaterfallOrder()));

  waterfallBox->addWidget(btn);
  btn = new QPushButton(tr("Fill Area..."));
  connect (btn, SIGNAL(clicked()), this, SLOT(showWaterfallFillDialog()));
  waterfallBox->addWidget(btn);
}

void MultiLayer::updateWaterfalls()
{
  if (!d_is_waterfall_plot || graphsList.isEmpty())
    return;

  foreach(Graph *g, graphsList){
    if (g->isWaterfallPlot())
      g->updateDataCurves();
  }
}

void MultiLayer::showWaterfallOffsetDialog()
{
  if (graphsList.isEmpty() || !active_graph)
    return;
  if (active_graph->curvesList().isEmpty())
    return;

  QDialog *offsetDialog = new QDialog(this);
  offsetDialog->setWindowTitle(tr("Offset Dialog"));

  QGroupBox *gb1 = new QGroupBox();
  QGridLayout *hl1 = new QGridLayout(gb1);

  hl1->addWidget(new QLabel(tr("Total Y Offset (%)")), 0, 0);
  QSpinBox *yOffsetBox = new QSpinBox();
  yOffsetBox->setRange(0, INT_MAX);
  yOffsetBox->setValue(active_graph->waterfallYOffset());
  hl1->addWidget(yOffsetBox, 0, 1);

  hl1->addWidget(new QLabel(tr("Total X Offset (%)")), 1, 0);
  QSpinBox *xOffsetBox = new QSpinBox();
  xOffsetBox->setRange(0, INT_MAX);
  xOffsetBox->setValue(active_graph->waterfallXOffset());
  hl1->addWidget(xOffsetBox, 1, 1);
  hl1->setRowStretch(2, 1);

  connect(yOffsetBox, SIGNAL(valueChanged(int)), active_graph, SLOT(setWaterfallYOffset(int)));
  connect(xOffsetBox, SIGNAL(valueChanged(int)), active_graph, SLOT(setWaterfallXOffset(int)));

  //QPushButton *applyBtn = new QPushButton(tr("&Apply"));
  //connect(applyBtn, SIGNAL(clicked()), this, SLOT(updateWaterfalls()));

  QPushButton *closeBtn = new QPushButton(tr("&Close"));
  connect(closeBtn, SIGNAL(clicked()), offsetDialog, SLOT(reject()));

  QHBoxLayout *hl2 = new QHBoxLayout();
  hl2->addStretch();
  //hl2->addWidget(applyBtn);
  hl2->addWidget(closeBtn);

  QVBoxLayout *vl = new QVBoxLayout(offsetDialog);
  vl->addWidget(gb1);
  vl->addLayout(hl2);
  offsetDialog->exec();
}

void MultiLayer::reverseWaterfallOrder()
{
  if (graphsList.isEmpty() || !active_graph)
    return;

  active_graph->reverseCurveOrder();
  active_graph->updateDataCurves();
  active_graph->replot();
  emit modifiedWindow(this);
}

void MultiLayer::showWaterfallFillDialog()
{
  if (graphsList.isEmpty() || !active_graph)
    return;
  if (active_graph->curvesList().isEmpty())
    return;

  new WaterfallFillDialog(this, active_graph);  
}


void MultiLayer::setWaterfallFillColor(const QColor& c)
{
  d_waterfall_fill_color = c;
  if (active_graph)
    active_graph->setWaterfallFillColor(c);
}


WaterfallFillDialog::WaterfallFillDialog(MultiLayer *parent, Graph *active_graph) 
{
  this->setParent(parent);
  this->m_active_graph = active_graph;
  QDialog *waterfallFillDialog = new QDialog(this);
  waterfallFillDialog->setWindowTitle(tr("Fill Curves"));
  
  QGroupBox *enableFillGroup = new QGroupBox(tr("Enable Fill"), waterfallFillDialog);
  enableFillGroup->setCheckable(true);
  
  QGridLayout *enableFillLayout =  new QGridLayout(enableFillGroup);  

  // use line colour
  QRadioButton *rLineC = new QRadioButton("Use Line Colour", enableFillGroup);
  this->m_lineRadioButton = rLineC;
  enableFillLayout->addWidget(rLineC,0,0);
  
  // use solid colour
  QRadioButton *rSolidC = new QRadioButton("Use Solid Colour", enableFillGroup);
  this->m_solidRadioButton = rSolidC;
  enableFillLayout->addWidget(rSolidC, 1,0);

  QGroupBox *colourModeGroup = new QGroupBox( tr("Fill with Colour"), enableFillGroup);  
  
  QGridLayout *hl1 = new QGridLayout(colourModeGroup);
  hl1->addWidget(new QLabel(tr("Colour")), 0, 0);
  ColorButton *fillColourBox = new ColorButton(colourModeGroup);
  this->m_colourBox = fillColourBox;
  fillColourBox->setColor(Qt::white); // Default colour
  hl1->addWidget(fillColourBox, 0, 1);
  enableFillLayout->addWidget(colourModeGroup,2,0);

  QCheckBox *sideLinesBox = new QCheckBox(tr("Side Lines"), enableFillGroup);
  enableFillLayout->addWidget(sideLinesBox, 3, 0); 

  QBrush brush = active_graph->curve(0)->brush();

  // check if all curve colours are the same (= solid fill)
  bool same = brush.style() != Qt::NoBrush; // check isn't first run against graph
  
  if(same)
  {
    int n = active_graph->curvesList().size();
    for (int i = 0; i < n; i++)
    {
      same = same && (active_graph->curve(i)->brush().color() == brush.color());    
    }
  }
  // set which is toggled
  enableFillGroup->setChecked(brush.style() != Qt::NoBrush);

  if(same)
  {   
    rSolidC->toggle();
    if(enableFillGroup->isChecked())
      fillColourBox->setColor(brush.color());
  }
  else
  {
    rLineC->toggle();
    if(enableFillGroup->isChecked())
      active_graph->updateWaterfallFill(true);  
  }

  // If sidelines previously enabled, check it.
  PlotCurve *c = dynamic_cast<PlotCurve*>(active_graph->curve(0));
  sideLinesBox->setChecked(c->sideLinesEnabled());   
  
  colourModeGroup->setEnabled(rSolidC->isChecked() && enableFillGroup->isChecked());  
  
  connect(enableFillGroup, SIGNAL(toggled(bool)), this, SLOT(enableFill(bool))); 
  connect(fillColourBox, SIGNAL(colorChanged(const QColor&)), active_graph, SLOT(setWaterfallFillColor(const QColor&)));
  connect(sideLinesBox, SIGNAL(toggled(bool)), active_graph, SLOT(setWaterfallSideLines(bool)));  
  connect(rSolidC, SIGNAL(toggled(bool)), colourModeGroup, SLOT(setEnabled(bool)));  
  connect(rSolidC, SIGNAL(toggled(bool)), this, SLOT(setFillMode())); 
  connect(rLineC, SIGNAL(toggled(bool)), this, SLOT(setFillMode())); 
  
  QPushButton *closeBtn = new QPushButton(tr("&Close"),waterfallFillDialog);
  connect(closeBtn, SIGNAL(clicked()), waterfallFillDialog, SLOT(reject()));

  QHBoxLayout *hlClose = new QHBoxLayout();
  hlClose->addStretch();
  hlClose->addWidget(closeBtn);

  QVBoxLayout *vl = new QVBoxLayout(waterfallFillDialog);
  vl->addWidget(enableFillGroup);
  vl->addLayout(hlClose);
  waterfallFillDialog->exec();
}

void WaterfallFillDialog::enableFill(bool b)
{
  if(b)
  {
    WaterfallFillDialog::setFillMode();
  }
  else
  {
    m_active_graph->curve(0)->setBrush(Qt::BrushStyle::NoBrush);
    m_active_graph->updateWaterfallFill(false);     
  }
}

void WaterfallFillDialog::setFillMode()
{
  if( m_solidRadioButton->isChecked() ) 
  {                  
    m_active_graph->setWaterfallFillColor(this->m_colourBox->color());
  }    
  else if( m_lineRadioButton->isChecked() )
  {       
    m_active_graph->updateWaterfallFill(true); 
  }
}

void MultiLayer::loadFromProject(const std::string& lines, ApplicationWindow* app, const int fileVersion)
{
  TSVSerialiser tsv(lines);

  if(tsv.hasLine("geometry"))
    app->restoreWindowGeometry(app, this, QString::fromStdString(tsv.lineAsString("geometry")));

  blockSignals(true);

  if(tsv.selectLine("WindowLabel"))
  {
    setWindowLabel(QString::fromUtf8(tsv.asString(1).c_str()));
    setCaptionPolicy((MdiSubWindow::CaptionPolicy)tsv.asInt(2));
  }

  if(tsv.selectLine("Margins"))
  {
    int left, right, top, bottom;
    tsv >> left >> right >> top >> bottom;
    setMargins(left, right, top, bottom);
  }

  if(tsv.selectLine("Spacing"))
  {
    int rowSpace, colSpace;
    tsv >> rowSpace >> colSpace;
    setSpacing(rowSpace, colSpace);
  }

  if(tsv.selectLine("LayerCanvasSize"))
  {
    int width, height;
    tsv >> width >> height;
    setLayerCanvasSize(width, height);
  }

  if(tsv.selectLine("Alignement"))
  {
    int hor, vert;
    tsv >> hor >> vert;
    setAlignement(hor, vert);
  }

  if(tsv.hasSection("waterfall"))
  {
    const std::string wfStr = tsv.sections("waterfall").front();

    if(wfStr == "1")
      setWaterfallLayout(true);
    else
      setWaterfallLayout(false);
  }

  if(tsv.hasSection("graph"))
  {
    std::vector<std::string> graphSections = tsv.sections("graph");
    for(auto it = graphSections.begin(); it != graphSections.end(); ++it)
    {
      const std::string graphLines = *it;

      TSVSerialiser gtsv(graphLines);

      if(gtsv.selectLine("ggeometry"))
      {
        int x, y, w, h;
        gtsv >> x >> y >> w >> h;

        Graph* g = dynamic_cast<Graph*>(addLayer(x,y,w,h));
        if(g)
          g->loadFromProject(graphLines, app, fileVersion);
      }
    }
  }

  blockSignals(false);
}

std::string MultiLayer::saveToProject(ApplicationWindow* app)
{
  TSVSerialiser tsv;

  tsv.writeRaw("<multiLayer>");

  tsv.writeLine(objectName().toStdString()) << d_cols << d_rows << birthDate();
  tsv.writeRaw(app->windowGeometryInfo(this));

  tsv.writeLine("WindowLabel") << windowLabel() << captionPolicy();
  tsv.writeLine("Margins") << left_margin << right_margin << top_margin << bottom_margin;
  tsv.writeLine("Spacing") << rowsSpace << colsSpace;
  tsv.writeLine("LayerCanvasSize") << l_canvas_width << l_canvas_height;
  tsv.writeLine("Alignement") << hor_align << vert_align;

  foreach(Graph* g, graphsList)
    tsv.writeSection("graph", g->saveToProject());

  if(d_is_waterfall_plot)
    tsv.writeInlineSection("waterfall", "1");

  tsv.writeRaw("</multiLayer>");

  return tsv.outputLines();
}
