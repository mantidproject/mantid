#ifndef DIMENSIONSLICEWIDGET_H
#define DIMENSIONSLICEWIDGET_H

#include "DllOption.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "ui_DimensionSliceWidget.h"
#include <QWidget>

namespace MantidQt {
namespace SliceViewer {

/** Widget for the 2D slice viewer. Select whether the dimension
 * is X or Y, or if it is not one to be shown, where is the
 * slice.
 * Shows the dimension name and units
 *
 * @author Janik Zikovsky
 * @date Oct 3, 2011
 */

class EXPORT_OPT_MANTIDQT_SLICEVIEWER DimensionSliceWidget : public QWidget {
  Q_OBJECT

  //  /** Enum for each shown dimension */
  //  enum eShownDim
  //  {
  //    None=-1,
  //    X=0,
  //    Y=1
  //  };

public:
  DimensionSliceWidget(QWidget *parent = nullptr);
  ~DimensionSliceWidget() override;

  void setDimension(int index, Mantid::Geometry::IMDDimension_const_sptr dim);
  void setMinMax(double min, double max);
  void setShownDim(int dim);
  void setSlicePoint(double value);

  void showRebinControls(bool show);
  bool showRebinControls() const;

  int getNumBins() const;
  void setNumBins(int val);

  double getThickness() const;
  void setThickness(double val);

  double getSlicePoint() const { return m_slicePoint; }

  std::string getDimName() const { return m_dim->getName(); }

  /// @return the shown dimension, 0=X, 1=Y, -1=None
  int getShownDim() const { return m_shownDim; }

public slots:
  void sliderMoved();
  void btnXYChanged();
  void spinBoxChanged();
  void spinThicknessChanged();
  void spinBinsChanged();

signals:
  void changedShownDim(int index, int dim, int oldDim);
  void changedSlicePoint(int index, double value);
  void changedThickness(int index, double value);
  void changedNumBins(int index, int numbins);

public:
  /// Auto-gen UI class
  Ui::DimensionSliceWidgetClass ui;

private:
  /// Sptr to the dimensions being displayed
  Mantid::Geometry::IMDDimension_const_sptr m_dim;

  /// The index of the dimension into the workspace
  int m_dimIndex;

  /// Which dimension is being shown. -1 = None, 0 = X, 1 = Y. 2+ reserved for
  /// higher dimensions
  int m_shownDim;

  /// If the dimensions is not shown, where is the slice point?
  double m_slicePoint;

  /// Show the controls for rebinning (thickness/number of bins)
  bool m_showRebinControls;
};

} // namespace SliceViewer
} // namespace MantidQt

#endif // DIMENSIONSLICEWIDGET_H
