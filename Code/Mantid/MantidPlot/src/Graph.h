/***************************************************************************
    File                 : Graph.h
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief, Tilman Hoener zu Siederdissen
    Email (use @ for *)  : ion_vasilief*yahoo.fr, thzs*gmx.net
    Description          : Graph widget

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
#ifndef GRAPH_H
#define GRAPH_H

#include <QList>
#include <QPointer>
#include <QPrinter>
#include <QVector>
#include <QEvent>
#include <QSettings>
#include <QSet>

#include <qwt_text.h>
#include <qwt_plot.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_panner.h>
#include <qwt_plot_magnifier.h>

#include "Plot.h"
#include "Table.h"
#include "AxesDialog.h"
#include "PlotToolInterface.h"
#include "MultiLayer.h"
#include "ScaleDraw.h"
//#include "MantidKernel/Logger.h"
#include "GraphOptions.h"
#include <boost/shared_ptr.hpp>
#include<set>


#include <float.h>

class QwtPlotCurve;
class QwtPlotPanner;
class QwtPlotMagnifier;
class QwtPlotZoomer;
class QwtPieCurve;
class Table;
class LegendWidget;
class ArrowMarker;
class ImageMarker;
class TitlePicker;
class ScalePicker;
class CanvasPicker;
class ApplicationWindow;
class Matrix;
class SelectionMoveResizer;
class RangeSelectorTool;
class DataCurve;
class PlotCurve;
class QwtErrorPlotCurve;
class MultiLayer;
class Spectrogram;
class FunctionCurve;
class VectorCurve;
class BoxCurve;
class QwtHistogram;
class UserHelperFunction;
class QMutex;


//! Structure containing curve layout parameters
typedef struct{
  int lCol;        //!< line color
  float lWidth;    //!< line width
  int lStyle;      //!< line style
  int filledArea;  //!< flag: toggles area filling under curve
  int aCol;        //!< curve area color
  int aStyle;      //!< area filling style
  int symCol;      //!< symbol outline color
  int fillCol;     //!< symbol fill color
  float penWidth;  //!< symbol outline width
  int sSize;       //!< symbol size
  int sType;       //!< symbol type (shape)
  int connectType; //!< symbol connection type
}  CurveLayout;
namespace Mantid
{
  namespace API
  {
    class IInstrument;
    class MatrixWorkspace;
  }
}
/**
 * \brief A 2D-plotting widget.
 *
 * Graphs are managed by a MultiLayer, where they are sometimes referred to as "graphs" and sometimes as "layers".
 * Other parts of the code also call them "plots", regardless of the fact that there's also a class Plot.
 * Within the user interface, they are quite consistently called "layers".
 *
 * Each graph owns a Plot called #d_plot, which handles parts of the curve, axis and marker management (similarly to QwtPlot),
 * as well as the pickers #d_zoomer (a QwtPlotZoomer), #titlePicker (a TitlePicker), #scalePicker (a ScalePicker) and #cp (a CanvasPicker),
 * which handle various parts of the user interaction.
 *
 * Graph contains support for various curve types (see #CurveType),
 * some of them relying on Qtiplot-specific QwtPlotCurve subclasses for parts of the functionality.
 *
 * %Note that some of Graph's methods are implemented in analysis.cpp.
 *
 * \section future Future Plans
 * Merge with Plot and CanvasPicker.
 * Think about merging in TitlePicker and ScalePicker.
 * On the other hand, things like range selection, peak selection or (re)moving data points could be split out into tool classes
 * like QwtPlotZoomer or SelectionMoveResizer.
 *
 * What definitely should be split out are plot types like histograms and pie charts (TODO: which others?).
 * We need a generic framework for this in any case so that new plot types can be implemented in plugins,
 * and Graph could do with a little diet. Especially after merging in Plot and CanvasPicker.
 * [ Framework needs to support plug-ins; assigned to ion ]
 */

class Graph: public QWidget 
{
  Q_OBJECT

public:
  Graph (int x = 0, int y = 0, int width = 500, int height = 400, QWidget* parent=0, Qt::WFlags f=0);
  ~Graph();

  enum Ticks{NoTicks = 0, Out = 1, InOut = 2, In = 3};
  enum MarkerType{None = -1, Text = 0, Arrow = 1, Image = 2};
  enum CurveType{Line, Scatter, LineSymbols, VerticalBars, Area, Pie, VerticalDropLines,
                 Spline, HorizontalSteps, Histogram, HorizontalBars, VectXYXY, ErrorBars,
                 Box, VectXYAM, VerticalSteps, ColorMap, GrayScale, Contour, Function, ImagePlot,User};

