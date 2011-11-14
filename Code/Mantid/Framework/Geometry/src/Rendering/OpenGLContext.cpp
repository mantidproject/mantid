#include "MantidGeometry/Rendering/OpenGLContext.h"

namespace Mantid
{
  namespace Geometry
  {

    Kernel::Logger& OpenGLContext::g_log(Kernel::Logger::get("OpenGLContext"));

    OpenGLContext::OpenGLContext():
    m_isOn(false)
    {
    }

    OpenGLContext& OpenGLContext::Instance()
    {
      static OpenGLContext context;
      return context;
    }


  } // Geometry
} // Mantid
