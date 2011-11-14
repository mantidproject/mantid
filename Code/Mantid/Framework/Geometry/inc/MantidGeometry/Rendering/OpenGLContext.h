#ifndef OPENGL_CONTEXT_H
#define OPENGL_CONTEXT_H

#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/SingletonHolder.h"
#include "MantidKernel/Logger.h"
namespace Mantid
{

  namespace Geometry
  {
    /**
       \class OpenGLContext
       \brief Keep infomation about the status of OpenGL.
       \author Roman Tolchenov, Tessella plc
       \date 14/11/2011
       \version 1.0

       This is a singleton class to keep information about the current status of OpenGL. 
       The minimum information it should provide is whether OpenGL is avalable and it is safe and meaninful 
       to issue an gl command.

       It is a quick fix solution to a problem that on some systems calling gl functions outside OpenGL context
       can crash Mantid.

       Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    */
    class MANTID_GEOMETRY_DLL OpenGLContextImpl
    {
    public:
      /// True if OpenGL is avalable
      bool isAvailable() const {return m_isOn;}
      /// Set avalability of OpenGL. Must be updated by the GL widget.
      void setAvalable(bool yes) {m_isOn = yes;}
    private:
      friend struct Mantid::Kernel::CreateUsingNew<OpenGLContextImpl>;

      OpenGLContextImpl();       ///< Constructor
      OpenGLContextImpl(const OpenGLContextImpl&);             ///< Private copy constructor
      OpenGLContextImpl& operator=(const OpenGLContextImpl&);  ///< Private assignment operator
      ~OpenGLContextImpl(){}      ///< Destructor

      bool m_isOn;    ///< flag indicating whether OpenGL is avalable or not
      static Kernel::Logger& g_log;   ///< The logger

    };

        ///Forward declaration of a specialisation of SingletonHolder for OpenGLContextImpl (needed for dllexport/dllimport) and a typedef for it.
#ifdef _WIN32
// this breaks new namespace declaraion rules; need to find a better fix
        template class MANTID_GEOMETRY_DLL Mantid::Kernel::SingletonHolder<OpenGLContextImpl>;
#endif /* _WIN32 */
        typedef MANTID_GEOMETRY_DLL Mantid::Kernel::SingletonHolder<OpenGLContextImpl> OpenGLContext;

  }   // NAMESPACE Geometry

}  // NAMESPACE Mantid

#endif // OPENGL_CONTEXT_H
