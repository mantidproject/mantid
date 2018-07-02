#include "MantidGeometry/Rendering/vtkGeometryCacheWriter.h"

#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Rendering/GeometryHandler.h"
#include "MantidKernel/Logger.h"

#include <Poco/DOM/AutoPtr.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/DOMWriter.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/Text.h>
#include <Poco/File.h>
#include <Poco/FileStream.h>
#include <Poco/Path.h>
#include <Poco/XML/XMLWriter.h>

#include <fstream>
#include <sstream>

using Poco::XML::Document;
using Poco::XML::Element;
using Poco::XML::Text;
using Poco::XML::AutoPtr;
using Poco::XML::DOMWriter;
using Poco::XML::XMLWriter;

namespace Mantid {
namespace Geometry {
namespace {
/// static object
Kernel::Logger g_log("vtkGeometryCacheWriter");
}

/**
 * Constructor
 */
vtkGeometryCacheWriter::vtkGeometryCacheWriter(std::string filename) {
  mFileName = filename;

  mDoc = new Document();
  Init();
}

/**
 * Destructor
 */
vtkGeometryCacheWriter::~vtkGeometryCacheWriter() {
  mRoot->release();
  mDoc->release();
}

/**
 * Initialises the XML Document with the required vtk XML Headings
 */
void vtkGeometryCacheWriter::Init() {
  mRoot = mDoc->createElement("PolyData");
  createVTKFileHeader();
}
/**
 * creates VTK XML header
 * \<VTKFile type="PolyData" version="1.0" byte_order="LittleEndian"\>
 *    \<PolyData\>
 *    \</PolyData\>
 * \</VTKFile\>
 */
void vtkGeometryCacheWriter::createVTKFileHeader() {
  AutoPtr<Element> pRoot = mDoc->createElement("VTKFile");
  pRoot->setAttribute("type", "PolyData");
  pRoot->setAttribute("version", "1.0");
  pRoot->setAttribute("byte_order", "LittleEndian");
  mDoc->appendChild(pRoot);
  pRoot->appendChild(mRoot);
}

/**
 * Adds the geometry of the Object to the document
 * @param obj :: The object to add
 */
void vtkGeometryCacheWriter::addObject(CSGObject *obj) {
  // First check whether Object can be written to the file
  boost::shared_ptr<GeometryHandler> handle = obj->getGeometryHandler();
  if (!(handle->canTriangulate()))
    return; // Cannot add the object to the file
  std::stringstream buf;
  // get the name of the Object
  int name = obj->getName();
  // get number of point
  auto noPts = handle->numberOfPoints();
  // get number of triangles
  auto noTris = handle->numberOfTriangles();
  // Add Piece
  AutoPtr<Element> pPiece = mDoc->createElement("Piece");
  // Add attribute name
  buf << name;
  pPiece->setAttribute("name", buf.str());
  // Add Number of Points
  buf.str("");
  buf << noPts;
  pPiece->setAttribute("NumberOfPoints", buf.str());
  // Add Number of Triangles/Polys
  buf.str("");
  buf << noTris;
  pPiece->setAttribute("NumberOfPolys", buf.str());
  // write the points
  AutoPtr<Element> pPoints = mDoc->createElement("Points");
  AutoPtr<Element> pPtsDataArray = mDoc->createElement("DataArray");
  // Add attributes to data array
  pPtsDataArray->setAttribute("type", "Float32");
  pPtsDataArray->setAttribute("NumberOfComponents", "3");
  pPtsDataArray->setAttribute("format", "ascii");
  buf.str("");
  // get the triangles info
  const auto &points = handle->getTriangleVertices();
  size_t i;
  for (i = 0; i < points.size(); i++) {
    buf << points[i] << " ";
  }
  AutoPtr<Text> pPointText = mDoc->createTextNode(buf.str());
  pPtsDataArray->appendChild(pPointText);
  pPoints->appendChild(pPtsDataArray);
  // get triangles faces info
  AutoPtr<Element> pFaces = mDoc->createElement("Polys");
  AutoPtr<Element> pTrisDataArray = mDoc->createElement("DataArray");
  // add attribute
  pTrisDataArray->setAttribute("type", "UInt32");
  pTrisDataArray->setAttribute("Name", "connectivity");
  pTrisDataArray->setAttribute("format", "ascii");

  buf.str("");
  const auto &faces = handle->getTriangleFaces();
  for (i = 0; i < faces.size(); i++) {
    buf << faces[i] << " ";
  }
  AutoPtr<Text> pTrisDataText = mDoc->createTextNode(buf.str());
  pTrisDataArray->appendChild(pTrisDataText);
  pFaces->appendChild(pTrisDataArray);
  // set the offsets
  AutoPtr<Element> pTrisOffsetDataArray = mDoc->createElement("DataArray");
  // add attribute
  pTrisOffsetDataArray->setAttribute("type", "UInt32");
  pTrisOffsetDataArray->setAttribute("Name", "offsets");
  pTrisOffsetDataArray->setAttribute("format", "ascii");
  buf.str("");
  for (i = 1; i < noTris + 1; i++) {
    buf << i * 3 << " ";
  }
  AutoPtr<Text> pTrisOffsetDataText = mDoc->createTextNode(buf.str());
  pTrisOffsetDataArray->appendChild(pTrisOffsetDataText);
  pFaces->appendChild(pTrisOffsetDataArray);

  // append all
  pPiece->appendChild(pPoints);
  pPiece->appendChild(pFaces);

  // add this piece to root
  mRoot->appendChild(pPiece);
}
/**
 * Write the XML to the file
 */
void vtkGeometryCacheWriter::write() {
  DOMWriter writer;
  writer.setNewLine("\n");
  writer.setOptions(XMLWriter::PRETTY_PRINT);
  std::ofstream file;
  try {
    g_log.information("Writing Geometry Cache file to " + mFileName);
    file.open(mFileName.c_str(), std::ios::trunc);
    writer.writeNode(file, mDoc);
    file.close();
  } catch (...) {
    g_log.error("Geometry Cache file writing exception");
  }
}
}
}
