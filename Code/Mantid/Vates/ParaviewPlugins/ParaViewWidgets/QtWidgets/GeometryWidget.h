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



#include <qgridlayout.h>
#include <qwidget.h>
#include <memory>
#include <boost/shared_ptr.hpp>
#include "WidgetDllOption.h"
#include "MantidVatesAPI/GeometryXMLParser.h"

//Foward decs
class QLabel;
class QComboBox;
class IntegratedDimensionWidget;
class DimensionWidget;

namespace Mantid
{
 namespace Geometry
 {
 class MDGeometry;
 class IMDDimension;
 }
}

enum BinChangeStatus{ApplyBinChanges, IgnoreBinChanges};

class EXPORT_OPT_MANTIDPARVIEW GeometryWidget: public QWidget
{
Q_OBJECT
public:
Q_PROPERTY(QString GeometryXML READ getGeometryXML WRITE setGeometryXML NOTIFY valueChanged)
GeometryWidget();
void constructWidget(Mantid::VATES::GeometryXMLParser& source);

void childAppliedNewDimensionSelection(
    const unsigned int oldDimensionIndex,
    boost::shared_ptr<Mantid::Geometry::IMDDimension> newDimension,
    DimensionWidget* pDimensionWidget);

void dimensionWidgetChanged(BinChangeStatus status);

~GeometryWidget();

/// Gets the chosen geometry configuration.
QString getGeometryXML() const;

bool hasXDimension() const
{
  return m_xDimensionWidget != NULL;
}

bool hasYDimension() const
{
  return m_yDimensionWidget != NULL;
}

bool hasZDimension() const
{
  return m_zDimensionWidget != NULL;
}

bool hasTDimension() const
{
  return m_tDimensionWidget != NULL;
}

void setGeometryXML(QString value)
{
  //Do nothing.
  UNUSED_ARG(value);
}


bool isSetup() const
{
  return m_isConstructed;
}

/// Single signal gets raised if anything changes
Q_SIGNALS:
        void valueChanged();
        void ignoreBinChanges();

private:

/// Check that constructWidget has been called.
void validateSetup() const;

/// When dimensions are swapped. Ensure that every dimension widget is given the opportunity to update the nbins to 
/// reflect the value contained in the dimension it now wraps.
void applyBinsFromDimensions();

DimensionWidget* m_xDimensionWidget;
DimensionWidget* m_yDimensionWidget;
DimensionWidget* m_zDimensionWidget;
DimensionWidget* m_tDimensionWidget;
bool m_isConstructed;

};

#endif
