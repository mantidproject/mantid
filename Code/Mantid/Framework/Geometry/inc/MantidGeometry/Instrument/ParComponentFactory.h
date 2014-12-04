#ifndef MANTID_GEOMETRY_PARCOMPONENT_FACTORY_H_
#define MANTID_GEOMETRY_PARCOMPONENT_FACTORY_H_
//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidGeometry/DllConfig.h"
#ifndef Q_MOC_RUN
# include <boost/shared_ptr.hpp>
#endif
#include <vector>
#include <stdexcept>

namespace Mantid
{
  namespace Geometry
  {
    class ParameterMap;
    class IComponent;
    class IDetector;
    class Detector;
    class Instrument;

    /** 
      @brief A Factory for creating Parameterized component 
      from their respective non-parameterized objects.
      @author Nicholas Draper, ISIS RAL
      @date 20/10/2009
      
      Copyright &copy; 2007-11 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source
      
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
    class MANTID_GEOMETRY_DLL ParComponentFactory 
    {
    public:
      /// Create a parameterized detector from the given base component and ParameterMap and
      /// return a shared_ptr<Detector>
      static boost::shared_ptr<Detector> createDetector(const IDetector *base, const ParameterMap *map);
      /// Create a parameterized instrument from the given base and ParameterMap
      static boost::shared_ptr<Instrument> createInstrument(boost::shared_ptr<const Instrument> base,
                                                            boost::shared_ptr<ParameterMap> map);
      /// Create a parameterized component from the given base component and ParameterMap
      static boost::shared_ptr<IComponent> create(boost::shared_ptr<const IComponent> base,
                                                  const ParameterMap * map);
    };
    
} //Namespace Geometry
} //Namespace Mantid

#endif
