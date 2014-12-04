#ifndef GEOMETRY_WIDGET_H
#define GEOMETRY_WIDGET_H

/** This is the GUI implementation of the geometry layout for the Rebinning operations.
*  Inpects input geometry to determine possibilities for shaping the geometry via the user interface.
*  Manages DimensionWidget and IntegratedDimensionWidget types.

@author Owen Arnold Tessella/ISIS
@date January 10/2011

Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

#include <memory>
#include <boost/shared_ptr.hpp>
#include "WidgetDllOption.h"
#include "MantidGeometry/MDGeometry/MDGeometryXMLParser.h"
#include "MantidVatesAPI/GeometryView.h"
#include "MantidVatesAPI/DimensionView.h"
#include "DimensionWidgetFactory.h"
#include "DimensionWidget.h"

/// Foward decs
class QLabel;
class QComboBox;
class QLayout;
class QCheckBox;

namespace Mantid
{
  namespace Geometry
  {
    /// Forward decs
    class IMDDimension;
  }
  namespace VATES
  {
    /// Forward decs
    class GeometryPresenter;
  }
}

/**
GeometryWidget is a QWidget and a GeometryView.

View of an MVP pattern. Controlled by an Presenter, which this View owns.

- Internally, this type generates a layout onto which the presenter can command the placement of Dimensions.
- This type also owns a factory for creating DimensionViews, which the presenter will utilise.
- The view may be commanded by the presenter to raise events, so that owners of this widget may subscribe to and observe changes.

*/
// cppcheck-suppress class_X_Y
class EXPORT_OPT_MANTIDPARVIEW GeometryWidget: public QWidget, public Mantid::VATES::GeometryView
{

private:

  /// Dimension generating factory.
  DimensionWidgetFactory m_widgetFactory;

  /// MVP presenter.
  Mantid::VATES::GeometryPresenter* m_pPresenter;

  /// Checkbox for changing the bin display mode.
  QCheckBox* m_ckBinDisplay;

  Q_OBJECT
public:
  Q_PROPERTY(QString GeometryXML READ getGeometryXML WRITE setGeometryXML NOTIFY valueChanged)

   /// Constructor
   GeometryWidget(Mantid::VATES::GeometryPresenter* pPresenter, Mantid::VATES::BinDisplay binDisplay);

  /// Raise geometry modified event.
  virtual void raiseModified();

  /// Destructor
  ~GeometryWidget();

  /// Gets the chosen geometry configuration.
  QString getGeometryXML() const
  {
    return getGeometryXMLString().c_str();
  }

  /*
  Sets the geometry xml.
  @param value: xml string.
  */
  void setGeometryXML(QString value)
  {
    //Do nothing.
    UNUSED_ARG(value);
  }

  /// Add a dimension view.
  virtual void addDimensionView(Mantid::VATES::DimensionView*);

  /// Get the new geometry xml.
  virtual std::string getGeometryXMLString() const;
  
  /// Get the dimension generating factory.
  virtual const Mantid::VATES::DimensionViewFactory& getDimensionViewFactory();

  /// Getter to indicate whether the number of bins should be used, or low
  virtual Mantid::VATES::BinDisplay getBinDisplayMode() const;

  /// Single signal gets raised if anything changes
Q_SIGNALS:
  void valueChanged();
  void ignoreBinChanges();

private slots:

  // Handler for the bin mode changing.
  void binModeChanged(bool);

};

#endif
