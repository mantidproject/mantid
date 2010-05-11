/***************************************************************************
    File                 : PlotCurve.h
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2007 by Ion Vasilief
    Email (use @ for *)  : ion_vasilief*yahoo.fr
    Description          : AbstractPlotCurve and DataCurve classes

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
#ifndef PLOTCURVE_H
#define PLOTCURVE_H

#include <qwt_plot_curve.h>
#include <qwt_plot_marker.h>
#include "Table.h"

class PlotMarker;

//! Abstract 2D plot curve class
class PlotCurve: public QObject, public QwtPlotCurve
{
Q_OBJECT
public:
	PlotCurve(const QString& name = QString()): QwtPlotCurve(name), d_type(0), d_x_offset(0.0), d_y_offset(0.0){};
 PlotCurve(const PlotCurve& c): QObject(), QwtPlotCurve(c.title().text()), d_type(c.d_type), 
    d_x_offset(c.d_x_offset), d_y_offset(c.d_y_offset){};

  virtual PlotCurve* clone()const = 0;

	int type()const{return d_type;};
	void setType(int t){d_type = t;};

	double xOffset()const{return d_x_offset;};
	void setXOffset(double dx){d_x_offset = dx;};

	double yOffset()const{return d_y_offset;};
	void setYOffset(double dy){d_y_offset = dy;};

	QString saveCurveLayout();
	void restoreCurveLayout(const QStringList& lst);

  // remove this curve form the graph (and delete from memory)
  void removeMe(){emit removeMe(this);}

  // to be called only by Plot before deleting the curve
  void aboutToBeDeleted();

signals:

  // fired to tell the owing plot to remove it
  void removeMe(PlotCurve*);
  void dataUpdated();
  // fired to indicate that this curve is about to be deleted from memory
  // and any reference to it must be invalidated
  void forgetMe(PlotCurve*);

protected:
	int d_type;
	double d_x_offset, d_y_offset;
};

class DataCurve: public PlotCurve
{
public:
	DataCurve(Table *t, const QString& xColName, const QString& name, int startRow = 0, int endRow = -1);
  DataCurve(const DataCurve& c);
    void clone(DataCurve* c);

    PlotCurve* clone()const;

    QString saveToString();

	QString xColumnName(){return d_x_column;};
	void setXColumnName(const QString& name){d_x_column = name;};

	bool hasLabels()const{return !d_labels_list.isEmpty();};
	QString labelsColumnName()const{return d_labels_column;};
	void setLabelsColumnName(const QString& name);

    int labelsAlignment()const{return d_labels_align;};
    void setLabelsAlignment(int flags);

    int labelsXOffset()const{return d_labels_x_offset;};
    int labelsYOffset()const{return d_labels_y_offset;};
    void setLabelsOffset(int x, int y);

    double labelsRotation()const{return d_labels_angle;};
    void setLabelsRotation(double angle);

    QFont labelsFont()const{return d_labels_font;};
    void setLabelsFont(const QFont& font);

    QColor labelsColor()const{return d_labels_color;};
    void setLabelsColor(const QColor& c);

    bool labelsWhiteOut()const{return d_white_out_labels;};
    void setLabelsWhiteOut(bool whiteOut = true);

	Table* table()const{return d_table;};

	int startRow()const{return d_start_row;};
	int endRow()const{return d_end_row;};
	void setRowRange(int startRow, int endRow);

	bool isFullRange()const;
	void setFullRange();

	virtual bool updateData(Table *t, const QString& colName);
	virtual void loadData();

	//! Returns the row index in the data source table corresponding to the data point index.
	int tableRow(int point)const;

	void remove();

        /**
		 * \brief A list of data sources for this curve.
		 *
		 * Elements must be in either of the following forms:
		 *  - &lt;id of X column> "(X)," &lt;id of Y column> "(Y)" [ "," &lt;id of error column> ("(xErr)" | "(yErr)") ]
		 *  - &lt;id of Xstart column> "(X)," &lt;id of Ystart column>"(Y)," &lt;id of Xend column> "(X)," &lt;id of Yend column> "(Y)"\n
		 *    (denoting start and end coordinates for the #VectXYXY style)
		 *  - &lt;id of Xstart column> "(X)," &lt;id of Ystart column> "(Y)," &lt;id of angle column> "(A)," &lt;id of magnitude column> "(M)"\n
		 *    (denoting start coordinates, angle in radians and length for the #VectXYAM style)
		 *
		 * Column ids are of the form '&lt;name of table> "_" &lt;name of column>'.
		 */
    virtual QString plotAssociation()const;
	virtual void updateColumnNames(const QString& oldName, const QString& newName, bool updateTableName);

	//! The list of attached error bars.
	QList<DataCurve *> errorBarsList()const{return d_error_bars;};
	//! Adds a single error bars curve to the list of attached error bars.
	void addErrorBars(DataCurve *c){if (c) d_error_bars << c;};
	//! Remove a single error bars curve from the list of attached error bars.
	void removeErrorBars(DataCurve *c);
	//! Clears the list of attached error bars.
	void clearErrorBars();
	//! Clears the list of attached text labels.
	void clearLabels();

	void setVisible(bool on);

	bool selectedLabels(const QPoint& pos);
	bool hasSelectedLabels()const;
	void setLabelsSelected(bool on = true);

	void moveLabels(const QPoint& pos);
    void updateLabelsPosition();

protected:
	bool validCurveType()const;
    void loadLabels();

	//! List of the error bar curves associated to this curve.
	QList <DataCurve *> d_error_bars;
	//! The data source table.
	Table *d_table;
	//!\brief The name of the column used for abscissae values.
	/*
	 *The column name used for Y values is stored in title().text().
	 */
	QString d_x_column;

	int d_start_row;
	int d_end_row;

	//!\brief The name of the column used for text labels.
	QString d_labels_column;

	//! List of the text labels associated to this curve.
	QList <PlotMarker *> d_labels_list;
	//! Keep track of the coordinates of the point where the user clicked when selecting the labels.
	double d_click_pos_x, d_click_pos_y;

	QColor d_labels_color;
	QFont d_labels_font;
	double d_labels_angle;
	bool d_white_out_labels;
	int d_labels_align, d_labels_x_offset, d_labels_y_offset;
	//! Keeps track of the plot marker on which the user clicked when selecting the labels.
	PlotMarker *d_selected_label;
};

class PlotMarker: public QwtPlotMarker
{
public:
	PlotMarker(int index, double angle);

	int index(){return d_index;};
	void setIndex(int i){d_index = i;};

	double angle(){return d_angle;};
	void setAngle(double a){d_angle = a;};
	void setLabelOffset(double xOffset, double yOffset){d_label_x_offset = xOffset; d_label_y_offset = yOffset;};

	//QwtDoubleRect boundingRect() const;

protected:
	//! Does the actual drawing; see QwtPlotItem::draw.
	void draw(QPainter *p, const QwtScaleMap &xMap, const QwtScaleMap &yMap, const QRect &r) const;

	int d_index;
	double d_angle;
	double d_label_x_offset;
	//! y coordinate offset
	double d_label_y_offset;
};
#endif
