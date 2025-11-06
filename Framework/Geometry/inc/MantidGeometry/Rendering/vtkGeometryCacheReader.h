// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/DllConfig.h"

namespace Poco {
namespace XML {
class Document;
class Element;
class DOMParser;
} // namespace XML
} // namespace Poco
namespace Mantid {

namespace Geometry {
class IObject;

/**
   \class vtkGeometryCacheReader
   \brief Reads the Geometry Cache from the file to the Object
   \author Srikanth Nagella
   \date January 2009
   \version 1.0

   This class reads the geometry (triangles) cached in the vtk format file and
   copies them to the object.
*/
class MANTID_GEOMETRY_DLL vtkGeometryCacheReader {
private:
  Poco::XML::Document *m_doc;      ///< The XML document
  Poco::XML::DOMParser *m_pParser; ///< The XML parser
  std::string m_filename;          ///< The file name
  // Private Methods
  void Init();
  Poco::XML::Element *getElementByObjectName(const std::string &name);
  void readPoints(Poco::XML::Element *pEle, int noOfPoints, std::vector<double> &points);
  void readTriangles(Poco::XML::Element *pEle, int noOfTriangles, std::vector<uint32_t> &faces);

public:
  vtkGeometryCacheReader(std::string filename);
  ~vtkGeometryCacheReader();
  void readCacheForObject(IObject const *obj);
};

} // NAMESPACE Geometry

} // NAMESPACE Mantid
