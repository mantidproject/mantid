#ifndef DIMENSION_WIDGET_H
#define DIMENSION_WIDGET_H

#include <qgridlayout.h>
#include <qwidget.h>
#include <memory>
#include <boost/shared_ptr.hpp>
#include <vector>
#include "WidgetDllOption.h"
#include "GeometryWidget.h"

//Foward decs
class QLabel;
class QComboBox;
class QLineEdit;

namespace Mantid
{
 namespace Geometry
 {
 class IMDDimension;
 }
}


class EXPORT_OPT_MANTIDPARVIEW DimensionWidget: public QWidget
{
Q_OBJECT
public:

  /// Constructor.
  DimensionWidget(GeometryWidget* geometryWidget, const std::string& name, const int dimensionIndex,
      std::vector<boost::shared_ptr<Mantid::Geometry::IMDDimension> > nonIntegratedDimensions, DimensionLimitsOption limitsOption );

  /// Destructor
  ~DimensionWidget();

  /// Get minimum
  double getMinimum() const;

  /// Get maximum
  double getMaximum() const;

  /// Set minimum
  void setMinimum(double minimum);

  /// Set maximum
  void setMaximum(double maximum);

  /// Get the number of dimensions.
  boost::shared_ptr<Mantid::Geometry::IMDDimension>  getDimension();

  /// Get the number of bins.
  int getNBins();

  /// Get the selected index.
  int getSelectedIndex() const;

  /// Populates gui controls. May be called more than once.
  void populateWidget(const int dimensionIndex);

  /// Reset the bin values.
  void resetBins();

signals:
  void maxSet();
  void minSet();
  void nBinsSet();
private:
  QGridLayout* m_layout;

  QLineEdit* m_nBinsBox;

  QLineEdit* m_minBox;

  QLineEdit* m_maxBox;

  QComboBox* m_dimensionCombo;

  int m_currentDimensionIndex;

  std::string m_name;

  GeometryWidget* m_geometryWidget;

  std::vector<boost::shared_ptr<Mantid::Geometry::IMDDimension> > m_vecNonIntegratedDimensions;

  /// Creates gui layout and controls.
  void constructWidget(const int dimensionIndex, DimensionLimitsOption limitsOption);


private slots:

  /// Handles dimension change events.
  void dimensionSelectedListener();

  void nBinsListener();

  void maxBoxListener();

  void minBoxListener();
};

#endif
