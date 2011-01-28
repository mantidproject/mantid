//-------------------------------------------------------------
// Includes
//-------------------------------------------------------------
#include "MantidGeometry/Objects/InstrumentRayTracer.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidGeometry/Objects/Track.h"
#include "MantidGeometry/V3D.h"
#include "MantidKernel/Exception.h"
#include <deque>
#include <iterator>

namespace Mantid
{
  namespace Geometry
  {

    // Initialize logger
    Kernel::Logger & InstrumentRayTracer::g_log = Kernel::Logger::get("InstrumentRayTracer");

    //-------------------------------------------------------------
    // Public member functions
    //-------------------------------------------------------------
    
    /**
     * Constructor specifying the instrument involved in the tracing. The instrument must have defined a source
     * component.
     * @param instrument :: The instrument to perform the ray tracings on. It must have a defined source.
     */
    InstrumentRayTracer::InstrumentRayTracer(IInstrument_sptr instrument) : m_instrument(instrument)
    {
      if( !m_instrument )
      {
        std::ostringstream lexer;
        lexer << "Cannot create a InstrumentRayTracer, invalid instrument given. Input = " << m_instrument.get() << "\n";
        g_log.error(lexer.str()); 
        throw std::invalid_argument(lexer.str());
      }
      if( !m_instrument->getSource() )
      {
        std::string errorMsg = "Cannot create InstrumentRayTracer, instrument has no defined source.\n";
        g_log.error(errorMsg);
        throw std::invalid_argument(errorMsg);
      }
    }

    /**
     * Trace a given track from the instrument source in the given direction. For performance reasons the 
     * results are accumulated within the object and can be returned using getResults.
     * @param dir :: A directional vector. The starting point is defined by the instrument source.
     */
    void InstrumentRayTracer::trace(const V3D & dir) const
    {
      // Define the track with the source position and the given direction.
      m_resultsTrack.reset(m_instrument->getSource()->getPos(), dir);
      //m_resultsTrack.reset(m_instrument->getSample()->getPos() + V3D(1.0,0.0,0.0), dir);
      // The intersection results are accumulated within the ray object
      fireRay(m_resultsTrack);
    }

    /**
     * Return the results of any trace() calls since the last call the getResults.
     * @returns A collection of links defining intersection information
     */
    Links InstrumentRayTracer::getResults() const
    {
      Links results(m_resultsTrack.begin(), m_resultsTrack.end());
      m_resultsTrack.clearIntersectionResults();
      return results;
    }

    //-------------------------------------------------------------
    // Private member functions
    //-------------------------------------------------------------
    /**
     * Fire the test ray at the instrument and perform a bread-first search of the 
     * object tree to find the objects that were intersected.
     * @param testRay :: An input/output parameter that defines the track and accumulates the
     * intersection results
     */
    void InstrumentRayTracer::fireRay(Track & testRay) const
    {
      // Go through the instrument tree and see if we get any hits by
      // (a) first testing the bounding box and if we're inside that then
      // (b) test the lower components.
      std::deque<IComponent_sptr> nodeQueue;
      //Start at the root of the tree
      nodeQueue.push_back(m_instrument);
      IComponent_sptr node;
      while( !nodeQueue.empty() )
      {
        node = nodeQueue.front();
        nodeQueue.pop_front();
        BoundingBox bbox;
        node->getBoundingBox(bbox);
        // Quick test. If this suceeds moved on to test the children
        if( bbox.doesLineIntersect(testRay) )
        {
          if( ICompAssembly_sptr assembly = boost::dynamic_pointer_cast<ICompAssembly>(node) )
          {
            testIntersectionWithChildren(testRay, assembly, nodeQueue);
          }
          else
          {
            throw Kernel::Exception::NotImplementedError("Implement non-comp assembly interactions");
          }
        }
      }

    }

    /**
    * Test the intersection of the ray with the children of the component assembly
    * @param testRay :: Track under test. The results are stored here.
    * @param assembly :: The children of this assembly will be tested
    * @param searchQueue :: If a child is a sub-assembly then it is appended for later searching
    */
    void InstrumentRayTracer::testIntersectionWithChildren(Track & testRay, 
      ICompAssembly_sptr assembly, std::deque<IComponent_sptr> & searchQueue) const
    {
      int nchildren = assembly->nelements();
      for( int i = 0; i < nchildren; ++i )
      {
        boost::shared_ptr<Geometry::IComponent> comp = assembly->getChild(i);
        if( ICompAssembly_sptr childAssembly = boost::dynamic_pointer_cast<ICompAssembly>(comp) )
        {
          searchQueue.push_back(comp);
        }
        // Check the physical object intersection
        else if( IObjComponent *physicalObject = dynamic_cast<IObjComponent*>(comp.get()) ) 
        {
           physicalObject->interceptSurface(testRay);
        }
        else {}
      }

    }


    ///** 
    // * Perform a quick check as to whether the ray passes through the component
    // * @param component :: The test component
    // */
    // bool InstrumentRayTracer::quickIntersectCheck(boost::shared_ptr<IComponent> component, const Track & testRay) const
    // {
    //   
    // }
    // 
    // /**
    //  * Perform a proper intersection test of the physical object and accumulate the results if necessary
    //  * @param testRay :: An input/output parameter that defines the track and accumulates the
    //  * intersection results
    //  */
    // void slowIntersectCheck(boost::shared_ptr<IComponent> component, Track & testRay) const;     

  }
}