  //! Returns a pointer to the parent MultiLayer object.
  MultiLayer *multiLayer(){return (MultiLayer *)(this->parent()->parent()->parent());};

  //! Change the active tool, deleting the old one if it exists.
  void setActiveTool(PlotToolInterface *tool);
  //! Return the active tool, or NULL if none is active.
  PlotToolInterface* activeTool() const { return d_active_tool; }

  Grid *grid(){return d_plot->grid();};

  QList <LegendWidget *> textsList();
  LegendWidget *selectedText(){return d_selected_text;};
  void setSelectedText(LegendWidget *l);

  void deselect();
  DataCurve* selectedCurveLabels();
  //! Used when restoring DataCurve curveID from a project file
  void restoreCurveLabels(int curveID, const QStringList& lst);
  //! Called first time we add curves in order to determine the best plot limits.
  void initScaleLimits();

  //! Returns true if a plot/data tool is enabled.
  bool hasActiveTool();

  ///seeting a boolean flag to when the intensity(start and end values) changed
  void changeIntensity(bool bIntensityChanged);

 // This method zooms the selected grpah
  void enablePanningMagnifier(bool on = true);
	
public slots:
  //! Accessor method for #d_plot.
  Plot* plotWidget(){return d_plot;};
  void copy(Graph* g);

  //! \name Pie Curves
  //@{
  //! Returns true if this Graph is a pie plot, false otherwise.
  bool isPiePlot(){return (c_type.count() == 1 && c_type[0] == Pie);};
  //! Used when creating a pie plot.
  QwtPieCurve* plotPie(Table* w,const QString& name, int startRow = 0, int endRow = -1);
  //! Used when restoring a pie plot from a project file.
  QwtPieCurve* plotPie(Table* w, const QString& name, const QPen& pen, int brush, int size,
                       int firstColor, int startRow = 0, int endRow = -1, bool visible = true,
                       double d_start_azimuth = 270, double d_view_angle = 90, double d_thickness = 33,
                       double d_horizontal_offset = 0.0, double d_edge_dist = 25, bool d_counter_clockwise = false,
                       bool d_auto_labeling = true, bool d_values = false, bool d_percentages = true,
                       bool d_categories = false, bool d_fixed_labels_pos = true);

  void removePie();
  QString pieLegendText();
  QString savePieCurveLayout();
  //@}

  bool addCurves(Table* w, const QStringList& names, int style = 0, double lWidth = 1, int sSize = 3, int startRow = 0, int endRow = -1);
  PlotCurve* insertCurve(Table* w, const QString& name, int style, int startRow = 0, int endRow = -1);
  PlotCurve* insertCurve(Table* w, int xcol, const QString& name, int style);
  PlotCurve* insertCurve(Table* w, const QString& xColName, const QString& yColName, int style, int startRow = 0, int endRow = -1);
  PlotCurve* insertCurve(PlotCurve* c, int lineWidth = -1, int curveType = User);
  void insertPlotItem(QwtPlotItem *i, int type);

  void insertCurve(Graph* g, int i);

  //! Shows/Hides a curve defined by its index.
  void showCurve(int index, bool visible = true);
  int visibleCurves();

  //! Removes a curve defined by its index.
  void removeCurve(int index);
  /**
   * \brief Removes a curve defined by its title string s.
   */
  void removeCurve(const QString& s);
  /**
   * \brief Removes all curves defined by the title/plot association string s.
   */
  void removeCurves(const QString& s);
  void removeCurve(PlotCurve* c);

  void updateCurvesData(Table* w, const QString& yColName);

  int curves(){return n_curves;};
  bool validCurvesDataSize();
  double selectedXStartValue();
  double selectedXEndValue();

  long curveKey(int curve){return c_keys[curve];}
  int curveIndex(long key){return c_keys.indexOf(key);};
  //! Map curve pointer to index.
  int curveIndex(QwtPlotCurve *c) const;
  //! map curve title to index
  int curveIndex(const QString &title){return plotItemsList().findIndex(title);}
  //! get curve by index
  QwtPlotCurve* curve(int index);
  //! get curve by name
  QwtPlotCurve* curve(const QString &title){return curve(curveIndex(title));}

