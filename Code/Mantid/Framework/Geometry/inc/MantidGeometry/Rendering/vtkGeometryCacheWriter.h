#ifndef VTKGEOMETRYCACHEWRITER_H
#define VTKGEOMETRYCACHEWRITER_H

#include "MantidKernel/System.h"
#include "MantidKernel/Logger.h"
#include <string>
namespace Poco{
  namespace XML{
    class Document;
    class Element;
  }
}
namespace Mantid
{

  namespace Geometry
  {
    /**
       \class vtkGeometryCacheWriter
       \brief Writes the Geometry from Object to Cache
       \author Srikanth Nagella
       \date January 2009
       \version 1.0

       This class writes the geometry (triangles) cached from Object to the vtk format file.

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
    class Object;
    class DLLExport vtkGeometryCacheWriter
    {
    private:

      static Kernel::Logger& PLog;           ///< The official logger
      Poco::XML::Document* mDoc;         ///< The XML document    
      Poco::XML::Element*  mRoot;	     ///< The root XML element
      std::string          mFileName;    ///< The file name
      //Private Methods
      void Init();
      void createVTKFileHeader();
    public:
      vtkGeometryCacheWriter(std::string);       ///< Constructor
      ~vtkGeometryCacheWriter();      ///< Destructor
      void addObject(Object* obj);
      void write();  ///< Write the XML to a file
    };

  }   // NAMESPACE Geometry

}  // NAMESPACE Mantid

#endif
