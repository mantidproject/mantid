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
    using Geometry::BoundingBox;
    using Geometry::Track;
    using Kernel::Material;
    using Kernel::V3D;

    //------------------------------------------------------------------------------
    // Public methods
    //------------------------------------------------------------------------------

    /**
     * Constructor specifying a name for the environment. It is empty by default and
     * required by various other users of it
     * @param name :: A name for the environment kit
     */
    SampleEnvironment::SampleEnvironment(const std::string & name) :
      m_name(name), m_elements()
    {
    }

    /**
     * @return An axis-aligned BoundingBox object that encompasses the whole kit.
     */
    Geometry::BoundingBox SampleEnvironment::boundingBox() const
    {
      BoundingBox box;
      auto itrEnd = m_elements.end();
      for(auto itr = m_elements.begin(); itr != itrEnd; ++itr)
      {
        box.grow(itr->getBoundingBox());
      }
      return box;
    }

    /**
     * @param element A shape + material object
     */
    void SampleEnvironment::add(const Geometry::Object &element)
    {
      m_elements.push_back(element);
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
        if( itr->isValid(point) )
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
        itr->interceptSurface(track);
      }
    }

  }

}
