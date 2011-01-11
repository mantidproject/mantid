#ifndef DIMENSION_WIDGET_H
#define DIMENSION_WIDGET_H

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

//Foward decs
class QLabel;
class QComboBox;
class IntegratedDimensionWidget;
class DimensionPickerWidget;

namespace Mantid
{
 namespace Geometry
 {
 class MDGeometry;
 }
}


class GeometryWidget: public QWidget
{
Q_OBJECT
public:

GeometryWidget(Mantid::Geometry::MDGeometry const * const geometry);

~GeometryWidget();

private:


};

#endif