  //! Returns the names of all the curves suitable for data analysis, as a string list. The list excludes error bars and spectrograms.
  QStringList analysableCurvesList();
  //! Returns the names of all the QwtPlotCurve items on the plot, as a string list
  QStringList curvesList();
  //! Returns the names of all plot items, including spectrograms, as a string list
  QStringList plotItemsList();
  //! get plotted item by index
  QwtPlotItem* plotItem(int index);
  //! get plot item by index
  int plotItemIndex(QwtPlotItem *it) const;

  void updateCurveNames(const QString& oldName, const QString& newName, bool updateTableName = true);

  int curveType(int curveIndex);
  void setCurveType(int curve, int style);
  void setCurveFullRange(int curveIndex);

  //! \name Output: Copy/Export/Print
  //@{
  void print();
  void setScaleOnPrint(bool on){d_scale_on_print = on;};
  void printCropmarks(bool on){d_print_cropmarks = on;};

  void copyImage();
  QPixmap graphPixmap();
  //! Provided for convenience in scripts
  void exportToFile(const QString& fileName);
  void exportSVG(const QString& fname);
#ifdef EMF_OUTPUT
  void exportEMF(const QString& fname);
#endif
  void exportVector(const QString& fileName, int res = 0, bool color = true,
                    bool keepAspect = true, QPrinter::PageSize pageSize = QPrinter::Custom);
  void exportImage(const QString& fileName, int quality = 100, bool transparent = false);
  //@}

  void replot(){d_plot->replot();};
  void updatePlot();

  //! \name Error Bars
  //@{
  QwtErrorPlotCurve* addErrorBars(const QString& xColName, const QString& yColName, Table *errTable,
                                  const QString& errColName, int type = 1, double width = 1, int cap = 8, const QColor& color = QColor(Qt::black),
                                  bool through = true, bool minus = true, bool plus = true);

  QwtErrorPlotCurve* addErrorBars(const QString& yColName, Table *errTable, const QString& errColName,
                                  int type = 1, double width = 1, int cap = 8, const QColor& color = QColor(Qt::black),
                                  bool through = true, bool minus = true, bool plus = true);

  /// Adds the errors to an existing MantidCurve
  void addMantidErrorBars(const QString& curveName,bool drawAll);
  /// Removes the errors off an existing MantidCurve
  void removeMantidErrorBars(const QString& curveName);

  void updateErrorBars(QwtErrorPlotCurve *er, bool xErr, double width, int cap, const QColor& c, bool plus, bool minus, bool through);

  //! Returns a valid master curve for the error bars curve.
  DataCurve* masterCurve(QwtErrorPlotCurve *er);
  //! Returns a valid master curve for a plot association.
  DataCurve* masterCurve(const QString& xColName, const QString& yColName);
  //@}

  //! \name Event Handlers
  //@{
  void contextMenuEvent(QContextMenuEvent *);
  void closeEvent(QCloseEvent *e);
  bool focusNextPrevChild ( bool next );
  void hideEvent(QHideEvent* e);
  //@}

  //! Set axis scale
  void invertScale(int axis);
  void setScale(int axis, double start, double end, double step = 0.0,
                int majorTicks = 5, int minorTicks = 5, int type = 0, bool inverted = false,
                double left_break = -DBL_MAX, double right_break = DBL_MAX, int pos = 50,
                double stepBeforeBreak = 0.0, double stepAfterBreak = 0.0, int minTicksBeforeBreak = 4,
                int minTicksAfterBreak = 4, bool log10AfterBreak = false, int breakWidth = 4, bool breakDecoration = true);
  void setScale(QwtPlot::Axis axis, QwtScaleTransformation::Type scaleType);
  void setScale(QwtPlot::Axis axis, QString logOrLin);
  double axisStep(int axis){return d_user_step[axis];};
  //! Set the axis scale
  void setAxisScale(int axis, double start, double end, int type = -1, double step = 0.0,
                    int majorTicks = 5, int minorTicks = 5);
  
