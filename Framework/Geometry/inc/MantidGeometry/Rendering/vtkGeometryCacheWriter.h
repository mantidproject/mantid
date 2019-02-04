// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef VTKGEOMETRYCACHEWRITER_H
#define VTKGEOMETRYCACHEWRITER_H

#include "MantidGeometry/DllConfig.h"
#include <string>

namespace Poco {
namespace XML {
class Document;
class Element;
} // namespace XML
} // namespace Poco
namespace Mantid {

namespace Geometry {
/**
   \class vtkGeometryCacheWriter
   \brief Writes the Geometry from Object to Cache
   \author Srikanth Nagella
   \date January 2009
   \version 1.0

   This class writes the geometry (triangles) cached from Object to the vtk
   format file.
*/
class CSGObject;
class MANTID_GEOMETRY_DLL vtkGeometryCacheWriter {
private:
  Poco::XML::Document *mDoc; ///< The XML document
  Poco::XML::Element *mRoot; ///< The root XML element
  std::string mFileName;     ///< The file name
  // Private Methods
  void Init();
  void createVTKFileHeader();

public:
  vtkGeometryCacheWriter(std::string); ///< Constructor
  ~vtkGeometryCacheWriter();           ///< Destructor
  void addObject(CSGObject *obj);
  void write(); ///< Write the XML to a file
};

} // NAMESPACE Geometry

} // NAMESPACE Mantid

#endif
