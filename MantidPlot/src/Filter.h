/***************************************************************************
    File                 : Filter.h
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2007 by Ion Vasilief
    Email (use @ for *)  : ion_vasilief*yahoo.fr
    Description          : Abstract base class for data analysis operations

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
#ifndef FILTER_H
#define FILTER_H

#include <QObject>

#include "ApplicationWindow.h"

class QwtPlotCurve;
class Graph;
class Table;

//! Abstract base class for data analysis operations
class Filter : public QObject {
  Q_OBJECT

public:
  Filter(ApplicationWindow *parent, Table *t = nullptr,
         const QString &name = QString());
  Filter(ApplicationWindow *parent, Graph *g = nullptr,
         const QString &name = QString());
  ~Filter() override;

  //! Actually does the job. Should be reimplemented in derived classes.
  virtual bool run();

  virtual void setDataCurve(int curve, double start, double end);
  bool setDataFromCurve(const QString &curveTitle, Graph *g = nullptr);
  bool setDataFromCurve(const QString &curveTitle, double from, double to,
                        Graph *g = nullptr);

  virtual bool setDataFromTable(Table *, const QString &, const QString &,
                                int = 1, int = -1);

  //! Changes the data range if the source curve was already assigned. Provided
  // for convenience.
  void setInterval(double from, double to);

  //! Sets the tolerance used by the GSL routines
  void setTolerance(double eps) { d_tolerance = eps; };

  //! Sets the color of the output fit curve.
  void setColor(int colorId) { d_curveColorIndex = colorId; };

  //! Sets the color of the output fit curve. Provided for convenience. To be
  // used in scripts only!
  void setColor(const QString &colorName);

  //! Sets the number of points in the output curve
  void setOutputPoints(int points) { d_points = points; };

  //! Sets the precision used for the output
  void setOutputPrecision(int digits) { d_prec = digits; };

  //! Sets the maximum number of iterations to be performed during an iterative
  // session
  void setMaximumIterations(int iter) { d_max_iterations = iter; };

  //! Adds a new legend to the plot. Calls virtual legendInfo()
  virtual void showLegend();

  //! Output string added to the plot as a new legend
  virtual QString legendInfo() { return QString(); };

  //! Returns the size of the input data set
  int dataSize() { return d_n; };
  //! Returns the x values of the input data set
  double *x() { return d_x; };
  //! Returns the y values of the input data set
  double *y() { return d_y; };
  //! Returns a pointer to the table created to display the results
  Table *resultTable() { return d_result_table; };

  bool error() { return d_init_err; };

  virtual void enableGraphicsDisplay(bool on = true, Graph *g = nullptr);

protected:
  void init();

  //! Sets x and y to the curve points between start and end. Memory will be
  // allocated with new double[].
  //! Returns the number of points within range == size of x and y arrays.
  virtual int curveData(QwtPlotCurve *c, double start, double end, double **x,
                        double **y);
  //! Same as curveData, but sorts the points by their x value.
  virtual int sortedCurveData(QwtPlotCurve *c, double start, double end,
                              double **x, double **y);

  int curveRange(QwtPlotCurve *c, double start, double end, int *iStart,
                 int *iEnd);

  //! Adds the result curve to the target output plot window. Creates a hidden
  // table and frees the input data from memory.
  QwtPlotCurve *addResultCurve(double *x, double *y);

  //! Performs checks and returns the index of the source data curve if OK, -1
  // otherwise
  int curveIndex(const QString &curveTitle, Graph *g);

  //! Output string added to the log pannel of the application
  virtual QString logInfo() { return QString(); };

  //! Performs the data analysis and takes care of the output
  virtual void output();

  //! Calculates the data for the output curve and store it in the X an Y
  // vectors
  virtual void calculateOutputData(double *X,
                                   double *Y){Q_UNUSED(X) Q_UNUSED(Y)};

  MultiLayer *createOutputGraph();

  //! The source graph with the curve to be analyzed
  Graph *d_graph;

  //! The graph where the result curve should be displayed
  Graph *d_output_graph;

  //! A table source of data
  Table *d_table;

  //! The table displaying the results of the filtering operation (not alvays
  // valid!)
  Table *d_result_table;

  //! Size of the data arrays
  int d_n;

  //! x data set to be analysed
  double *d_x;

  //! y data set to be analysed
  double *d_y;

  //! GSL Tolerance, if ever needed...
  double d_tolerance;

  //! Number of result points to de calculated and displayed in the output curve
  int d_points;

  //! Color index of the result curve
  int d_curveColorIndex;

  //! Maximum number of iterations per fit
  int d_max_iterations;

  //! The curve to be analysed
  QwtPlotCurve *d_curve;

  //! Precision (number of significant digits) used for the results output
  int d_prec;

  //! Error flag telling if something went wrong during the initialization
  // phase.
  bool d_init_err;

  //! Data interval
  double d_from, d_to;

  //! Specifies if the filter needs sorted data as input
  bool d_sort_data;

  //! Minimum number of data points necessary to perform the operation
  int d_min_points;

  //! String explaining the operation in the comment of the result table and in
  // the project explorer
  QString d_explanation;

  //! Specifies if the filter should display a result curve
  bool d_graphics_display;

  QString d_y_col_name;
};

#endif