	/// in plot windows change both axis to log-log
  void logLogAxes();
  /// the plot's x-axis will be log and y-axis linear
  void logXLinY();
  /// the plot's y-axis will be log and x-axis linear
  void logYlinX();
  /// set both the x and y axes to linear
  void linearAxes();
  /// set a log scale for the color bar
  void logColor();
  /// change the color bar to use a linear scale
  void linColor();
  //! \name Curves Layout
  //@{
  CurveLayout initCurveLayout(int style, int curves = 0);
  static CurveLayout initCurveLayout();
  void updateCurveLayout(PlotCurve* c, const CurveLayout *cL);
  //! Tries to guess not already used curve color and symbol style
  void guessUniqueCurveLayout(int& colorIndex, int& symbolIndex);
  //@}

  //! \name Zoom
  //@{
  void zoomed (const QwtDoubleRect &);
  void zoom(bool on);
  void zoomOut();
  bool zoomOn();
  //@}

  void setAutoScale();
  void updateScale();
  // Set axis that will not be autoscaled
  void setFixedScale(int axis);

  //! \name Saving to File
  //@{
  QString saveToString(bool saveAsTemplate = false);
  QString saveScale();
  QString saveScaleTitles();
  QString saveFonts();
  QString saveMarkers();
  QString saveCurveLayout(int index);
  QString saveAxesTitleColors();
  QString saveAxesColors();
  QString saveEnabledAxes();
  QString saveCanvas();
  QString saveTitle();
  QString saveAxesTitleAlignement();
  QString saveEnabledTickLabels();
  QString saveTicksType();
  QString saveCurves();
  QString saveLabelsFormat();
  QString saveLabelsRotation();
  QString saveAxesLabelsType();
  QString saveAxesBaseline();
  QString saveAxesFormulas();
  //@}

  //! \name Text Markers
  //@{
  void drawText(bool on);
  bool drawTextActive(){return drawTextOn;};
  LegendWidget* insertText(LegendWidget*);

  //! Used when opening a project file
  LegendWidget* insertText(const QStringList& list, int fileVersion);

  void addTimeStamp();
  void removeLegend();
  void removeLegendItem(int index);
  void insertLegend(const QStringList& lst, int fileVersion);

  LegendWidget *legend(){return d_legend;};
  LegendWidget* newLegend(const QString& text = QString());

  //! Creates a new legend text using the curves titles
  QString legendText();
  //@}

  //! \name Line Markers
  //@{
  ArrowMarker* arrow(long id);
  ArrowMarker* addArrow(ArrowMarker* mrk);

  //! Used when opening a project file
  void addArrow(QStringList list, int fileVersion);
  QVector<int> lineMarkerKeys(){return d_lines;};

  //!Draws a line/arrow depending on the value of "arrow"
  void drawLine(bool on, bool arrow = FALSE);
  bool drawArrow(){return drawArrowOn;};
  bool drawLineActive(){return drawLineOn;};

  bool arrowMarkerSelected();
  //@}

  //! \name Image Markers
  //@{
  ImageMarker* imageMarker(long id);
  QVector<int> imageMarkerKeys(){return d_images;};
  ImageMarker* addImage(ImageMarker* mrk);
  ImageMarker* addImage(const QString& fileName);

  void insertImageMarker(const QStringList& lst, int fileVersion);
  bool imageMarkerSelected();
  void updateImageMarker(int x, int y, int width, int height);
  //@}

  //! \name Common to all Markers
  //@{
  void removeMarker();
  //! Keep the markers on screen each time the scales are modified by adding/removing curves
  void updateMarkersBoundingRect();

  long selectedMarkerKey();
  /**\brief Set the selected marker.
   * @param mrk :: key of the marker to be selected.
   * @param add :: whether the marker is to be added to an existing selection.
   * If <i>add</i> is false (the default) or there is no existing selection, a new SelectionMoveResizer is
   * created and stored in #d_markers_selector.
   */
  void setSelectedMarker(long mrk, bool add=false);
  QwtPlotMarker* selectedMarkerPtr();
  bool markerSelected();
  //! Reset any selection states on markers.
  void deselectMarker();
  //@}

  //! \name Axes
  //@{
  QwtScaleWidget* currentScale();
  QwtScaleWidget* selectedScale();
  QRect axisTitleRect(const QwtScaleWidget *scale);

  ScaleDraw::ScaleType axisType(int axis);

  void setXAxisTitle(const QString& text);
  void setYAxisTitle(const QString& text);
  void setRightAxisTitle(const QString& text);
  void setTopAxisTitle(const QString& text);

  QString axisTitle(int axis){return d_plot->axisTitle(axis).text();};
  void setAxisTitle(int axis, const QString& text);
  //! TODO: eliminate this function in version 0.9.1 (used only when restoring project files)
  void setScaleTitle(int axis, const QString& text);

