#ifndef GEOMETRY_VIEW_H
#define GEOMETRY_VIEW_H
#include "MantidKernel/System.h"
#include "MantidVatesAPI/DimensionView.h"
#include "MantidVatesAPI/DimensionViewFactory.h"
namespace Mantid
{
  namespace VATES
  {
    class QString;

    /** 
    @class GeometryView
    Abstract view for controlling multi-dimensional geometries.
    @author Owen Arnold, Tessella plc
    @date 03/06/2011

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class DLLExport GeometryView 
    {
    public:
      virtual void addDimensionView(DimensionView*) = 0;
      virtual std::string getGeometryXMLString() const = 0;
      virtual const DimensionViewFactory& getDimensionViewFactory() = 0;
      virtual ~GeometryView(){};
      virtual void raiseModified() = 0;
      virtual Mantid::VATES::BinDisplay getBinDisplayMode() const = 0;
    };
  }
}

#endif
