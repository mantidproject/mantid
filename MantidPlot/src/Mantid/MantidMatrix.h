// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDMATRIX_H
#define MANTIDMATRIX_H

#include <map>
#include <math.h>
#include <string>

#include "MantidMatrixExtensionRequest.h"
#include "MantidMatrixModel.h"
#include "MantidMatrixTabExtension.h"

#include "../ContourLinesEditor.h"
#include "../Graph.h"
#include "../MdiSubWindow.h"
#include "../UserFunction.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtWidgets/Common/WorkspaceObserver.h"

#include <Poco/NObserver.h>

#include <QAction>
#include <QHeaderView>
#include <QMap>
#include <QMessageBox>
#include <QPointer>
#include <QPrinter>
#include <QTableView>
#include <QThread>
#include <QVector>

#include <qwt_color_map.h>
#include <qwt_double_rect.h>

class QLabel;
class QStackedWidget;
class QShortcut;
class MantidMatrix;
class ApplicationWindow;
class Graph3D;
class MultiLayer;
class QTabWidget;
class UpdateDAEThread;
class ProjectData;

/**
 * Find the minimum and maximum Y values in a matrix workspace.
 *
 * @param ws :: A matrix workspace.
 * @param miny :: Variable to receive the minimum value.
 * @param maxy :: Variable to receive the maximum value.
 */
void findYRange(Mantid::API::MatrixWorkspace_const_sptr ws, double &miny,
                double &maxy);

/** MantidMatrix is the class that represents a Qtiplot window for displaying
workspaces.
It has separate tabs for displaying spectrum values, bin boundaries, and errors.

@author Roman Tolchenov, Tessella Support Services plc

*/
class MantidMatrix : public MdiSubWindow, MantidQt::API::WorkspaceObserver {
  Q_OBJECT

public:
  MantidMatrix(Mantid::API::MatrixWorkspace_const_sptr ws, QWidget *parent,
               const QString &label, const QString &name = QString(),
               int start = -1, int end = -1);

  void connectTableView(QTableView *, MantidMatrixModel *);
  MantidMatrixModel *model() { return m_modelY; }
  MantidMatrixModel *modelY() { return m_modelY; }
  MantidMatrixModel *modelX() { return m_modelX; }
  MantidMatrixModel *modelE() { return m_modelE; }
  QItemSelectionModel *selectionModel() {
    return m_table_viewY->selectionModel();
  }
  QItemSelectionModel *selectionModelY() {
    return m_table_viewY->selectionModel();
  }
  QItemSelectionModel *selectionModelX() {
    return m_table_viewX->selectionModel();
  }
  QItemSelectionModel *selectionModelE() {
    return m_table_viewE->selectionModel();
  }

  /// Get the window type as a string
  std::string getWindowType() override { return "Workspace"; }

  int numRows() const { return m_rows; }
  int numCols() const { return m_cols; }
  double dataX(int row, int col) const;
  double dataY(int row, int col) const;
  double dataE(int row, int col) const;
  double dataDx(int row, int col) const;
  int indexX(int row, double s) const;
  int indexY(double s) const;

  Mantid::API::MatrixWorkspace_const_sptr workspace() { return m_workspace; }
  QString workspaceName() const;

  QPixmap matrixIcon() { return m_matrix_icon; }
  // void copy(Matrix *m);
  ApplicationWindow *appWindow() { return m_appWindow; }
  Graph3D *plotGraph3D(int style);

  // Creates a MultiLayer graph and plots this MantidMatrix as a Spectrogram
  MultiLayer *plotGraph2D(GraphOptions::CurveType type);

  void setBinGraph(MultiLayer *ml, Table *t = nullptr);

  bool setSelectedRows();
  bool setSelectedColumns();
  const QList<int> &getSelectedRows() const;
  const QList<int> &getSelectedColumns() const;

  int workspaceIndex(int row) { return row + m_startRow; }
  bool yShown() { return m_tabs->currentIndex() == 0; }
  QTableView *activeView();
  MantidMatrixModel *activeModel();

  bool isHistogram() { return m_histogram; }

  // Set format and precision of displayed numbers.
  void setNumberFormat(const QChar &f, int prec, bool all = false);
  void setNumberFormat(int i, const QChar &f, int prec, bool all = false);

  // Return number format of the active model
  QChar numberFormat();

  // Return number precision of the active model
  int precision();