  QFont axisTitleFont(int axis);
  void setAxisTitleFont(int axis,const QFont &fnt);

  void setAxisFont(int axis, const QFont &fnt);
  QFont axisFont(int axis);
  void initFonts(const QFont &scaleTitleFnt,const QFont &numbersFnt);

  QColor axisTitleColor(int axis);
  void setAxisTitleColor(int axis, const QColor& c);

  int axisTitleAlignment (int axis);
  void setAxisTitleAlignment(int axis, int align);

  QColor axisColor(int axis);
  void setAxisColor(int axis, const QColor& color);

  QColor axisLabelsColor(int axis);
  void setAxisLabelsColor(int axis, const QColor& color);

  void showAxis(int axis, int type, const QString& formatInfo, Table *table, bool axisOn,
                int majTicksType, int minTicksType, bool labelsOn, const QColor& c, int format,
                int prec, int rotation, int baselineDist, const QString& formula, const QColor& labelsColor);

  void enableAxis(int axis, bool on = true);
  void enableAxisLabels(int axis, bool on = true);

  int labelsRotation(int axis);
  void setAxisLabelRotation(int axis, int rotation);

  void setAxesLinewidth(int width);
  //! used when opening a project file
  void loadAxesLinewidth(int width);

  void drawAxesBackbones(bool yes);
  bool axesBackbones(){return drawAxesBackbone;};
  //! used when opening a project file
  void loadAxesOptions(const QString& s);

  void setAxisMargin(int axis, int margin);

  bool isColorBarEnabled(int axis) const;
  bool isLog(const QwtPlot::Axis axis) const;

  void setMajorTicksType(const QList<int>& lst);
  void setMajorTicksType(const QStringList& lst);

  void setMinorTicksType(const QList<int>& lst);
  void setMinorTicksType(const QStringList& lst);

  int minorTickLength();
  int majorTickLength();
  void setAxisTicksLength(int axis, int majTicksType, int minTicksType, int minLength, int majLength);
  void setTicksLength(int minLength, int majLength);
  void changeTicksLength(int minLength, int majLength);
  //! Used for restoring project files
  void setLabelsNumericFormat(const QStringList& l);
  void setLabelsNumericFormat(int axis, int format, int prec = 6, const QString& formula = QString());
  void setLabelsDateTimeFormat(int axis, int type, const QString& formatInfo);
  void setLabelsDayFormat(int axis, int format);
  void setLabelsMonthFormat(int axis, int format);

  QString axisFormatInfo(int axis);

  void setLabelsTextFormat(int axis, int type, const QString& name, const QStringList& lst);
  void setLabelsTextFormat(int axis, int type, const QString& labelsColName, Table *table);

  QString axisFormula(int axis);
  void setAxisFormula(int axis, const QString &);
  //@}

  //! \name Canvas Frame
  //@{
  void setCanvasFrame(int width = 1, const QColor& color =  QColor(Qt::black));
  QColor canvasFrameColor();
  int canvasFrameWidth();
  //@}

  //! \name Plot Title
  //@{
  void setTitle(const QString& t);
  void setTitleFont(const QFont &fnt);
  void setTitleColor(const QColor &c);
  void setTitleAlignment(int align);

  bool titleSelected();
  void selectTitle(bool select = true);

  void removeTitle();
  void initTitle( bool on, const QFont& fnt);

  void setCurveTitle(int index, const QString & title);
  //@}

  //! \name Modifing insertCurve Data
  //@{
  int selectedCurveID();
  int selectedCurveIndex() { return curveIndex(selectedCurveID()); }
  QString selectedCurveTitle();
  //@}

  void disableTools();

  /** Enables the data range selector tool.
   *
   * This one is a bit special, because other tools can depend upon an existing selection.
   * Therefore, range selection (like zooming) has to be provided in addition to the generic
   * tool interface.
   */
  bool enableRangeSelectors(const QObject *status_target=NULL, const char *status_slot="");
  /// Check if the gange selectors are active
  bool areRangeSelectorsOn()const{return !d_range_selector.isNull();}

  //! \name Border and Margin
  //@{
  void setMargin (int d);
  void setFrame(int width = 1, const QColor& color = QColor(Qt::black));
  void setBackgroundColor(const QColor& color);
  void setCanvasBackground(const QColor& color);
  //@}

