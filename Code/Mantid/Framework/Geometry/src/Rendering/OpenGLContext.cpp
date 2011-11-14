#include "MantidGeometry/Rendering/OpenGLContext.h"

namespace Mantid
{
  namespace Geometry
  {

    Kernel::Logger& OpenGLContextImpl::g_log(Kernel::Logger::get("OpenGLContextImpl"));

    OpenGLContextImpl::OpenGLContextImpl():
    m_isOn(false)
    {
    }


  } // Geometry
} // Mantid