  // Loading and saving projects
  static MantidQt::API::IProjectSerialisable *
  loadFromProject(const std::string &lines, ApplicationWindow *app,
                  const int fileVersion);
  std::string saveToProject(ApplicationWindow *app) override;
  /// Returns a list of workspace names that are used by this window
  std::vector<std::string> getWorkspaceNames() override;

  /// returns the workspace name
  const std::string &getWorkspaceName();

  Spectrogram *plotSpectrogram(Graph *plot, ApplicationWindow *app,
                               GraphOptions::CurveType type, bool project,
                               const ProjectData *const prjdata);
  /// Add a multilayer as a dependent mdi sub-window. This method is addeed to
  /// fix a crash (ticket #5732).
  /// A better solution is needed
  void attachMultilayer(MultiLayer *);

  void afterReplaceHandle(
      const std::string &wsName,
      const boost::shared_ptr<Mantid::API::Workspace> ws) override;
  void
  preDeleteHandle(const std::string &wsName,
                  const boost::shared_ptr<Mantid::API::Workspace> ws) override;
  void clearADSHandle() override;

signals:
  void needWorkspaceChange(Mantid::API::MatrixWorkspace_sptr ws);
  void needToClose();
  void needsUpdating();

public slots:

  void changeWorkspace(Mantid::API::MatrixWorkspace_sptr ws);
  void closeMatrix();

  //! Return the width of all columns
  int columnsWidth(int i = -1);
  //! Set the width of all columns for all views (all==true) or the active view
  //(all==false)
  void setColumnsWidth(int width, bool all = true);
  // Set the width of column in view i (0 - Y, 1 - X, 2 - E)
  void setColumnsWidth(int i, int width);

  //! Return the content of the cell as a string
  QString text(int row, int col);
  //! Return the value of the cell as a double
  double cell(int row, int col);

  //! Returns the X value corresponding to column 1
  double xStart() { return x_start; }
  //! Returns the X value corresponding to the last column
  double xEnd() { return x_end; }
  //! Returns the Y value corresponding to row 1
  double yStart() { return y_start; }
  //! Returns the Y value corresponding to the last row
  double yEnd() { return y_end; }

  //! Returns the step of the X axis
  double dx() { return fabs(x_end - x_start) / ((double)(numCols()) - 1.0); }
  //! Returns the step of the Y axis
  double dy() { return fabs(y_end - y_start) / ((double)(numRows()) - 1.0); }

  //! Returns the bounding rect of the matrix coordinates
  QwtDoubleRect boundingRect();
  /// Invalidates the bounding rect forcing it to be recalculated
  void invalidateBoundingRect() { m_boundingRect = QwtDoubleRect(); }

  //! Min and max values in the matrix.
  void range(double *min, double *max);
  //! Set min and max values in the matrix.
  void setRange(double min, double max);

  // Scroll to row and column
  void goTo(int row, int col);
  //! Scroll to row (row starts with 1)
  void goToRow(int row);
  //! Scroll to column (column starts with 1)
  void goToColumn(int col);
  // Set the active tab by name
  void goToTab(const QString &name);

  void copySelection();

  //! Allocate memory for a matrix buffer
  static double **allocateMatrixData(int rows, int columns);
  //! Free memory used for a matrix buffer
  static void freeMatrixData(double **data, int rows);

  int verticalHeaderWidth() { return m_table_viewY->verticalHeader()->width(); }

  void dependantClosed(MdiSubWindow *w);
  void selfClosed(MdiSubWindow *w);
  void repaintAll();
  void closeDependants();
  // for context menu filtering
  bool eventFilter(QObject *object, QEvent *e) override;
  // to synchronize the views
  void viewChanged(int);

  // Opens modified QtiPlot's MatrixDialog and sets column width and number
  // format
  void setMatrixProperties();

protected:
  void setup(Mantid::API::MatrixWorkspace_const_sptr ws, int start = -1,
             int end = -1);

