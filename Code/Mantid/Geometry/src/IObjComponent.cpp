//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/IObjComponent.h"
#include "MantidGeometry/Object.h"
#include "MantidKernel/Exception.h"
#include "MantidGeometry/GeometryHandler.h"
#include "MantidGeometry/CacheGeometryHandler.h"
#include <cfloat>

namespace Mantid
{
	namespace Geometry
	{

		IObjComponent::IObjComponent():m_ScaleFactor(1.0,1.0,1.0)
		{
			handle=new CacheGeometryHandler(this);
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
	} // namespace Geometry
} // namespace Mantid
