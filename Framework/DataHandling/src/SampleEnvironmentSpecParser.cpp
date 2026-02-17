// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/SampleEnvironmentSpecParser.h"
#include "MantidAPI/FileFinder.h"
#include "MantidDataHandling/LoadStlFactory.h"
#ifdef ENABLE_LIB3MF
#include "MantidDataHandling/Mantid3MFFileIO.h"
#endif
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Objects/ShapeFactory.h"

#include "MantidKernel/MaterialXMLParser.h"
#include "MantidKernel/Strings.h"

#include "Poco/DOM/AutoPtr.h"
#include "Poco/DOM/DOMParser.h"
#include "Poco/DOM/DOMWriter.h"
#include "Poco/DOM/Document.h"
#include "Poco/DOM/NamedNodeMap.h"
#include "Poco/DOM/NodeFilter.h"
#include "Poco/DOM/NodeIterator.h"
#include "Poco/SAX/InputSource.h"
#include "Poco/SAX/SAXException.h"

#include <boost/algorithm/string.hpp>
#include <filesystem>
#include <memory>
#include <sstream>

using namespace Poco::XML;

//------------------------------------------------------------------------------
// Anonymous
//------------------------------------------------------------------------------
namespace {
std::string MATERIALS_TAG = "materials";
std::string COMPONENTS_TAG = "components";
std::string FULL_SPEC_TAG = "fullspecification";
std::string COMPONENT_TAG = "component";
std::string CONTAINERS_TAG = "containers";
std::string CONTAINER_TAG = "container";
std::string COMPONENTGEOMETRY_TAG = "geometry";
std::string SAMPLEGEOMETRY_TAG = "samplegeometry";
std::string COMPONENTSTLFILE_TAG = "stlfile";
std::string SAMPLESTLFILE_TAG = "samplestlfile";
} // namespace

