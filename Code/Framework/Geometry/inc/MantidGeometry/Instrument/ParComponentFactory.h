#ifndef PARCOMPONENT_FACTORY_
#define PARCOMPONENT_FACTORY_
#include "MantidKernel/System.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/IComponent.h"

namespace Mantid
{
namespace Geometry
{

/** @class ParComponentFactory 
    @brief A Factory for creating Parameterized component from their respective non parameterized classes.
    @author Nicholas Draper, ISIS RAL
    @date 20/10/2009

    
    Copyright &copy; 2007-9 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport ParComponentFactory 
{
public:
  ///String description of the type of component
  static boost::shared_ptr<IComponent> create(boost::shared_ptr<const IComponent> base, 
					      const ParameterMap * map);

};

} //Namespace Geometry
} //Namespace Mantid

#endif
