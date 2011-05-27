#ifndef GEOMETRY_WIDGET_H
#define GEOMETRY_WIDGET_H

/** This is the GUI implementation of the geometry layout for the Rebinning operations.
 *  Inpects input geometry to determine possibilities for shaping the geometry via the user interface.
 *  Manages DimensionWidget and IntegratedDimensionWidget types.

    @author Owen Arnold Tessella/ISIS
    @date January 10/2011

    Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
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

//Foward decs
class QLabel;
class QComboBox;
class QLayout;

namespace Mantid
{
 namespace Geometry
 {
 class MDGeometry;
 class IMDDimension;
 }
 namespace VATES
 {
   class GeometryPresenter;
 }
}

enum BinChangeStatus{ApplyBinChanges, IgnoreBinChanges};

class EXPORT_OPT_MANTIDPARVIEW GeometryWidget: public QWidget, public Mantid::VATES::GeometryView
{

private:

  DimensionWidgetFactory m_widgetFactory;

  Mantid::VATES::GeometryPresenter* m_pPresenter;

Q_OBJECT
public:
Q_PROPERTY(QString GeometryXML READ getGeometryXML WRITE setGeometryXML NOTIFY valueChanged)

 GeometryWidget(Mantid::VATES::GeometryPresenter* pPresenter, bool readOnlyLimits);

virtual void raiseModified();

virtual void raiseNoClipping();

~GeometryWidget();

/// Gets the chosen geometry configuration.
QString getGeometryXML() const
{
  return getGeometryXMLString().c_str();
}


void setGeometryXML(QString value)
{
  //Do nothing.
  UNUSED_ARG(value);
}



virtual void addDimensionView(Mantid::VATES::DimensionView*);

virtual std::string getGeometryXMLString() const;

virtual const Mantid::VATES::DimensionViewFactory& getDimensionViewFactory();

/// Single signal gets raised if anything changes
Q_SIGNALS:
        void valueChanged();
        void ignoreBinChanges();

};

#endif
