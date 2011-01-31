//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/IObjComponent.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidKernel/Exception.h"
#include "MantidGeometry/Rendering/GeometryHandler.h"
#include "MantidGeometry/Rendering/CacheGeometryHandler.h"
#include <cfloat>

namespace Mantid
{
  namespace Geometry
  {

    IObjComponent::IObjComponent()
      :m_ScaleFactor(1.0,1.0,1.0)
    {
      handle = new CacheGeometryHandler(this);
    }

    /** Constructor, specifying the GeometryHandler (renderer engine)
     * for this IObjComponent.
     */
    IObjComponent::IObjComponent(GeometryHandler* the_handler)
      :m_ScaleFactor(1.0,1.0,1.0)
    {
      handle = the_handler;
    }

    // Looking to get rid of the first of these constructors in due course (and probably add others)
    IObjComponent::~IObjComponent()
    {
      if(handle!=NULL)
	delete handle;
    }

    /**
     * Sets the scaling factor of the object for the Object Component.
     * @param xFactor: scaling factor in x-direction
     * @param yFactor: scaling factor in y-direction
     * @param zFactor: scaling factor in z-direction
     */
    void IObjComponent::setScaleFactor(double xFactor,double yFactor, double zFactor)
    {
      m_ScaleFactor=V3D(xFactor,yFactor,zFactor);
    }

    /**
     * Set the geometry handler for IObjComponent
     * @param[in] h is pointer to the geometry handler. don't delete this pointer in the calling function.
     */
    void IObjComponent::setGeometryHandler(GeometryHandler *h)
    {
      if(h==NULL)return;
      this->handle=h;
    }

    /**
     * Copy constructor
     * @param origin :: The object to initialize this with
     */
    IObjComponent::IObjComponent(const IObjComponent& origin) : 
      m_ScaleFactor(origin.m_ScaleFactor)
    {
      // Handler contains a pointer to 'this' therefore needs regenerating
      // with new object
      handle = origin.handle->createInstance(this);
    }

    /**
     * Assignment operator
     * @param rhs The rvalue to copy into this object
     * @returns A reference to this object
     */
    IObjComponent& IObjComponent::operator=(const IObjComponent& rhs)
    {
      if( &rhs != this )
      {
	m_ScaleFactor = rhs.m_ScaleFactor;
	handle = rhs.handle->createInstance(this);
      }
      return *this;
    }

    
  } // namespace Geometry
} // namespace Mantid
