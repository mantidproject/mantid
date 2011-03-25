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


class EXPORT_OPT_MANTIDPARVIEW GeometryWidget: public QWidget
{
Q_OBJECT
public:
Q_PROPERTY(QString XDimensionXML READ getXDimensionXML WRITE setXDimensionXML NOTIFY valueChanged)
Q_PROPERTY(QString YDimensionXML READ getYDimensionXML WRITE setYDimensionXML NOTIFY valueChanged)
Q_PROPERTY(QString ZDimensionXML READ getZDimensionXML WRITE setZDimensionXML NOTIFY valueChanged)
Q_PROPERTY(QString tDimensionXML READ gettDimensionXML WRITE settDimensionXML NOTIFY valueChanged)
GeometryWidget();
void constructWidget(std::vector<boost::shared_ptr<Mantid::Geometry::IMDDimension> > nonIntegratedVector);

void childAppliedNewDimensionSelection(
    const unsigned int oldDimensionIndex,
    boost::shared_ptr<Mantid::Geometry::IMDDimension> newDimension,
    DimensionWidget* pDimensionWidget);

void dimensionWidgetChanged();

~GeometryWidget();

/// Gets the x dimension in a serialized form
QString getXDimensionXML() const;

/// Gets the y dimension in a serialized form
QString getYDimensionXML() const;

/// Gets the z dimension in a serialzed form
QString getZDimensionXML() const;

/// Gets the t dimension in a serialzed form
QString gettDimensionXML() const;

void setXDimensionXML(QString value)
{
  //Do nothing.
}

void setYDimensionXML(QString value)
{
  //Do nothing.
}

void setZDimensionXML(QString value)
{
  //Do nothing.
}

void settDimensionXML(QString value)
{
  //Do nothing.
}

bool isSetup() const
{
  return m_isConstructed;
}

/// Single signal gets raised if anything changes
Q_SIGNALS:
        void valueChanged();
        //void textChanged(const QString &);

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

std::vector<boost::shared_ptr<Mantid::Geometry::IMDDimension> > m_nonIntegratedVector;

};

#endif