  ApplicationWindow *m_appWindow;
  Mantid::API::MatrixWorkspace_const_sptr m_workspace;
  QTabWidget *m_tabs;
  QTableView *m_table_viewY;
  QTableView *m_table_viewX;
  QTableView *m_table_viewE;
  QPointer<MantidMatrixModel> m_modelY;
  QPointer<MantidMatrixModel> m_modelX;
  QPointer<MantidMatrixModel> m_modelE;
  QColor m_bk_color;
  QPixmap m_matrix_icon;
  double x_start, //!< X value corresponding to column 1
      x_end,      //!< X value corresponding to the last column
      y_start,    //!< Y value corresponding to row 1
      y_end;      //!< Y value corresponding to the last row
  int m_rows, m_cols;
  int m_startRow;
  int m_endRow;
  int m_workspaceTotalHist;
  bool m_histogram;
  double m_min;           // Saved minimum Y-value
  double m_max;           // Saved maximum Y-value
  bool m_are_min_max_set; // If true ::range does not iterate over WS to find
                          // min and max but uses m_min and m_max instead
  QwtDoubleRect m_boundingRect; // The bounding box in x and y coordinates used
                                // in spectrogram drawing
  int m_spectrogramRows; // Number of rows in the spectrogram created from this
                         // matris
  int m_spectrogramCols; // Number of columns in the spectrogram created from
                         // this matris

  // MDI windows created by this MantidMatrix
  QVector<MultiLayer *> m_plots2D;
  QMap<MultiLayer *, Table *> m_plots1D;

  // MantidMatrixFunction m_funct;
  int m_column_width;

  // MantidMatrixExtensionCollection
  MantidMatrixTabExtensionMap m_extensions;

private:
  // name of the underlying workspace
  std::string m_strName;

  /// Storage for column and row selection
  QList<int> m_selectedRows, m_selectedCols;

  // The tab labels
  QString m_YTabLabel, m_XTabLabel, m_ETabLabel;
  // index to identify the previous view on tab switch
  int m_PrevIndex;

  QString m_colormapName;

  /// Add a MantidMatrixExtension
  void addMantidMatrixTabExtension(MantidMatrixModel::Type type);
  /// Hook up a MantidMatrixExtension
  void setupNewExtension(MantidMatrixModel::Type type);
  /// Update the existing extensions
  void updateExtensions(Mantid::API::MatrixWorkspace_sptr ws);

  /// ExtensioRequest handler
  MantidMatrixExtensionRequest m_extensionRequest;

  friend class MantidMatrixFunction;
};

/// Typedef for a shared pointer to a MantidMatrix
using MantidMatrix_sptr = QSharedPointer<MantidMatrix>;

class ProjectData {
public:
  ProjectData()
      : m_grayScale(0), m_intensityChanged(0), m_contourMode(0),
        m_contourLevels(0), m_customPen(0), m_contourLabels(0),
        m_colormapPen(0), m_ContourLinesEditor(nullptr) {}
  ~ProjectData() {}
  bool getGrayScale() const { return m_grayScale; }
  bool getIntensity() const { return m_intensityChanged; }
  bool getContourMode() const { return m_contourMode; }
  const QString &getColormapFile() const { return m_colormapFile; }
  void setGrayScale(bool grayscale) { m_grayScale = grayscale; }
  void setIntensity(bool intensity) { m_intensityChanged = intensity; }
  void setColormapFile(const QString &fileName) { m_colormapFile = fileName; }
  void setContourMode(bool contourmode) { m_contourMode = contourmode; }
  void setContourLevels(int levels) { m_contourLevels = levels; }
  int getContourLevels() const { return m_contourLevels; }
  void setDefaultContourPen(const QPen &defaultpen) {
    m_defaultPen = defaultpen;
  }
  QPen getDefaultContourPen() const { return m_defaultPen; }
  void setColorMapPen(bool colormappen) { m_colormapPen = colormappen; }
  bool getColorMapPen() const { return m_colormapPen; }
  void setCustomPen(bool custompen) { m_customPen = custompen; }
  bool getcustomPen() const { return m_customPen; }
  void setContourLineLabels(bool contourlabels) {
    m_contourLabels = contourlabels;
  }
  bool getContourLineLabels() const { return m_contourLabels; }
  void setCotntourLinesEditor(ContourLinesEditor *ceditor) {
    m_ContourLinesEditor = ceditor;
  }
  ContourLinesEditor *getContourLinesEditor() const {
    return m_ContourLinesEditor;
  }

private:
  bool m_grayScale;
  bool m_intensityChanged;
  bool m_contourMode;
  QString m_colormapFile;
  int m_contourLevels;
  QPen m_defaultPen;
  bool m_customPen;
  bool m_contourLabels;
  bool m_colormapPen;
  ContourLinesEditor *m_ContourLinesEditor;
};

#endif
