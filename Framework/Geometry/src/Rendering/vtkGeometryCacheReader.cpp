#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/DOM/NodeFilter.h>
#include <Poco/DOM/AutoPtr.h>
#include <Poco/SAX/InputSource.h>
#include <Poco/Exception.h>
#include <Poco/DOM/Element.h>

#include "MantidKernel/Exception.h"
#include "MantidKernel/Logger.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Rendering/GeometryHandler.h"
#include "MantidGeometry/Rendering/vtkGeometryCacheReader.h"

using Poco::XML::DOMParser;
using Poco::XML::Element;

namespace Mantid {
namespace Geometry {
namespace {
/// static logger
Kernel::Logger g_log("vtkGeometryCacheReader");
}

/**
 * Constructor
 */
vtkGeometryCacheReader::vtkGeometryCacheReader(std::string filename) {
  mFileName = filename;
  mDoc = nullptr;
  Init();
}

/**
 * Destructor
 */
vtkGeometryCacheReader::~vtkGeometryCacheReader() {
  mDoc->release();
  delete pParser;
}

/**
 * Initialise Reading of the cached file
 */
void vtkGeometryCacheReader::Init() {
  // Set up the DOM parser and parse xml file
  pParser = new DOMParser();
  try {
    mDoc = pParser->parse(mFileName);
  } catch (...) {
    throw Kernel::Exception::FileError("Unable to parse File:", mFileName);
  }
}

/**
 * Set the geometry for the object
 */
void vtkGeometryCacheReader::readCacheForObject(IObject *obj) {
  // Get the element corresponding to the name of the object
  std::stringstream objName;
  objName << obj->getName();
  Poco::XML::Element *pEle = getElementByObjectName(objName.str());
  if (pEle == nullptr) // Element not found
  {
    g_log.debug("Cache not found for Object with name " + objName.str());
    return;
  }
  // Read the cache from the element
  int noOfTriangles = 0, noOfPoints = 0;
  std::vector<double> Points;
  std::vector<uint32_t> Faces;
  std::stringstream buff;
  // Read number of points
  buff << pEle->getAttribute("NumberOfPoints");
  buff >> noOfPoints;
  buff.clear();
  // Read number of triangles
  buff << pEle->getAttribute("NumberOfPolys");
  buff >> noOfTriangles;
  buff.clear();

  // Read Points
  Element *pPts = pEle->getChildElement("Points")->getChildElement("DataArray");
  readPoints(pPts, noOfPoints, Points);

  // Read Triangles
  Element *pTris = pEle->getChildElement("Polys")->getChildElement("DataArray");
  readTriangles(pTris, noOfTriangles, Faces);

  // First check whether Object can be written to the file
  boost::shared_ptr<GeometryHandler> handle = obj->getGeometryHandler();
  handle->setGeometryCache(noOfPoints, noOfTriangles, std::move(Points),
                           std::move(Faces));
}

/**
 * Get the Element by using the object name
 */
Poco::XML::Element *
vtkGeometryCacheReader::getElementByObjectName(std::string name) {
  Element *pRoot = mDoc->documentElement();
  if (pRoot == nullptr || pRoot->nodeName() != "VTKFile")
    return nullptr;
  Element *pPolyData = pRoot->getChildElement("PolyData");
  if (pPolyData == nullptr)
    return nullptr;
  return pPolyData->getElementById(name, "name");
}

/**
 * Read the points from the element
 */
void vtkGeometryCacheReader::readPoints(Poco::XML::Element *pEle,
                                        int noOfPoints,
                                        std::vector<double> &points) {
  if (pEle == nullptr) {
    noOfPoints = 0;
    return;
  }
  // Allocate memory
  points.resize(noOfPoints * 3);
  if (points.size() != static_cast<size_t>(noOfPoints * 3)) // Out of memory
  {
    g_log.error("Cannot allocate memory for triangle cache of Object ");
    return;
  }
  if (pEle->getAttribute("format") == "ascii") { // Read from Ascii
    std::stringstream buf;
    buf << pEle->innerText();
    for (double &point : points) {
      buf >> point;
    }
  }
  // Read from binary otherwise
}

/**
 * Read triangle face indexs
 */
void vtkGeometryCacheReader::readTriangles(Poco::XML::Element *pEle,
                                           int noOfTriangles,
                                           std::vector<uint32_t> &faces) {
  if (pEle == nullptr) {
    noOfTriangles = 0;
    return;
  }
  // Allocate memory
  faces.resize(noOfTriangles * 3);
  if (faces.size() != static_cast<size_t>(noOfTriangles * 3)) // Out of memory
  {
    g_log.error("Cannot allocate memory for triangle cache of Object ");
    return;
  }
  if (pEle->getAttribute("format") == "ascii") { // Read from Ascii
    std::stringstream buf;
    buf << pEle->innerText();
    for (unsigned int &face : faces) {
      buf >> face;
    }
  }
  // Read from binary otherwise
}
}
}
