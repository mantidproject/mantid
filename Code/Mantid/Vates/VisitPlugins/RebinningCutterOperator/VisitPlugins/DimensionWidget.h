#ifndef DIMENSION_WIDGET_H
#define DIMENSION_WIDGET_H

#include <qgridlayout.h>
#include <qwidget.h>
#include <memory>
#include "boost/shared_ptr.hpp"
#include <vector>

//Foward decs
class QLabel;
class QComboBox;
class QLineEdit;
class GeometryWidget;

namespace Mantid
{
 namespace Geometry
 {
 class IMDDimension;
 }
}


class DimensionWidget: public QWidget
{
Q_OBJECT
public:

  DimensionWidget(GeometryWidget* geometryWidget, const std::string& name, const int dimensionIndex,
      std::vector<boost::shared_ptr<Mantid::Geometry::IMDDimension> > nonIntegratedDimensions );

  ~DimensionWidget();

  double getMinimum() const;

  double getMaximum() const;

  void setMinimum(double minimum);

  void setMaximum(double maximum);

  boost::shared_ptr<Mantid::Geometry::IMDDimension>  getDimension() const;

  int getNBins() const;

  int getSelectedIndex() const;

  /// Populates gui controls. May be called more than once.
  void populateWidget(const int dimensionIndex);

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

  /// Creates gui layout and controls.
  void constructWidget(const int dimensionIndex);

  std::vector<boost::shared_ptr<Mantid::Geometry::IMDDimension> > m_vecNonIntegratedDimensions;

private slots:

  /// Handles dimension change events.
  void dimensionSelectedListener();

  void nBinsListener();

  void maxBoxListener();

  void minBoxListener();
};

#endif
