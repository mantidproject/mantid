// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

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
  Poco::XML::Document *m_doc; ///< The XML document
  Poco::XML::Element *m_root; ///< The root XML element
  std::string m_filename;     ///< The file name
  // Private Methods
  void Init();
  void createVTKFileHeader();

public:
  vtkGeometryCacheWriter(std::string); ///< Constructor
  ~vtkGeometryCacheWriter();           ///< Destructor

  // Disable copies as we have a file handle
  vtkGeometryCacheWriter(const vtkGeometryCacheWriter &) = delete;
  vtkGeometryCacheWriter &operator=(const vtkGeometryCacheWriter) = delete;

  void addObject(CSGObject const *obj);
  void write(); ///< Write the XML to a file
};

} // NAMESPACE Geometry

} // NAMESPACE Mantid