  void addFitCurve(QwtPlotCurve *c);
  void deleteFitCurves();
  QList<QwtPlotCurve *> fitCurvesList(){return d_fit_curves;};
  /** Set start and end to selected X range of curve index or, if there's no selection, to the curve's total range.
   *
   * @return the number of selected or total points
   */
  int range(int index, double *start, double *end);

  //!  Used for VerticalBars, HorizontalBars and Histograms
  void setBarsGap(int curve, int gapPercent, int offset);

  //! \name User-defined Functions
  //@{
  void modifyFunctionCurve(int curve, int type, const QStringList &formulas, const QString &var, double start, double end, int points);
  FunctionCurve* addFunction(const QStringList &formulas, double start, double end, int points = 100, const QString &var = "x", int type = 0, const QString& title = QString::null);
  //! Used when reading from a project file with version < 0.9.5.
  FunctionCurve* insertFunctionCurve(const QString& formula, int points, int fileVersion);
  //! Used when reading from a project file with version >= 0.9.5.
  void restoreFunction(const QStringList& lst);

  //! Returns an unique function name
  QString generateFunctionName(const QString& name = tr("F"));
  //@}

  //! Provided for convenience in scripts.
  void createTable(const QString& curveName);
  void createTable(const QwtPlotCurve* curve);
  void activateGraph();

  //! \name Vector Curves
  //@{
  VectorCurve* plotVectorCurve(Table* w, const QStringList& colList, int style, int startRow = 0, int endRow = -1);
  void updateVectorsLayout(int curve, const QColor& color, double width, int arrowLength, int arrowAngle, bool filled, int position,
                           const QString& xEndColName = QString(), const QString& yEndColName = QString());
  //@}

  //! \name Box Plots
  //@{
  BoxCurve* openBoxDiagram(Table *w, const QStringList& l, int fileVersion);
  void plotBoxDiagram(Table *w, const QStringList& names, int startRow = 0, int endRow = -1);
  //@}

  void setCurveSymbol(int index, const QwtSymbol& s);
  void setCurvePen(int index, const QPen& p);
  void setCurveBrush(int index, const QBrush& b);
  void setCurveStyle(int index, int s);

  //! \name Resizing
  //@{
  bool ignoresResizeEvents(){return ignoreResize;};
  void setIgnoreResizeEvents(bool ok){ignoreResize=ok;};
  void resizeEvent(QResizeEvent *e);
  void scaleFonts(double factor);
  //@}

  void notifyChanges();

  void updateSecondaryAxis(int axis);
  void enableAutoscaling(bool yes){d_auto_scale = yes;};

  bool autoscaleFonts(){return autoScaleFonts;};
  void setAutoscaleFonts(bool yes){autoScaleFonts = yes;};

  static int obsoleteSymbolStyle(int type);
  static QString penStyleName(Qt::PenStyle style);
  static Qt::PenStyle getPenStyle(const QString& s);
  static Qt::PenStyle getPenStyle(int style);
  static void showPlotErrorMessage(QWidget *parent, const QStringList& emptyColumns);
  static QPrinter::PageSize minPageSize(const QPrinter& printer, const QRect& r);

  void showTitleContextMenu();
  void copyTitle();
  void cutTitle();

  void removeAxisTitle();
  void cutAxisTitle();
  void copyAxisTitle();
  void showAxisTitleMenu();
  void showAxisContextMenu(int axis);
  void hideSelectedAxis();
  void showGrids();

  //! Convenience function enabling the grid for QwtScaleDraw::Left and Bottom Scales
  void showGrid();
  //! Convenience function enabling the grid for a user defined axis
  void showGrid(int axis);

  void showAxisDialog();
  void showScaleDialog();

  //! Add a spectrogram to the graph
  Spectrogram* plotSpectrogram(Matrix *m, CurveType type);
  Spectrogram* plotSpectrogram(UserHelperFunction *f,int nrows, int ncols,double left, double top, double width, double height,double minz,double maxz, CurveType type);//Mantid
  Spectrogram* plotSpectrogram(UserHelperFunction *f,int nrows, int ncols,QwtDoubleRect bRect,double minz,double maxz,CurveType type);//Mantid
  // Spectrogram* plotSpectrogram(UserHelperFunction *f,int nrows, int ncols,QwtDoubleRect bRect,double minz,double maxz,CurveType type);//Mantid
  Spectrogram* plotSpectrogram(Spectrogram *d_spectrogram, CurveType type);//Mantid
  //! Restores a spectrogram. Used when opening a project file.
  void restoreSpectrogram(ApplicationWindow *app, const QStringList& lst);
  //! Add a matrix histogram  to the graph
  QwtHistogram* addHistogram(Matrix *m);
  //! Restores a histogram from a project file.
  QwtHistogram* restoreHistogram(Matrix *m, const QStringList& l);

