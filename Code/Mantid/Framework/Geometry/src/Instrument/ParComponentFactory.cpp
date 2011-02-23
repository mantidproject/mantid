//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidGeometry/Instrument/ParComponentFactory.h"

#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/CompAssembly.h"
#include "MantidGeometry/Instrument/ObjCompAssembly.h" 
#include "MantidGeometry/Instrument/Detector.h" 
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Instrument/ObjComponent.h"
#include "MantidGeometry/Instrument/Instrument.h"

namespace Mantid
{
  namespace Geometry
  {

    /// Detector pool static storage
    ComponentPool<Detector> ParComponentFactory::g_detPool = 
      ComponentPool<Detector>(2*PARALLEL_GET_MAX_THREADS);

    //--------------------------------------------------------------------------
    // ComponentPool
    //--------------------------------------------------------------------------
    /**
     * Constructor
     * @param poolSize The size of the pool to create
     */
    template<typename ClassType>
    ComponentPool<ClassType>::ComponentPool(const size_t poolSize) : 
      m_storeSize(poolSize), m_store(poolSize, PtrType())
    {
    }

    /**
     * Get a pointer to a ParComponent object whether replacing
     * store or creating a new object
     * @param base The base object to wrap
     * @param map A pointer to the ParamterMap
     * @returns A parameterized object
     */
    template<typename ClassType>
    typename ComponentPool<ClassType>::PtrType
    ComponentPool<ClassType>::create(const ClassType *base, 
				     const ParameterMap *map)
    {
      try
      {
    	const size_t index = getIndexInCache();
    	PtrType & cached = m_store[index];
    	if( !cached ) 
    	{
    	  // Creating cached object
    	  cached = PtrType(createUsingNew(base, map));
    	}
    	cached->swap(base, map);
    	return cached;
      }
      catch(std::runtime_error&)
      {
    	return PtrType(createUsingNew(base, map));
      }
      // This will never get hit but MSVC complains if it's not here.
      return PtrType();
    }

    /**
     * Retrieve a index for a pre-allocated object, throwing if one cannot be found
     * @returns An index pointing to a valid area within the cache
     */
    template<typename ClassType>
    size_t ComponentPool<ClassType>::getIndexInCache() const
    {
      size_t index = PARALLEL_THREAD_NUMBER;
      const PtrType & cached_first = m_store[index];
      if( cached_first && !cached_first.unique() )
      {
    	// Try the extra storage
    	index += PARALLEL_GET_MAX_THREADS;
    	const PtrType & cached_second = m_store[index];
    	if( cached_second && !cached_second.unique() )
    	{
    	  throw std::runtime_error("ComponentPool::getIndexInCache - Cannot use cache index.");
    	}
      }
      return index;
    }    
    
    /**
     * Create an object with the new operator
     * @param base The base object to wrap
     * @param map A pointer to the ParamterMap
     * @returns A parameterized object
     */
    template<typename ClassType>
    ClassType* 
    ComponentPool<ClassType>::createUsingNew(const ClassType *base, 
					     const ParameterMap *map)
    {
      return new ClassType(base,map);
    }

    //--------------------------------------------------------------------------
    // ParComponentFactory
    //--------------------------------------------------------------------------

    /**
     * Create a parameterized detector from the given base detector and ParameterMap. This version
     * avoids a cast by directly returning the Detector pointer
     * @param base A pointer to the unparameterized version
     * @param map A pointer to the ParameterMap
     * @returns A pointer to a parameterized component
     */
    boost::shared_ptr<Detector> 
    ParComponentFactory::createDetector(const IDetector *base, const ParameterMap *map)
    {
      // Use a pool for the detectors as the are created very frequently
      const Detector *baseDet = dynamic_cast<const Detector*>(base);
      if( baseDet )
      {
	return g_detPool.create(baseDet,map);
      }
      return boost::shared_ptr<Detector>();
    }

    /**
     * Create a parameterized instrument from the given base detector and ParameterMap. This version
     * avoids a cast by directly returning the Instrument pointer
     * @param base A pointer to the unparameterized version
     * @param map A pointer to the ParameterMap
     * @returns A pointer to a parameterized component
     */
    boost::shared_ptr<Instrument> 
    ParComponentFactory::createInstrument(boost::shared_ptr<Instrument> base, 
					  boost::shared_ptr<ParameterMap> map)
    {
      return boost::shared_ptr<Instrument>(new Instrument(base, map));
    }

    /**
     * Create a parameterized component from the given base component and ParameterMap
     * SLOW VERSION as it has to check each possible type
     * @param base A pointer to the unparameterized version
     * @param map A pointer to the ParameterMap
     * @returns A pointer to a parameterized component
     */
    IComponent_sptr ParComponentFactory::create(IComponent_const_sptr base, 
						const ParameterMap * map)
    {
      boost::shared_ptr<const IDetector> det_sptr = boost::dynamic_pointer_cast<const IDetector>(base);
      if( det_sptr )
      {
    	return createDetector(det_sptr.get(), map);
      }

      boost::shared_ptr<const Instrument> inst_sptr = 
    	boost::dynamic_pointer_cast<const Instrument>(base);
      // @todo One of the review tasks is to take a look at the parametertized mess and
      // short out this problem with different classes carrying different types of pointers around
      if( inst_sptr )
      {
	return createInstrument(boost::const_pointer_cast<Instrument>(inst_sptr), 
				boost::shared_ptr<ParameterMap>(const_cast<ParameterMap*>(map), NoDeleting()));
      }

      // Everything gets created on the fly. Note that the order matters here
      // @todo Really could do with a better system than this. Virtual function maybe?
      const RectangularDetector* rd = dynamic_cast<const RectangularDetector*>(base.get());
      if (rd)
	return boost::shared_ptr<IComponent>(new RectangularDetector(rd,map));

      const CompAssembly* ac = dynamic_cast<const CompAssembly*>(base.get());
      if (ac)
	return boost::shared_ptr<IComponent>(new CompAssembly(ac,map));
      const ObjCompAssembly* oac = dynamic_cast<const ObjCompAssembly*>(base.get());
      if (oac)
	return boost::shared_ptr<IComponent>(new ObjCompAssembly(oac,map));
      
      const ObjComponent* oc = dynamic_cast<const ObjComponent*>(base.get());
      if (oc)
	return boost::shared_ptr<IComponent>(new ObjComponent(oc,map));
      // Must be a component
      const IComponent* cc = dynamic_cast<const IComponent*>(base.get());
      if (cc)
	return boost::shared_ptr<IComponent>(new Component(cc,map));

      return IComponent_sptr();
    }

  }
}