namespace Mantid::DataHandling {

namespace {

std::string MATERIALS_TAG = "materials";
std::string COMPONENTS_TAG = "components";
std::string COMPONENT_TAG = "component";
std::string GLOBAL_OFFSET_TAG = "globaloffset";
std::string TRANSLATION_VECTOR_TAG = "translationvector";
std::string STL_FILENAME_TAG = "stlfilename";
std::string SCALE_TAG = "scale";
std::string XDEGREES_TAG = "xdegrees";
std::string YDEGREES_TAG = "ydegrees";
std::string ZDEGREES_TAG = "zdegrees";
} // namespace

namespace {
double DegreesToRadians(double angle) { return angle * M_PI / 180; }
} // namespace
//------------------------------------------------------------------------------
// Public methods
//------------------------------------------------------------------------------
/**
 * Takes a stream that is assumed to contain a single complete
 * SampleEnvironmentSpec definition, reads the definition
 * and produces a new SampleEnvironmentSpec object.
 * @param name The name of the environment
 * @param filename Optional file name for .xml spec file
 * @param istr A reference to a stream
 * @return A new SampleEnvironmentSpec object
 */
SampleEnvironmentSpec_uptr SampleEnvironmentSpecParser::parse(const std::string &name, const std::string &filename,
                                                              std::istream &istr) {
  using DocumentPtr = AutoPtr<Document>;

  InputSource src(istr);
  DOMParser parser;
  // Do not use auto here or anywhereas the Poco API returns raw pointers
  // but in some circumstances requires AutoPtrs to manage the memory
  DocumentPtr doc;
  try {
    doc = parser.parse(&src);
  } catch (SAXParseException &exc) {
    std::ostringstream msg;
    msg << "SampleEnvironmentSpecParser::parse() - Error parsing content "
           "as valid XML: "
        << exc.what();
    throw std::runtime_error(msg.str());
  }
  m_filepath = filename;
  return parse(name, doc->documentElement());
}

/**
 * Takes a pointer to an XML node that is assumed to point at a
 * "environmentspec" tag.
 * It reads the definition and produces a new SampleEnvironmentSpec object.
 * @param name The name of the environment
 * @param element A pointer to an Element node that is a "environmentspec" tag
 * @return A new SampleEnvironmentSpec object
 */
SampleEnvironmentSpec_uptr SampleEnvironmentSpecParser::parse(const std::string &name, Poco::XML::Element *element) {
  validateRootElement(element);

  // Iterating is apparently much faster than getElementsByTagName
  NodeIterator nodeIter(element, NodeFilter::SHOW_ELEMENT);
  Node *node = nodeIter.nextNode();
  auto spec = std::make_unique<SampleEnvironmentSpec>(name);

  while (node) {
    auto *childElement = static_cast<Element *>(node);
    if (node->nodeName() == MATERIALS_TAG) {
      parseMaterials(childElement);
    } else if (node->nodeName() == COMPONENTS_TAG) {
      parseAndAddComponents(spec.get(), childElement);
    } else if (node->nodeName() == FULL_SPEC_TAG) {
      loadFullSpecification(spec.get(), childElement);
    }
    node = nodeIter.nextNode();
  }
  return spec;
}

//------------------------------------------------------------------------------
// Private methods
//------------------------------------------------------------------------------
/**
 * Validate that the element points to the expected root element
 * @param element A pointer to the root element
 */
void SampleEnvironmentSpecParser::validateRootElement(Poco::XML::Element *element) const {
  if (element->nodeName() != ROOT_TAG) {
    std::ostringstream msg;
    msg << "SampleEnvironmentSpecParser::validateRootElement() - Element tag "
           "does not match '"
        << ROOT_TAG << "'. Found " << element->nodeName() << "\n";
    throw std::invalid_argument(msg.str());
  }
}

/**
 * Parse the set of materials in the document
 * @param element A pointer to the materials tag
 */
void SampleEnvironmentSpecParser::parseMaterials(Poco::XML::Element *element) {
  using Kernel::MaterialXMLParser;

  m_materials.clear();
  NodeIterator nodeIter(element, NodeFilter::SHOW_ELEMENT);
  // Points at <materials>
  nodeIter.nextNode();
  // Points at first <material>
  Node *node = nodeIter.nextNode();
  MaterialXMLParser parser;
  while (node) {
    auto material = parser.parse(static_cast<Poco::XML::Element *>(node), m_filepath);
    m_materials.emplace(material.name(), material);
    node = nodeIter.nextNode();
  }
}

/**
 * Take a \<components\> tag, parse the definitions and add them to the spec.
 * It requires the materials to have been parsed
 * @param spec A pointer to a SampleEnvironmentSpec to update
 * @param element A pointer to a components element
 */
void SampleEnvironmentSpecParser::parseAndAddComponents(SampleEnvironmentSpec *spec, Element *element) const {
  if (m_materials.empty()) {
    throw std::runtime_error("SampleEnvironmentSpecParser::parseComponents() - "
                             "Trying to parse list of components but no "
                             "materials have been found. Please ensure the "
                             "materials are defined first.");
  }
  NodeIterator nodeIter(element, NodeFilter::SHOW_ELEMENT);
  // Points at <components>
  nodeIter.nextNode();
  // Points at first <child>
  Node *node = nodeIter.nextNode();
  while (node) {
    auto *childElement = static_cast<Element *>(node);
    const auto &nodeName = childElement->nodeName();
    if (nodeName == CONTAINERS_TAG) {
      parseAndAddContainers(spec, childElement);
    } else if (nodeName == COMPONENT_TAG) {
      spec->addComponent(parseComponent(childElement));
    }
    node = nodeIter.nextNode();
  }
}

void SampleEnvironmentSpecParser::loadFullSpecification(SampleEnvironmentSpec *spec, Poco::XML::Element *element) {
  using Mantid::Geometry::Container;
  auto filename = element->getAttribute("filename");
  if (!filename.empty()) {

    std::string stlFileName = findFile(filename);

    std::filesystem::path suppliedFileName(stlFileName);
    std::string fileExt = suppliedFileName.extension().string().substr(1); // drop the '.'
    std::transform(fileExt.begin(), fileExt.end(), fileExt.begin(), toupper);

    std::vector<std::shared_ptr<Geometry::MeshObject>> environmentMeshes;
    std::shared_ptr<Geometry::MeshObject> sampleMesh;

    if (fileExt == "3MF") {
#ifdef ENABLE_LIB3MF
      Mantid3MFFileIO MeshLoader;
      MeshLoader.LoadFile(stlFileName);

      MeshLoader.readMeshObjects(environmentMeshes, sampleMesh);
#else
      throw std::runtime_error("3MF format not supported on this platform");
#endif

      for (auto cpt : environmentMeshes) {
        if (spec->ncans() == 0) {
          // 3mf format doesn't nicely support multiple cans so just
          // hardcode id to default
          cpt->setID("default");
          auto can = std::make_shared<Container>(cpt);
          can->setSampleShape(sampleMesh);
          spec->addContainer(can);
        } else {

          spec->addComponent(cpt);
        }
      }
    } else {
      throw std::runtime_error("Full specification must be a .3mf file");
    }
  } else {
    throw std::runtime_error("fullspecification element supplied without a filename");
  }
}

/**
 * Take a \<containers\> tag, parse the definitions and add them to the spec.
 * It requires the materials to have been parsed.
 * @param spec A pointer to a SampleEnvironmentSpec to update
 * @param element A pointer to a cans element
 */
void SampleEnvironmentSpecParser::parseAndAddContainers(SampleEnvironmentSpec *spec, Element *element) const {
  NodeIterator nodeIter(element, NodeFilter::SHOW_ELEMENT);
  nodeIter.nextNode();
  Node *node = nodeIter.nextNode();
  while (node) {
    auto *childElement = static_cast<Element *>(node);
    if (childElement->nodeName() == CONTAINER_TAG) {
      spec->addContainer(parseContainer(childElement));
    }
    node = nodeIter.nextNode();
  }
}

/**
 * Parse a single definition of a Can
 * @param element A pointer to an XML \<container\> element
 * @return A new Can instance
 */
Geometry::Container_const_sptr SampleEnvironmentSpecParser::parseContainer(Element *element) const {
  using Mantid::Geometry::Container;
  auto can = std::make_shared<Container>(parseComponent(element));
  auto sampleGeometry = element->getChildElement(SAMPLEGEOMETRY_TAG);
  auto sampleSTLFile = element->getChildElement(SAMPLESTLFILE_TAG);

  if ((sampleGeometry) && (sampleSTLFile)) {
    throw std::runtime_error("SampleEnvironmentSpecParser::parseComponent() - "
                             "Cannot define sample using both a" +
                             SAMPLEGEOMETRY_TAG + " and a " + SAMPLESTLFILE_TAG + " child tag.");
  }

  if (sampleGeometry) {
    DOMWriter writer;
    std::stringstream sampleShapeXML;
    writer.writeNode(sampleShapeXML, sampleGeometry);
    can->setSampleShape(sampleShapeXML.str());
  }
  if (sampleSTLFile) {
    can->setSampleShape(loadMeshFromSTL(sampleSTLFile));
  }
  return can;
}

/**
 * Load a double from an optional XML element
 * @param componentElement XML element
 * @param attributeName Attribute that double should be loaded from
 * @param targetVariable Value read from element attribute
 */
void SampleEnvironmentSpecParser::LoadOptionalDoubleFromXML(Poco::XML::Element *componentElement,
                                                            const std::string &attributeName,
                                                            double &targetVariable) const {

  auto attributeText = componentElement->getAttribute(attributeName);
  if (!attributeText.empty()) {
    try {
      targetVariable = std::stod(attributeText);
    } catch (std::invalid_argument &ex) {
      throw std::invalid_argument(std::string("Invalid string supplied for " + attributeName + " ") + ex.what());
    }
  }
}

/**
 * Take a comma separated translation vector and return it as a std::vector
 * @param translationVectorStr Translation vector string
 * @return vector containing translations
 */
std::vector<double> SampleEnvironmentSpecParser::parseTranslationVector(const std::string &translationVectorStr) const {

  std::vector<double> translationVector;

  // Split up comma-separated properties
  using tokenizer = Mantid::Kernel::StringTokenizer;
  tokenizer values(translationVectorStr, ",", tokenizer::TOK_IGNORE_EMPTY | tokenizer::TOK_TRIM);

  translationVector.clear();
  translationVector.reserve(values.count());

  std::transform(values.cbegin(), values.cend(), std::back_inserter(translationVector),
                 [](const std::string &str) { return boost::lexical_cast<double>(str); });
  return translationVector;
}

std::string SampleEnvironmentSpecParser::findFile(const std::string &filename) const {
  std::filesystem::path suppliedStlFileName(filename);
  std::filesystem::path stlFileName;
  if (suppliedStlFileName.is_relative()) {
    bool useSearchDirectories = true;
    if (!m_filepath.empty()) {
      // if environment spec xml came from a file, search in the same
      // directory as the file
      stlFileName = std::filesystem::path(m_filepath).parent_path() / filename;
      if (std::filesystem::exists(stlFileName)) {
        useSearchDirectories = false;
      }
    }

    if (useSearchDirectories) {
      // ... and if that doesn't work look in the search directories
      stlFileName = Mantid::API::FileFinder::Instance().getFullPath(filename);
      if (stlFileName.empty()) {
        stlFileName = suppliedStlFileName;
      }
    }
  } else {
    stlFileName = suppliedStlFileName;
  }
  return stlFileName.string();
}

/**
 * Create a mesh shape from an STL input file. This can't be in the ShapeFactory
 * because that is in Geometry. This function needs acccess to the STL readers
 * @param stlFileElement A pointer to an XML \<stlfile\> element
 * @return A new Object instance of the given type
 */
std::shared_ptr<Geometry::MeshObject> SampleEnvironmentSpecParser::loadMeshFromSTL(Element *stlFileElement) const {
  std::string filename = stlFileElement->getAttribute("filename");
  if (!filename.empty()) {

    std::string stlFileName = findFile(filename);

    if (std::filesystem::exists(stlFileName)) {

      std::string scaleStr = stlFileElement->getAttribute("scale");
      if (scaleStr.empty()) {
        throw std::runtime_error("Scale must be supplied for stl file:" + filename);
      }
      const ScaleUnits scaleType = getScaleTypeFromStr(scaleStr);

      std::unique_ptr<LoadStl> reader = LoadStlFactory::createReader(stlFileName, scaleType);

      std::shared_ptr<Geometry::MeshObject> comp = reader->readShape();

      Element *rotation = stlFileElement->getChildElement("rotation");
      if (rotation) {

        double xDegrees = 0, yDegrees = 0, zDegrees = 0;
        LoadOptionalDoubleFromXML(rotation, XDEGREES_TAG, xDegrees);
        LoadOptionalDoubleFromXML(rotation, YDEGREES_TAG, yDegrees);
        LoadOptionalDoubleFromXML(rotation, ZDEGREES_TAG, zDegrees);

        const double xRotation = DegreesToRadians(xDegrees);
        const double yRotation = DegreesToRadians(yDegrees);
        const double zRotation = DegreesToRadians(zDegrees);
        comp = reader->rotate(comp, xRotation, yRotation, zRotation);
      }
      Element *translation = stlFileElement->getChildElement("translation");
      if (translation) {
        std::string translationVectorStr = translation->getAttribute("vector");
        const std::vector<double> translationVector = parseTranslationVector(translationVectorStr);
        comp = reader->translate(comp, translationVector);
      }
      return comp;
    } else {
      throw std::runtime_error("Unable to find STLFile " + filename);
    }
  } else {
    throw std::runtime_error("STLFile element supplied without a filename");
  }
}

/**
 * Parse a single definition of a component. If the component is a can the
 * sample geometry, if available, is also parsed.
 * @param element A pointer to an XML \<container\> element
 * @return A new Object instance of the given type
 */
std::shared_ptr<Geometry::IObject> SampleEnvironmentSpecParser::parseComponent(Element *element) const {
  Element *geometry = element->getChildElement(COMPONENTGEOMETRY_TAG);
  Element *stlfile = element->getChildElement(COMPONENTSTLFILE_TAG);
  if ((!geometry) && (!stlfile)) {
    throw std::runtime_error("SampleEnvironmentSpecParser::parseComponent() - Expected a " + COMPONENTGEOMETRY_TAG +
                             " or " + COMPONENTSTLFILE_TAG + " child tag. None found.");
  }
  if ((geometry) && (stlfile)) {
    throw std::runtime_error("SampleEnvironmentSpecParser::parseComponent() - "
                             "Cannot define container using both a" +
                             COMPONENTGEOMETRY_TAG + " and a " + COMPONENTSTLFILE_TAG + " child tag.");
  }

  std::shared_ptr<Geometry::IObject> comp;
  if (stlfile) {
    comp = loadMeshFromSTL(stlfile);
  } else {
    Geometry::ShapeFactory factory;
    comp = factory.createShape(geometry);
  }
  auto materialID = element->getAttribute("material");
  auto iter = m_materials.find(materialID);
  Kernel::Material mat;
  if (iter != m_materials.end()) {
    mat = iter->second;
  } else {
    throw std::runtime_error("SampleEnvironmentSpecParser::parseComponent() - "
                             "Unable to find material with id=" +
                             materialID);
  }
  comp->setID(element->getAttribute("id"));
  comp->setMaterial(mat);
  return comp;
}

} // namespace Mantid::DataHandling
