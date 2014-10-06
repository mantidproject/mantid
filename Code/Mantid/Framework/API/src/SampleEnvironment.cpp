//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidAPI/SampleEnvironment.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/Objects/Track.h"

namespace Mantid
{
  namespace API
  {

    using Geometry::IComponent;
    using Geometry::IObjComponent;
    using Kernel::V3D;
    using Geometry::Track;

    //------------------------------------------------------------------------------
    // Public methods
    //------------------------------------------------------------------------------

    /**
     * Constructor specifying a name for the environment
     * @param name :: A name for the environment kit
     */
    SampleEnvironment::SampleEnvironment(const std::string & name) :
      CompAssembly(name, NULL), m_elements()
    {
    }

    /**
     * Copy constructor
     * @param original :: The object whose state is to be copied.
     */
    SampleEnvironment::SampleEnvironment(const SampleEnvironment & original) :
      CompAssembly(original), m_elements(nelements())
    {
      for(int i = 0; i < nelements(); ++i)
      {
        // type is enforced by ::add
        m_elements[i] = dynamic_cast<IObjComponent*>(m_children[i]);
      }
    }

    /**
     * Clone the environment assembly
     * @returns A pointer to the clone object
     */
    Geometry::IComponent* SampleEnvironment::clone() const
    {
      return new SampleEnvironment(*this);
    }

    /**
     * Override the add method so that we can only add physical components that
     * have a defined shape, i.e. an ObjComponent with a valid shape
     * @param comp :: A pointer to the phyiscal component. This object will take
     * ownership of the pointer
     * @returns The number of items within the assembly, after this component has
     * been added
     */
    int SampleEnvironment::add(IComponent * comp)
    {
      // Check if this is a component with a shape
      IObjComponent * physicalComp = dynamic_cast<IObjComponent*>(comp);
      if( !physicalComp )
      {
        throw std::invalid_argument("CompAssembly::add - Invalid component, it must implement "
                                    "the IObjComponent interface");
      }
      if( physicalComp->shape() && physicalComp->shape()->hasValidShape() )
      {
        // Accept this component
        m_elements.reserve(m_elements.size() + 1); // avoid the power-like memory increase
        m_elements.push_back(physicalComp);
        return CompAssembly::add(comp);
      }
      else
      {
        throw std::invalid_argument("CompAssembly::add - Component does not have a defined shape.");
      }
    }

    /**
     * Is the point given a valid point within the environment
     * @param point :: Is the point valid within the environment
     * @returns True if the point is within the environment
     */
    bool SampleEnvironment::isValid(const V3D & point) const
    {
      auto itrEnd = m_elements.end();
      for(auto itr = m_elements.begin(); itr != itrEnd; ++itr)
      {
        if( (*itr)->isValid(point) )
        {
          return true;
        }
      }
      return false;
    }

    /**
     * Update the given track with intersections within the environment
     * @param track :: The track is updated with an intersection with the
     *        environment
     */
    void SampleEnvironment::interceptSurfaces(Track & track) const
    {
      auto itrEnd = m_elements.end();
      for(auto itr = m_elements.begin(); itr != itrEnd; ++itr)
      {
        (*itr)->interceptSurface(track);
      }
    }

  }

}
