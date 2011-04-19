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

#include "Mantid/MantidDock.h"
#include "Mantid/MantidCurve.h"

#include <gsl/gsl_vector.h>

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
                         : MdiSubWindow(label, parent, name, f),
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
                         d_print_cropmarks(false)
{
	layerButtonsBox = new QHBoxLayout();
	buttonsLine = new QHBoxLayout();
	buttonsLine->addLayout(layerButtonsBox);
	buttonsLine->addStretch();

	canvas = new QWidget();

	QWidget *mainWidget = new QWidget();
	mainWidget->setAutoFillBackground(true);
	mainWidget->setBackgroundRole(QPalette::Window);

	QVBoxLayout* layout = new QVBoxLayout(mainWidget);
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

	setFocusPolicy(Qt::StrongFocus);
	setFocus();

  setAcceptDrops(true);
}

Graph *MultiLayer::layer(int num)
{
    int index = num - 1;
    if (index < 0 || index >= graphsList.count())
        return 0;

	return (Graph*) graphsList.at(index);
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
		LayerButton *btn=(LayerButton*)buttonsList.at(i);
		if (btn->isOn())
			btn->setOn(false);

		if (btn == button)
		{
			active_graph = (Graph*) graphsList.at(i);
			active_graph->setFocus();
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

	active_graph = g;
	active_graph->setFocus();

	if (d_layers_selector)
  {
    removeLayerSelectionFrame();
  }
	active_graph->raise();//raise layer on top of the layers stack

	for(int i=0; i<graphsList.count(); i++){
		Graph *gr = (Graph *)graphsList.at(i);
		gr->deselect();

		LayerButton *btn = (LayerButton *)buttonsList.at(i);
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
					((LegendWidget *)o)->setFixedCoordinatesMode();
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

	emit modifiedPlot();
	repaint();
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
	active_graph->close();
	if(index >= graphsList.count())
		index--;

	if (graphsList.count() == 0){
		active_graph = 0;
		return;
	}

	active_graph=(Graph*) graphsList.at(index);

	for (i=0;i<(int)graphsList.count();i++){
		Graph *gr=(Graph *)graphsList.at(i);
		if (gr == active_graph){
			LayerButton *button = (LayerButton *)buttonsList.at(i);
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
		Graph *gr = (Graph *)graphsList.at(i);
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
		Graph *gr = (Graph *)graphsList.at(i);
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
		Plot *plot = (Plot *)g->plotWidget();
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
        Plot *plot = (Plot *)g->plotWidget();

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
		Plot *plot = (Plot *)g->plotWidget();
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
		((Graph *)g)->deselectMarker();

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
			Graph *gr=(Graph *)graphsList.at(i);
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
			Graph *gr = (Graph *)graphsList.at(i);
			Plot *myPlot = (Plot *)gr->plotWidget();

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
		Graph *gr=(Graph *)graphsList.at(i);
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
	connect (g,SIGNAL(drawTextOff()),this,SIGNAL(drawTextOff()));
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
}

bool MultiLayer::eventFilter(QObject *object, QEvent *e)
{
	if(e->type() == QEvent::Resize && object == (QObject *)canvas)
		resizeLayers((QResizeEvent *)e);
  else if (e->type() == QEvent::MouseButtonPress )
  {
    if( object == (QObject *)canvas)
    {
      const QMouseEvent *me = (const QMouseEvent *)e;
      if (me->button() == Qt::RightButton)
        return QMdiSubWindow::eventFilter(object, e);

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
		Graph *g = (Graph *)graphsList.at(index);
		if (g)
			setActiveGraph(g);
		return;
	}

	if (e->key() == Qt::Key_F10){
    removeLayerSelectionFrame();
		int index = graphsList.indexOf(active_graph) - 1;
		if (index < 0)
			index = graphsList.size() - 1;
		Graph *g = (Graph *)graphsList.at(index);
		if (g)
			setActiveGraph(g);
		return;
	}

	if (e->key() == Qt::Key_F11){
		emit showContextMenu();
		return;
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
		Graph *gr=(Graph *)graphsList.at(i);
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

QString MultiLayer::saveToString(const QString& geometry, bool saveAsTemplate)
{
    bool notTemplate = !saveAsTemplate;
	QString s="<multiLayer>\n";
	if (notTemplate)
        s+=QString(objectName())+"\t";
	s+=QString::number(d_cols)+"\t";
	s+=QString::number(d_rows)+"\t";
	if (notTemplate)
        s+=birthDate()+"\n";
	s+=geometry;
	if (notTemplate)
        s+="WindowLabel\t" + windowLabel() + "\t" + QString::number(captionPolicy()) + "\n";
	s+="Margins\t"+QString::number(left_margin)+"\t"+QString::number(right_margin)+"\t"+
		QString::number(top_margin)+"\t"+QString::number(bottom_margin)+"\n";
	s+="Spacing\t"+QString::number(rowsSpace)+"\t"+QString::number(colsSpace)+"\n";
	s+="LayerCanvasSize\t"+QString::number(l_canvas_width)+"\t"+QString::number(l_canvas_height)+"\n";
	s+="Alignement\t"+QString::number(hor_align)+"\t"+QString::number(vert_align)+"\n";

	for (int i=0; i<(int)graphsList.count(); i++){
		Graph* ag=(Graph*)graphsList.at(i);
		s += ag->saveToString(saveAsTemplate);
	}
	return s+"</multiLayer>\n";
}

QString MultiLayer::saveAsTemplate(const QString& geometryInfo)
{
	return saveToString(geometryInfo, true);
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
			LayerButton *btn=(LayerButton*)buttonsList.last();
			if (btn){
				btn->close();
				buttonsList.removeLast();
			}

			Graph *g = (Graph *)graphsList.last();
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
			active_graph=(Graph*) graphsList.last();
		for (int j=0;j<(int)graphsList.count();j++){
			Graph *gr=(Graph *)graphsList.at(j);
			if (gr == active_graph){
				LayerButton *button=(LayerButton *)buttonsList.at(j);
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
  QObject * workspaceTree = this->parent()->parent()->parent()->findChild<QObject*>("WorkspaceTree");
  if ( event->source() == workspaceTree)
  {
    event->acceptProposedAction();
  }
}

/// Accepts drops from the Workspace browser and adds a curve from the selected workspace(s)
void MultiLayer::dropEvent( QDropEvent * event )
{
  MantidTreeWidget * tree = dynamic_cast<MantidTreeWidget*>(event->source());
  if ( tree == NULL ) return; // (shouldn't happen)

  // Ask the user which spectrum to plot
  QMultiMap<QString,int> toPlot = tree->chooseSpectrumFromSelected();
  Graph *g = this->activeGraph();
  if (!g) return; // (shouldn't happen either)

  // Iterate through the selected workspaces adding a curve from each
  for(QMultiMap<QString,int>::const_iterator it=toPlot.begin();it!=toPlot.end();it++)
  {
    try {
      new MantidCurve(it.key(),g,"spectra",it.value(),false); // Always without errors for now
    } catch (Mantid::Kernel::Exception::NotFoundError) {
      // Get here if workspace name is invalid - shouldn't be possible, but just in case
    } catch (std::invalid_argument&) {
      // Get here if invalid spectrum number given - shouldn't be possible, but just in case
    }
  }
  // Update the axes
  g->setAutoScale();
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
