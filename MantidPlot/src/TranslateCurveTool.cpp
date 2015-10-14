/***************************************************************************
    File                 : TranslateCurveTool.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006,2007 by Ion Vasilief,
                           Tilman Hoener zu Siederdissen, Knut Franke
    Email (use @ for *)  : ion_vasilief*yahoo.fr, thzs*gmx.net,
                           knut.franke*gmx.de
    Description          : Plot tool for translating curves.

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
#include "TranslateCurveTool.h"
#include "Graph.h"
#include "PlotCurve.h"
#include "FunctionCurve.h"
#include "ApplicationWindow.h"
#include "cursors.h"
#include "DataPickerTool.h"
#include "ScreenPickerTool.h"
#include <limits>
#include <QMessageBox>
#include <QLocale>
#include <qwt_plot_curve.h>

TranslateCurveTool::TranslateCurveTool(Graph *graph, ApplicationWindow *app, Direction dir, const QObject *status_target, const char *status_slot)
	: PlotToolInterface(graph),
	d_dir(dir),
        d_sub_tool(NULL),
        d_selected_curve(NULL),
        d_curve_point(),
	d_app(app)
{
	if (status_target)
		connect(this, SIGNAL(statusText(const QString&)), status_target, status_slot);
	//d_graph->plotWidget()->canvas()->setCursor(QCursor(getQPixmap("vizor_xpm"), -1, -1));
	//emit statusText(tr("Double-click on plot to select a data point!"));

	// Phase 1: select curve point
	d_sub_tool = new DataPickerTool(d_graph, app, DataPickerTool::Display, this, SIGNAL(statusText(const QString&)));
	connect(dynamic_cast<DataPickerTool*>(d_sub_tool), SIGNAL(selected(QwtPlotCurve*,int)),
			this, SLOT(selectCurvePoint(QwtPlotCurve*,int)));
  //d_sub_tool = NULL;
}

void TranslateCurveTool::selectCurvePoint(QwtPlotCurve *curve, int point_index)
{
  if (!d_sub_tool)
    return;
  DataCurve *c = dynamic_cast<DataCurve *>(curve);
  if (c && c->type() != Graph::Function) {
    
    Table *t = c->table();
    if (!t)
      return;

    if (d_dir == Horizontal &&
        t->isReadOnlyColumn(t->colIndex(c->xColumnName()))) {
      QMessageBox::warning(
          d_app, tr("MantidPlot - Warning"),
          tr("The column '%1' is read-only! Operation aborted!")
              .arg(c->xColumnName()));
      delete d_sub_tool;
      d_graph->setActiveTool(NULL);
      return;
    } else if (d_dir == Vertical &&
               t->isReadOnlyColumn(t->colIndex(c->title().text()))) {
      QMessageBox::warning(
          d_app, tr("MantidPlot - Warning"),
          tr("The column '%1' is read-only! Operation aborted!")
              .arg(c->title().text()));
      delete d_sub_tool;
      d_graph->setActiveTool(NULL);
      return;
    }
  }

  d_selected_curve = curve;
	d_curve_point = QwtDoublePoint(curve->x(point_index), curve->y(point_index));
	delete d_sub_tool;

	// Phase 2: select destination
	d_sub_tool = new ScreenPickerTool(d_graph, this, SIGNAL(statusText(const QString&)));
	connect(dynamic_cast<ScreenPickerTool*>(d_sub_tool), SIGNAL(selected(const QwtDoublePoint&)), this, SLOT(selectDestination(const QwtDoublePoint&)));
	emit statusText(tr("Curve selected! Move cursor and click to choose a point and double-click/press 'Enter' to finish!"));
}

void TranslateCurveTool::selectDestination(const QwtDoublePoint &point)
{
  if (!d_sub_tool) return;
	delete d_sub_tool;
	if (!d_selected_curve)
		return;

	// Phase 3: execute the translation

	if(auto c = dynamic_cast<PlotCurve *>(d_selected_curve)){
    if (c->type() == Graph::Function) {
	    if (d_dir == Horizontal){
            QMessageBox::warning(d_app, tr("MantidPlot - Warning"),
            tr("This operation cannot be performed on function curves."));
        } else if (FunctionCurve *func = dynamic_cast<FunctionCurve *>(d_selected_curve)) {
            if (func->functionType() == FunctionCurve::Normal){
                QString formula = func->formulas().first();
                double d = point.y() - d_curve_point.y();
                if (d > 0)
                    func->setFormula(formula + "+" + QString::number(d, 'g', 15));
                else
                    func->setFormula(formula + QString::number(d, 'g', 15));
                func->loadData();
            }
        }
	    d_graph->setActiveTool(NULL);
	    return;
    }
  } else if (DataCurve *c = dynamic_cast<DataCurve *>(d_selected_curve)) {
    double d;
		QString col_name;
		switch(d_dir) {
			case Vertical:
			{
				col_name = c->title().text();
				d = point.y() - d_curve_point.y();
				break;
			}
			case Horizontal:
			{
				col_name = c->xColumnName();
				d = point.x() - d_curve_point.x();
				break;
			}
            default: // this should never happen
            {
                d = std::numeric_limits<float>::quiet_NaN();
            }
	}
	Table *tab = d_app->table(col_name);
	if (!tab) return;
	int col = tab->colIndex(col_name);
	if (tab->columnType(col) != Table::Numeric) {
		QMessageBox::warning(d_app, tr("MantidPlot - Warning"),
				tr("This operation cannot be performed on curves plotted from columns having a non-numerical format."));
		return;
	}

	int prec; char f;
	tab->columnNumericFormat(col, &f, &prec);
	int row_start = c->tableRow(0);
    int row_end = row_start + c->dataSize();

    QLocale locale = d_app->locale();
	for (int i=row_start; i<row_end; i++){
		if (!tab->text(i, col).isEmpty())
			tab->setText(i, col, locale.toString(
					(d_dir==Horizontal ? d_selected_curve->x(i) : d_selected_curve->y(i)) + d, f, prec));
	}
	d_app->updateCurves(tab, col_name);
	d_app->modifiedProject();
	d_graph->setActiveTool(NULL);
	// attention: I'm now deleted. Maybe there is a cleaner solution...*/
    }
}
