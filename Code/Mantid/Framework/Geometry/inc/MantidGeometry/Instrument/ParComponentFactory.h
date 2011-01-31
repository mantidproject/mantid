#ifndef MANTID_GEOMETRY_PARCOMPONENT_FACTORY_H_
#define MANTID_GEOMETRY_PARCOMPONENT_FACTORY_H_
//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidKernel/System.h"
#include "MantidKernel/MultiThreaded.h"
#include <boost/shared_ptr.hpp>
#include <vector>
#include <stdexcept>
#include <iostream>

namespace Mantid
{
  namespace Geometry
  {
    class ParameterMap;
    class IComponent;
    class IDetector;
    class Detector;
    class IInstrument;
    class Instrument;

    /** 
     * Pool class for caching memory when creating parameterized components
     */
    template<typename ClassType>
    class ComponentPool
    {
    public:
      /// Typedef for held type
      typedef boost::shared_ptr<ClassType> PtrType;
      typedef boost::shared_ptr<const ClassType> ConstPtrType;
      /// Constructor
      ComponentPool(const size_t poolSize);
      
      /**
       * Get a pointer to a ParComponent object whether replacing
       * store or creating a new object
       * @param base The base object to wrap
       * @param map A pointer to the ParamterMap
       * @returns A parameterized object
       */
      PtrType create(ConstPtrType base, const ParameterMap * map);
      
    private:
      /// Default constructor
      ComponentPool();
      /// Retrieve a index for a pre-allocated object, throwing if one cannot be found
      size_t getIndexInCache() const;
      /// Create an object with the new operator
      ClassType* createUsingNew(ConstPtrType base, const ParameterMap *map);

      /// Size of the pool 
      size_t m_storeSize;
      /// Store of pre allocated objects
      std::vector<PtrType> m_store;
    };

    /** 
      @brief A Factory for creating Parameterized component 
      from their respective non-parameterized objects.
      @author Nicholas Draper, ISIS RAL
      @date 20/10/2009
      
      Copyright &copy; 2007-11 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory
      
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

      /// Create a parameterized component from the given base component and ParameterMap
      static boost::shared_ptr<IComponent> createParComponent(boost::shared_ptr<const IComponent> base, 
							      const ParameterMap * map);
      /// Create a parameterized component from the given base component and ParameterMap
      static boost::shared_ptr<Detector> createParDetector(boost::shared_ptr<const IComponent> base, 
							   const ParameterMap * map);


      // ------------ togo --------------------------------------------------------
      /// Create a new shared pointer to a parameterized version of the given base component
      static boost::shared_ptr<IComponent> create(boost::shared_ptr<const IComponent> base, 
						  const ParameterMap * map);
      //------------------

    private:
      // A pool of existing detectors
      static ComponentPool<Detector> g_detPool;
      // A pool of existing detectors
      static ComponentPool<Instrument> g_instPool;

      
    };
    
} //Namespace Geometry
} //Namespace Mantid

#endif