  bool antialiasing(){return d_antialiasing;};
  //! Enables/Disables antialiasing of plot items.
  void setAntialiasing(bool on = true, bool update = true);

  void setCurrentFont(const QFont& f);
  void notifyFontChange(const QFont& f){emit currentFontChanged(f);};
  void enableTextEditor();

signals:
  void selectedGraph (Graph*);
  void closedGraph();
  void drawTextOff();
  void drawLineEnded(bool);
  void cursorInfo(const QString&);
  void showPlotDialog(int);
  void createTable(const QString&,int,int,const QString&);

  void viewImageDialog();
  void viewTextDialog();
  void viewLineDialog();
  void viewTitleDialog();
  void modifiedGraph();
  void hiddenPlot(QWidget*);

  void showContextMenu();
  void showCurveContextMenu(int);
  void showMarkerPopupMenu();

  void showAxisDialog(int);
  void axisDblClicked(int);

  void showAxisTitleDialog();

  void dataRangeChanged();
  void showFitResults(const QString&);
  void currentFontChanged(const QFont&);
  void enableTextEditor(Graph *);
  void curveRemoved();

  /// sent to indicate that scale type changed to log (bool arg is true) or to linear (boolarg is false)
  /// int argument gives the axis as defined in QwtPlot::Axis
  void axisScaleChanged(int,bool);
  

private:
  //! Finds bounding interval of the plot data.
  QwtDoubleInterval axisBoundingInterval(int axis);
  void niceLogScales(QwtPlot::Axis axis);
  void deselectCurves();
  void addLegendItem();
  
	
  Plot *d_plot;
  QwtPlotZoomer *d_zoomer[2];
  TitlePicker *titlePicker;
  ScalePicker *scalePicker;
  CanvasPicker* cp;


  //! List storing pointers to the curves resulting after a fit session, in case the user wants to delete them later on.
  QList<QwtPlotCurve *>d_fit_curves;
  //! Render hint for plot items.
  bool d_antialiasing;
  bool autoScaleFonts;
  bool d_scale_on_print, d_print_cropmarks;

  //! Stores the step the user specified for the four scale. If step = 0.0, the step will be calculated automatically by the Qwt scale engine.
  QVector<double> d_user_step;
  //! Curve types
  QVector<int> c_type;
  //! Curves on plot keys
  QVector<int> c_keys;
  //! Arrows/lines on plot keys
  QVector<int> d_lines;
  //! Images on plot keys
  QVector<int> d_images;

  int n_curves, widthLine;
  long selectedMarker;
  bool drawTextOn, drawLineOn, drawArrowOn, ignoreResize, drawAxesBackbone;

  //! The markers selected for move/resize operations or NULL if none are selected.
  QPointer<SelectionMoveResizer> d_markers_selector;
  //! The current curve selection, or NULL if none is active.
  QPointer<RangeSelectorTool> d_range_selector;
  //! The currently active tool, or NULL for default (pointer).
  PlotToolInterface *d_active_tool,*d_peak_fit_tool;
  //! Pointer to the currently selected text/legend
  LegendWidget *d_selected_text;
  //! Pointer to the current legend
  LegendWidget *d_legend;
  //! Flag indicating if the axes limits should be changed in order to show all data each time a curva data change occurs
  bool d_auto_scale;
  //static Mantid::Kernel::Logger &g_log;
  QString mCurrentColorMap;
  QwtPlotMagnifier *d_magnifier;
  QwtPlotPanner *d_panner;
  //for saving the spectrogram axes number if the axes details like scale is changed
  //this is useful for saving/loading project.
  std::vector<int> updatedaxis;

  QMutex *m_mutex;
  /// The set of workpace-index pairs plotted in this graph
  QMultiMap<QString,int> m_wsspectrumMap;
  // to save error flag to project file for 1 PD plot
  bool m_errors;
  // to keep fixed axes
  QSet<int> m_fixed_axes;
};
#endif // GRAPH_H
