// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include <fstream>
#include <sstream>

#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/InstrumentDefinitionParser.h"
#include "MantidGeometry/Instrument/ObjCompAssembly.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/Instrument/StructuredDetector.h"
#include "MantidGeometry/Instrument/XMLInstrumentParameter.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidGeometry/Rendering/vtkGeometryCacheReader.h"
#include "MantidGeometry/Rendering/vtkGeometryCacheWriter.h"
#include "MantidKernel/ChecksumHelper.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/ProgressBase.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTypes/Core/DateAndTime.h"
#include "MantidTypes/Core/DateAndTimeHelpers.h"

#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/DOMWriter.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeFilter.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/Path.h>
#include <Poco/SAX/AttributesImpl.h>
#include <Poco/String.h>
#include <Poco/XML/XMLWriter.h>

#include <boost/regex.hpp>
#include <memory>
#include <unordered_set>
#include <utility>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::Types::Core;
using Poco::XML::Document;
using Poco::XML::DOMParser;
using Poco::XML::Element;
using Poco::XML::Node;
using Poco::XML::NodeFilter;
using Poco::XML::NodeIterator;
using Poco::XML::NodeList;

namespace Mantid::Geometry {
namespace {
// initialize the static logger
Kernel::Logger g_log("InstrumentDefinitionParser");
} // namespace
//----------------------------------------------------------------------------------------------
/** Default Constructor - not very functional in this state
 */
InstrumentDefinitionParser::InstrumentDefinitionParser()
    : m_xmlFile(std::make_shared<NullIDFObject>()), m_cacheFile(std::make_shared<NullIDFObject>()), m_pDoc(nullptr),
      m_hasParameterElement_beenSet(false), m_haveDefaultFacing(false), m_deltaOffsets(false), m_angleConvertConst(1.0),
      m_indirectPositions(false), m_cachingOption(NoneApplied) {
  initialise("", "", "", "");
}
//----------------------------------------------------------------------------------------------
/** Constructor
 * @param filename :: IDF .xml path (full). This is needed mostly to find the
 *instrument geometry cache.
 * @param instName :: name of the instrument
 * @param xmlText :: XML contents of IDF
 */
InstrumentDefinitionParser::InstrumentDefinitionParser(const std::string &filename, const std::string &instName,
                                                       const std::string &xmlText)
    : m_xmlFile(std::make_shared<NullIDFObject>()), m_cacheFile(std::make_shared<NullIDFObject>()), m_pDoc(nullptr),
      m_hasParameterElement_beenSet(false), m_haveDefaultFacing(false), m_deltaOffsets(false), m_angleConvertConst(1.0),
      m_indirectPositions(false), m_cachingOption(NoneApplied) {
  initialise(filename, instName, xmlText, "");
}

//----------------------------------------------------------------------------------------------
/** Construct the XML parser based on an IDF xml and cached vtp file objects.
 *
 * @param xmlFile :: The xml file, here wrapped in a IDFObject
 * @param expectedCacheFile :: Expected vtp cache file
 * @param instName :: Instrument name
 * @param xmlText :: XML contents of IDF
 */
InstrumentDefinitionParser::InstrumentDefinitionParser(const IDFObject_const_sptr &xmlFile,
                                                       const IDFObject_const_sptr &expectedCacheFile,
                                                       const std::string &instName, const std::string &xmlText)
    : m_xmlFile(std::make_shared<NullIDFObject>()), m_cacheFile(std::make_shared<NullIDFObject>()), m_pDoc(nullptr),
      m_hasParameterElement_beenSet(false), m_haveDefaultFacing(false), m_deltaOffsets(false), m_angleConvertConst(1.0),
      m_indirectPositions(false), m_cachingOption(NoneApplied) {
  initialise(xmlFile->getFileFullPathStr(), instName, xmlText, expectedCacheFile->getFileFullPathStr());

  m_cacheFile = expectedCacheFile;
}

//----------------------------------------------------------------------------------------------
/** Initialise method used in Constructor
 * @param filename :: IDF .xml path (full). This is needed mostly to find the
 *instrument geometry cache.
 * @param instName :: name of the instrument
 * @param xmlText :: XML contents of IDF
 * @param vtpFilename :: the path to the vtp file if you want to override the
 *default
 */
void InstrumentDefinitionParser::initialise(const std::string &filename, const std::string &instName,
                                            const std::string &xmlText, const std::string &vtpFilename) {

  IDFObject_const_sptr xmlFile = std::make_shared<const IDFObject>(filename);

  // Handle the parameters
  m_instName = instName;
  m_xmlFile = xmlFile;

  // do quick check for side-by-side-view-location string, if it doesn't exist we can skip checking every element,
  // thereby speeding up processing
  m_sideBySideViewLocation_exists = xmlText.find("side-by-side-view-location") != std::string::npos;

  // Create our new instrument
  // We don't want the instrument name taken out of the XML file itself, it
  // should come from the filename (or the property)
  m_instrument = std::make_shared<Instrument>(m_instName);

  // Save the XML file path and contents
  m_instrument->setFilename(filename);
  m_instrument->setXmlText(xmlText);

  // Use the filename to construct the cachefile name so that there is a 1:1 map
  // between a definition file & cache
  if (vtpFilename.empty()) {
    m_cacheFile = std::make_shared<const IDFObject>(createVTPFileName());
  } else {
    m_cacheFile = std::make_shared<const IDFObject>(vtpFilename);
  }
}

//----------------------------------------------------------------------------------------------
/**
 * Handle used in the singleton constructor for instrument file should append
 *the value
 * file sha-1 checksum to determine if it is already in
 *memory so that
 * changes to the instrument file will cause file to be reloaded.
 *
 * @return a mangled name combining the filename and the checksum
 *attribute of the XML contents
 * */
std::string InstrumentDefinitionParser::getMangledName() {

  std::string retVal;
  // use the xml in preference if available
  auto xml = Poco::trim(m_instrument->getXmlText());
  if (!(xml.empty())) {
    std::string checksum = Kernel::ChecksumHelper::sha1FromString(xml);
    retVal = m_instName + checksum;
  } else if (this->m_xmlFile->exists()) { // Use the file
    retVal = m_xmlFile->getMangledName();
  }

  return retVal;
}

//----------------------------------------------------------------------------------------------
/** Lazy loads the document and returns a autopointer
 *
 * @return an autopointer to the xml document
 */
Poco::AutoPtr<Poco::XML::Document> InstrumentDefinitionParser::getDocument() {
  if (!m_pDoc) {
    // instantiate if not created
    if (m_instrument->getXmlText().empty()) {
      throw std::invalid_argument("Instrument XML string is empty");
    }
    // Set up the DOM parser and parse xml file
    DOMParser pParser;
    try {
      m_pDoc = pParser.parseString(m_instrument->getXmlText());
    } catch (Poco::Exception &exc) {
      throw std::invalid_argument(exc.displayText() + ". Unable to parse XML");
    } catch (...) {
      throw std::invalid_argument("Unable to parse XML");
    }
  }
  return m_pDoc;
}

/**
 * Type names in the IDF must be unique. Throw an exception if this one is not.
 *
 * @param filename :: Filename of the IDF, for the exception message
 * @param typeName :: Name of the type being checked
 */
void InstrumentDefinitionParser::throwIfTypeNameNotUnique(const std::string &filename,
                                                          const std::string &typeName) const {
  if (getTypeElement.find(typeName) != getTypeElement.end()) {
    g_log.error(std::string("XML file: ")
                    .append(filename)
                    .append("contains more than one type element named ")
                    .append(typeName));
    throw Kernel::Exception::InstrumentDefinitionError(
        std::string("XML instrument file contains more than one type element named ")
            .append(typeName)
            .append(filename));
  }
}

//----------------------------------------------------------------------------------------------
/** Fully parse the IDF XML contents and returns the instrument thus created
 *
 * @param progressReporter :: Optional Progress reporter object. If NULL, no
 * progress reporting.
 * @return the instrument that was created
 */
Instrument_sptr InstrumentDefinitionParser::parseXML(Kernel::ProgressBase *progressReporter) {
  auto pDoc = getDocument();

  // Get pointer to root element
  Poco::XML::Element *pRootElem = pDoc->documentElement();

  if (!pRootElem->hasChildNodes()) {
    g_log.error("Instrument XML contains no root element.");
    throw Kernel::Exception::InstrumentDefinitionError("No root element in XML instrument");
  }

  setValidityRange(pRootElem);
  readDefaults(pRootElem->getChildElement("defaults"));
  Geometry::ShapeFactory shapeCreator;

  const std::string filename = m_xmlFile->getFileFullPathStr();

  std::vector<Element *> typeElems;
  std::vector<Element *> compElems;
  getTypeAndComponentPointers(pRootElem, typeElems, compElems);

  if (typeElems.empty()) {
    g_log.error("XML file: " + filename + "contains no type elements.");
    throw Kernel::Exception::InstrumentDefinitionError("No type elements in XML instrument file", filename);
  }

  collateTypeInformation(filename, typeElems, shapeCreator);

  // Populate m_hasParameterElement
  createVectorOfElementsContainingAParameterElement(pRootElem);

  // See if any parameters set at instrument level
  setLogfile(m_instrument.get(), pRootElem, m_instrument->getLogfileCache());

  parseLocationsForEachTopLevelComponent(progressReporter, filename, compElems);

  // Don't need this anymore (if it was even used) so empty it out to save
  // memory
  m_tempPosHolder.clear();

  // Read in or create the geometry cache file
  m_cachingOption = setupGeometryCache();

  // Add/overwrite any instrument params with values specified in
  // <component-link> XML elements
  setComponentLinks(m_instrument, pRootElem);

  if (m_indirectPositions)
    createNeutronicInstrument();

  // Instrument::markAsDetector is slow unless the detector IDs in the IDF are
  // sorted. To circumvent this we use the 2-part interface,
  // markAsDetectorIncomplete (which does not sort) and markAsDetectorFinalize
  // (which does the final sorting).
  m_instrument->markAsDetectorFinalize();

  // And give back what we created
  return m_instrument;
}

/**
 * Collect some information about types for later use including:
 * - populate directory getTypeElement
 * - populate directory isTypeAssembly
 * - create shapes for all none assembly components and store in
 * mapTypeNameToShape
 * - If 'Outline' attribute set for assembly add attribute object_created=no
 * to indicate the shape for this assembly should be created later.
 *
 * @param filename :: Name of the IDF, for exception message
 * @param typeElems :: Vector of pointers to type elements
 * @param shapeCreator :: Factory for creating a shape
 */
void InstrumentDefinitionParser::collateTypeInformation(const std::string &filename,
                                                        const std::vector<Element *> &typeElems,
                                                        ShapeFactory &shapeCreator) {
  const size_t numberOfTypes = typeElems.size();
  for (size_t iType = 0; iType < numberOfTypes; ++iType) {
    Element *pTypeElem = typeElems[iType];
    std::string typeName = pTypeElem->getAttribute("name");

    // If type contains <combine-components-into-one-shape> then make adjustment
    // after this loop has completed
    Poco::AutoPtr<NodeList> pNL_type_combine_into_one_shape =
        pTypeElem->getElementsByTagName("combine-components-into-one-shape");
    if (pNL_type_combine_into_one_shape->length() > 0) {
      continue;
    }

    throwIfTypeNameNotUnique(filename, typeName);
    getTypeElement[typeName] = pTypeElem;
    createShapeIfTypeIsNotAnAssembly(shapeCreator, iType, pTypeElem, typeName);
  }

  adjustTypesContainingCombineComponentsElement(shapeCreator, filename, typeElems, numberOfTypes);
}

/**
 * Aggregate locations and IDs for components
 *
 * @param progressReporter :: A progress reporter
 * @param filename :: Name of the IDF, for exception message
 * @param compElems :: Vector of pointers for component elements
 */
void InstrumentDefinitionParser::parseLocationsForEachTopLevelComponent(ProgressBase *progressReporter,
                                                                        const std::string &filename,
                                                                        const std::vector<Element *> &compElems) {
  if (progressReporter)
    progressReporter->resetNumSteps(compElems.size(), 0.0, 1.0);

  for (auto pElem : compElems) {
    if (progressReporter)
      progressReporter->report("Loading instrument Definition");

    {
      IdList idList; // structure to possibly be populated with detector IDs

      checkComponentContainsLocationElement(pElem, filename);

      // Loop through all children of this component and see if any
      // are a <location> or <locations>. Done this way, the
      // order they are processed is the order they are listed in the
      // IDF. This is necessary to match the order of the detector IDs.
      for (Node *pNode = pElem->firstChild(); pNode != nullptr; pNode = pNode->nextSibling()) {
        auto pChildElem = dynamic_cast<Element *>(pNode);
        if (!pChildElem)
          continue;
        if (pChildElem->tagName() == "location") {
          // process differently depending on whether component is and
          // assembly or leaf
          if (isAssembly(pElem->getAttribute("type"))) {
            appendAssembly(m_instrument.get(), pChildElem, pElem, idList);
          } else {
            appendLeaf(m_instrument.get(), pChildElem, pElem, idList);
          }
        } else if (pChildElem->tagName() == "locations") {
          // append <locations> elements in <locations>
          appendLocations(m_instrument.get(), pChildElem, pElem, idList);
        }
      } // finished looping over all children of this component

      checkIdListExistsAndDefinesEnoughIDs(idList, pElem, filename);
      idList.reset();
    }
  }
}

/**
 * Component must contain a \<location\> or \<locations\>
 * Throw an exception if it does not
 *
 * @param pElem :: Element with the idlist
 * @param filename :: Name of the IDF, for exception message
 */
void InstrumentDefinitionParser::checkComponentContainsLocationElement(Element *pElem,
                                                                       const std::string &filename) const {
  Poco::AutoPtr<NodeList> pNL_location = pElem->getElementsByTagName("location");
  Poco::AutoPtr<NodeList> pNL_locations = pElem->getElementsByTagName("locations");

  if (pNL_location->length() == 0 && pNL_locations->length() == 0) {
    g_log.error(std::string("A component element must contain at least one "
                            "<location> or <locations> element") +
                " even if it is just an empty location element of the form "
                "<location />");
    throw Kernel::Exception::InstrumentDefinitionError(std::string("A component element must contain at least one "
                                                                   "<location> or <locations> element") +
                                                           " even if it is just an empty location element of the form "
                                                           "<location />",
                                                       filename);
  }
}

/**
 * Check that the required IdList exists in the IDF and defines a sufficient
 * number of IDs
 *
 * @param idList :: The IdList
 * @param pElem :: Element with the idlist
 * @param filename :: Name of the IDF, for exception message
 */
void InstrumentDefinitionParser::checkIdListExistsAndDefinesEnoughIDs(const IdList &idList, Element *pElem,
                                                                      const std::string &filename) const {
  if (idList.counted != static_cast<int>(idList.vec.size())) {
    std::stringstream ss1, ss2;
    ss1 << idList.vec.size();
    ss2 << idList.counted;
    if (!pElem->hasAttribute("idlist")) {
      g_log.error("No detector ID list found for detectors of type " + pElem->getAttribute("type"));
    } else if (idList.vec.empty()) {
      g_log.error("No detector IDs found for detectors in list " + pElem->getAttribute("idlist") +
                  "for detectors of type" + pElem->getAttribute("type"));
    } else {
      g_log.error("The number of detector IDs listed in idlist named " + pElem->getAttribute("idlist") +
                  " is larger than the number of detectors listed in type = " + pElem->getAttribute("type"));
    }
    throw Kernel::Exception::InstrumentDefinitionError(
        "Number of IDs listed in idlist (=" + ss1.str() + ") is larger than the number of detectors listed in type = " +
            pElem->getAttribute("type") + " (=" + ss2.str() + ").",
        filename);
  }
}

/**
 * Create a vector of elements which contain a \<parameter\>
 *
 * @param pRootElem :: Pointer to the root element
 */
void InstrumentDefinitionParser::createVectorOfElementsContainingAParameterElement(Element *pRootElem) {
  Poco::AutoPtr<NodeList> pNL_parameter = pRootElem->getElementsByTagName("parameter");
  unsigned long numParameter = pNL_parameter->length();
  m_hasParameterElement.reserve(numParameter);

  // It turns out that looping over all nodes and checking if their nodeName is
  // equal to "parameter" is much quicker than looping over the pNL_parameter
  // NodeList.
  NodeIterator it(pRootElem, NodeFilter::SHOW_ELEMENT);
  Node *pNode = it.nextNode();
  while (pNode) {
    if (pNode->nodeName() == "parameter") {
      auto pParameterElem = dynamic_cast<Element *>(pNode);
      m_hasParameterElement.emplace_back(dynamic_cast<Element *>(pParameterElem->parentNode()));
    }
    pNode = it.nextNode();
  }

  m_hasParameterElement_beenSet = true;
}

/**
 * "Adjust" (see adjust method) each type which contains a
 * \<combine-components-into-one-shape\> element
 *
 * @param shapeCreator :: Factory for creating a shape
 * @param filename :: Name of the IDF file
 * @param typeElems :: Vector of pointers to type elements
 * @param numberOfTypes :: Total number of type elements
 */
void InstrumentDefinitionParser::adjustTypesContainingCombineComponentsElement(ShapeFactory &shapeCreator,
                                                                               const std::string &filename,
                                                                               const std::vector<Element *> &typeElems,
                                                                               const size_t numberOfTypes) {
  for (size_t iType = 0; iType < numberOfTypes; ++iType) {
    Element *pTypeElem = typeElems[iType];
    std::string typeName = pTypeElem->getAttribute("name");

    // In this loop only interested in types containing
    // <combine-components-into-one-shape>
    Poco::AutoPtr<NodeList> pNL_type_combine_into_one_shape =
        pTypeElem->getElementsByTagName("combine-components-into-one-shape");
    if (pNL_type_combine_into_one_shape->length() == 0)
      continue;

    throwIfTypeNameNotUnique(filename, typeName);
    getTypeElement[typeName] = pTypeElem;

    InstrumentDefinitionParser helper;
    helper.adjust(pTypeElem, isTypeAssembly, getTypeElement);

    isTypeAssembly[typeName] = false;

    mapTypeNameToShape[typeName] = shapeCreator.createShape(pTypeElem);
    // Only CSGObjects can be combined into one shape.
    if (auto csgObj = std::dynamic_pointer_cast<CSGObject>(mapTypeNameToShape[typeName])) {
      csgObj->setName(static_cast<int>(iType));
    }
  }
}

/**
 * If type does not contain a component element then it is not an assembly
 * and a shape can be created
 *
 * @param shapeCreator :: Factory for creating a shape
 * @param iType :: The i-th type
 * @param pTypeElem :: Pointer to the type element
 * @param typeName :: Name of the type
 */
void InstrumentDefinitionParser::createShapeIfTypeIsNotAnAssembly(ShapeFactory &shapeCreator, size_t iType,
                                                                  Element *pTypeElem, const std::string &typeName) {
  Poco::AutoPtr<NodeList> pNL_local = pTypeElem->getElementsByTagName("component");
  if (pNL_local->length() == 0) {
    isTypeAssembly[typeName] = false;

    // for now try to create a geometry shape associated with every type
    // that does not contain any component elements
    mapTypeNameToShape[typeName] = shapeCreator.createShape(pTypeElem);
    // Name can be set only for a CSGObject.
    if (auto csgObj = std::dynamic_pointer_cast<CSGObject>(mapTypeNameToShape[typeName])) {
      csgObj->setName(static_cast<int>(iType));
    }
  } else {
    isTypeAssembly[typeName] = true;
    if (pTypeElem->hasAttribute("outline")) {
      pTypeElem->setAttribute("object_created", "no");
    }
  }
}

/**
 * Create vectors of pointers to \<type\>s and \<component\>s"
 *
 * @param pRootElem :: Pointer to the root element
 * @param typeElems :: Reference to type vector to populate
 * @param compElems :: Reference to component vector to populate
 */
void InstrumentDefinitionParser::getTypeAndComponentPointers(const Element *pRootElem,
                                                             std::vector<Element *> &typeElems,
                                                             std::vector<Element *> &compElems) const {
  for (auto pNode = pRootElem->firstChild(); pNode != nullptr; pNode = pNode->nextSibling()) {
    auto pElem = dynamic_cast<Element *>(pNode);
    if (pElem) {
      if (pElem->tagName() == "type")
        typeElems.emplace_back(pElem);
      else if (pElem->tagName() == "component")
        compElems.emplace_back(pElem);
    }
  }
}

//-----------------------------------------------------------------------------------------------------------------------
/** Assumes second argument is a XML location element and its parent is a
 *component element
 *  which is assigned to be an assembly. This method appends the parent
 *component element of the location element to the CompAssembly passed as the
 *1st arg. Note this method may call itself, i.e. it may act recursively.
 *
 *  @param parent :: CompAssembly to append new component to
 *  @param pLocElems ::  Poco::XML element that points to a locations element in
 *an instrument description XML file, which optionally may be detached (meaning
 *it is not required to be part of the DOM tree of the IDF)
 *  @param pCompElem :: The Poco::XML \<component\> element that contains the
 *\<locations\> element
 *  @param idList :: The current IDList
 */
void InstrumentDefinitionParser::appendLocations(Geometry::ICompAssembly *parent, const Poco::XML::Element *pLocElems,
                                                 const Poco::XML::Element *pCompElem, IdList &idList) {
  // create detached <location> elements from <locations> element
  Poco::AutoPtr<Document> pLocationsDoc = convertLocationsElement(pLocElems);

  // Get pointer to root element
  const Element *pRootLocationsElem = pLocationsDoc->documentElement();
  const bool assembly = isAssembly(pCompElem->getAttribute("type"));

  auto *pElem = dynamic_cast<Poco::XML::Element *>(pRootLocationsElem->firstChild());

  while (pElem) {
    if (pElem->tagName() != "location") {
      pElem = dynamic_cast<Poco::XML::Element *>(pElem->nextSibling());
      continue;
    }

    if (assembly) {
      appendAssembly(parent, pElem, pCompElem, idList);
    } else {
      appendLeaf(parent, pElem, pCompElem, idList);
    }

    pElem = dynamic_cast<Poco::XML::Element *>(pElem->nextSibling());
  }
}

//-----------------------------------------------------------------------------------------------------------------------
/** Save DOM tree to xml file. This method was initially added for testing
 *purpose
 *  but may be useful for other purposes. During the parsing of the DOM tree
 *  in parseXML() the tree may be modified, e.g. if
 *<combine-components-into-one-shape>
 *  is used.
 *
 *  @param outFilename :: Output filename
 */
void InstrumentDefinitionParser::saveDOM_Tree(const std::string &outFilename) {
  Poco::XML::DOMWriter writer;
  writer.setNewLine("\n");
  writer.setOptions(Poco::XML::XMLWriter::PRETTY_PRINT);

  auto pDoc = getDocument();
  std::ofstream outFile(outFilename.c_str());
  writer.writeNode(outFile, pDoc);
  outFile.close();
}

double InstrumentDefinitionParser::attrToDouble(const Poco::XML::Element *pElem, const std::string &name) {
  if (pElem->hasAttribute(name)) {
    const std::string &value = pElem->getAttribute(name);
    if (!value.empty()) {
      try {
        return std::stod(value);
      } catch (...) {
        std::stringstream msg;
        msg << "failed to convert \"" << value << "\" to double for xml attribute \"" << name
            << "\" - using 0. instead";
        g_log.warning(msg.str());
        return 0.;
      }
    }
  }
  return 0.;
}

//-----------------------------------------------------------------------------------------------------------------------
/** Set location (position) of comp as specified in XML location element.
 *
 *  @param comp :: To set position/location off
 *  @param pElem ::  Poco::XML element that points a \<location\> element, which
 *optionally may be detached (meaning it is not required to be part of the DOM
 *tree of the IDF)
 *  @param angleConvertConst :: constant for converting deg to rad
 *  @param deltaOffsets :: radial position offsets
 *
 *  @throw logic_error Thrown if second argument is not a pointer to a
 *'location' XML element
 */
void InstrumentDefinitionParser::setLocation(Geometry::IComponent *comp, const Poco::XML::Element *pElem,
                                             const double angleConvertConst, const bool deltaOffsets) {
  comp->setPos(getRelativeTranslation(comp, pElem, angleConvertConst, deltaOffsets));

  // Rotate coordinate system of this component
  if (pElem->hasAttribute("rot")) {
    double rotAngle = angleConvertConst * attrToDouble(pElem, "rot"); // assumed to be in degrees

    double axis_x = 0.0;
    double axis_y = 0.0;
    double axis_z = 1.0;

    if (pElem->hasAttribute("axis-x"))
      axis_x = std::stod(pElem->getAttribute("axis-x"));
    if (pElem->hasAttribute("axis-y"))
      axis_y = std::stod(pElem->getAttribute("axis-y"));
    if (pElem->hasAttribute("axis-z"))
      axis_z = std::stod(pElem->getAttribute("axis-z"));

    comp->rotate(Kernel::Quat(rotAngle, Kernel::V3D(axis_x, axis_y, axis_z)));
  }

  // Check if sub-elements <trans> or <rot> of present - for now ignore these if
  // m_deltaOffset = true

  Element *pRecursive = nullptr;
  Element *tElem = pElem->getChildElement("trans");
  Element *rElem = pElem->getChildElement("rot");
  bool stillTransElement = true;
  bool firstRound = true; // during first round below pRecursive has not been set up front
  while (stillTransElement) {
    // figure out if child element is <trans> or <rot> or none of these

    if (firstRound) {
      firstRound = false;
    } else if (pRecursive != nullptr) {
      tElem = pRecursive->getChildElement("trans");
      rElem = pRecursive->getChildElement("rot");
    }

    if (tElem && rElem) {
      // if both a <trans> and <rot> child element present. Ignore <rot> element
      rElem = nullptr;
    }

    if (!tElem && !rElem) {
      stillTransElement = false;
    }

    Kernel::V3D posTrans;

    if (tElem) {
      posTrans = getRelativeTranslation(comp, tElem, angleConvertConst, deltaOffsets);

      // to get the change in translation relative to current rotation of comp
      Geometry::CompAssembly compToGetRot;
      Geometry::CompAssembly compRot;
      compRot.setRot(comp->getRotation());
      compToGetRot.setParent(&compRot);
      compToGetRot.setPos(posTrans);

      // Apply translation
      comp->translate(compToGetRot.getPos());

      // for recursive action
      pRecursive = tElem;
    } // end translation

    if (rElem) {
      double rotAngle = angleConvertConst * attrToDouble(rElem, "val"); // assumed to be in degrees

      double axis_x = 0.0;
      double axis_y = 0.0;
      double axis_z = 1.0;

      if (rElem->hasAttribute("axis-x"))
        axis_x = std::stod(rElem->getAttribute("axis-x"));
      if (rElem->hasAttribute("axis-y"))
        axis_y = std::stod(rElem->getAttribute("axis-y"));
      if (rElem->hasAttribute("axis-z"))
        axis_z = std::stod(rElem->getAttribute("axis-z"));

      comp->rotate(Kernel::Quat(rotAngle, Kernel::V3D(axis_x, axis_y, axis_z)));

      // for recursive action
      pRecursive = rElem;
    }

  } // end while
}

void InstrumentDefinitionParser::setSideBySideViewLocation(Geometry::IComponent *comp,
                                                           const Poco::XML::Element *pCompElem) {
  // return if no elements contain side-by-side-view-location parameter
  if (!m_sideBySideViewLocation_exists)
    return;

  auto pViewLocElem = pCompElem->getChildElement("side-by-side-view-location");
  if (pViewLocElem) {
    double x = attrToDouble(pViewLocElem, "x");
    double y = attrToDouble(pViewLocElem, "y");
    comp->setSideBySideViewPos(V2D(x, y));
  }
}

//-----------------------------------------------------------------------------------------------------------------------
/** Calculate the position of comp relative to its parent from info provided by
 *\<location\> element.
 *
 *  @param comp :: To set position/location off
 *  @param pElem ::  Poco::XML element that points a \<location\> element, which
 *optionally may be detached (meaning it is not required to be part of the DOM
 *tree of the IDF)
 *  @param angleConvertConst :: constant for converting deg to rad
 *  @param deltaOffsets :: radial position offsets
 *
 *  @return  Thrown if second argument is not a pointer to a 'location' XML
 *element
 */
Kernel::V3D InstrumentDefinitionParser::getRelativeTranslation(const Geometry::IComponent *comp,
                                                               const Poco::XML::Element *pElem,
                                                               const double angleConvertConst,
                                                               const bool deltaOffsets) {
  Kernel::V3D retVal; // position relative to parent

  // Polar coordinates can be labelled as (r,t,p) or (R,theta,phi)
  if (pElem->hasAttribute("r") || pElem->hasAttribute("t") || pElem->hasAttribute("p") || pElem->hasAttribute("R") ||
      pElem->hasAttribute("theta") || pElem->hasAttribute("phi")) {

    double R = attrToDouble(pElem, "r");
    double theta = angleConvertConst * attrToDouble(pElem, "t");
    double phi = angleConvertConst * attrToDouble(pElem, "p");

    if (pElem->hasAttribute("R"))
      R = attrToDouble(pElem, "R");
    if (pElem->hasAttribute("theta"))
      theta = angleConvertConst * attrToDouble(pElem, "theta");
    if (pElem->hasAttribute("phi"))
      phi = angleConvertConst * attrToDouble(pElem, "phi");

    if (deltaOffsets) {
      // In this case, locations given are radial offsets to the (radial)
      // position of the parent,
      // so need to do some extra calculation before they're stored internally
      // as x,y,z offsets.

      // Temporary vector to hold the parent's absolute position (will be 0,0,0
      // if no parent)
      Kernel::V3D parentPos;
      // Get the parent's absolute position (if the component has a parent)
      if (comp->getParent()) {
        std::map<const Geometry::IComponent *, SphVec>::iterator it;
        it = m_tempPosHolder.find(comp);
        SphVec parent;
        if (it == m_tempPosHolder.end())
          parent = m_tempPosHolder[comp->getParent().get()];
        else
          parent = it->second;

        // Add to the current component to get its absolute position
        R += parent.r;
        theta += parent.theta;
        phi += parent.phi;
        // Set the temporary V3D with the parent's absolute position
        parentPos.spherical(parent.r, parent.theta, parent.phi);
      }

      // Create a temporary vector that holds the absolute r,theta,phi position
      // Needed to make things work in situation when a parent object has a phi
      // value but a theta of zero
      SphVec tmp(R, theta, phi);
      // Add it to the map with the pointer to the Component object as key
      m_tempPosHolder[comp] = tmp;

      // Create a V3D and set its position to be the child's absolute position
      Kernel::V3D absPos;
      absPos.spherical(R, theta, phi);

      // Subtract the two V3D's to get what we want (child's relative position
      // in x,y,z)
      retVal = absPos - parentPos;
    } else {
      // In this case, the value given represents a vector from the parent to
      // the child
      retVal.spherical(R, theta, phi);
    }

  } else {
    double x = attrToDouble(pElem, "x");
    double y = attrToDouble(pElem, "y");
    double z = attrToDouble(pElem, "z");

    retVal(x, y, z);
  }

  return retVal;
}

//-----------------------------------------------------------------------------------------------------------------------
/** Get parent \<component\> element of \<location\> element.
 *
 *  @param pLocElem ::  Poco::XML element that points a location element in the
 *XML doc
 *  @return Parent XML element to a location XML element
 *
 *  @throw logic_error Thrown if argument is not a child of component element
 */
Poco::XML::Element *InstrumentDefinitionParser::getParentComponent(const Poco::XML::Element *pLocElem) {
  if (((pLocElem->tagName()) != "location") && ((pLocElem->tagName()) != "locations")) {
    const std::string &tagname = pLocElem->tagName();
    g_log.error("Argument to function getParentComponent must be a pointer to "
                "an XML element with tag name location or locations.");
    throw std::logic_error(std::string("Argument to function getParentComponent must be a pointer "
                                       "to an XML element") +
                           "with tag name location or locations." + " The tag name is " + tagname);
  }

  // The location element is required to be a child of a component element. Get
  // this component element

  Node *pCompNode = pLocElem->parentNode();

  Element *pCompElem;
  if (pCompNode->nodeType() == 1) {
    pCompElem = static_cast<Element *>(pCompNode);
    if ((pCompElem->tagName()) != "component") {
      g_log.error("Argument to function getParentComponent must be a XML "
                  "element sitting inside a component element.");
      throw std::logic_error("Argument to function getParentComponent must be "
                             "a XML element sitting inside a component "
                             "element.");
    }
  } else {
    g_log.error("Argument to function getParentComponent must be a XML element "
                "whos parent is an element.");
    throw std::logic_error("Argument to function getParentComponent must be a "
                           "XML element whos parent is an element.");
  }

  return pCompElem;
}

//-----------------------------------------------------------------------------------------------------------------------
/** Get name of a location element. It will return the value of the attribute
 *'name', or the
 *  parent's name attribute, or the parent's type, if all else fails.
 *
 *  @param pElem ::  Poco::XML element that points to a \<location\> element,
 *which optionally may be detached (meaning it is not required to be part of the
 *DOM tree of the IDF)
 *  @param pCompElem :: The Poco::XML \<component\> element that contain the
 *location element, which may optionally be detached from the DOM tree also
 *  @return name of location element
 */
std::string InstrumentDefinitionParser::getNameOfLocationElement(const Poco::XML::Element *pElem,
                                                                 const Poco::XML::Element *pCompElem) {
  std::string retVal;

  if (pElem->hasAttribute("name"))
    retVal = pElem->getAttribute("name");
  else if (pCompElem->hasAttribute("name")) {
    retVal = pCompElem->getAttribute("name");
  } else {
    retVal = pCompElem->getAttribute("type");
  }

  return retVal;
}

//------------------------------------------------------------------------------------------------------------------------------
/** Checks the validity range in the IDF and adds it to the instrument object
 *  @param pRootElem A pointer to the root element of the instrument definition
 */
void InstrumentDefinitionParser::setValidityRange(const Poco::XML::Element *pRootElem) {
  const std::string filename = m_xmlFile->getFileFullPathStr();
  // check if IDF has valid-from and valid-to tags defined
  if (!pRootElem->hasAttribute("valid-from")) {
    throw Kernel::Exception::InstrumentDefinitionError("<instrument> element must contain a valid-from tag", filename);
  } else {
    try {
      DateAndTime d(pRootElem->getAttribute("valid-from"));
      m_instrument->setValidFromDate(d);
    } catch (...) {
      throw Kernel::Exception::InstrumentDefinitionError("The valid-from <instrument> tag must be a ISO8601 string",
                                                         filename);
    }
  }

  if (!pRootElem->hasAttribute("valid-to")) {
    DateAndTime d = DateAndTime::getCurrentTime();
    m_instrument->setValidToDate(d);
    // Ticket #2335: no required valid-to date.
    // throw Kernel::Exception::InstrumentDefinitionError("<instrument> element
    // must contain a valid-to tag", filename);
  } else {
    try {
      DateAndTime d(pRootElem->getAttribute("valid-to"));
      m_instrument->setValidToDate(d);
    } catch (...) {
      throw Kernel::Exception::InstrumentDefinitionError("The valid-to <instrument> tag must be a ISO8601 string",
                                                         filename);
    }
  }
}

PointingAlong axisNameToAxisType(const std::string &label, const std::string &input) {
  PointingAlong direction;
  if (input == "x") {
    direction = X;
  } else if (input == "y") {
    direction = Y;
  } else if (input == "z") {
    direction = Z;
  } else {
    std::stringstream msg;
    msg << "Cannot create \"" << label << "\" with axis direction other than \"x\", \"y\", or \"z\", found \"" << input
        << "\"";
    throw Kernel::Exception::InstrumentDefinitionError(msg.str());
  }
  return direction;
}

//-----------------------------------------------------------------------------------------------------------------------
/** Reads the contents of the \<defaults\> element to set member variables,
 *  requires m_instrument to be already set
 *  @param defaults :: points to the data read from the \<defaults\> element,
 * can be null.
 */
void InstrumentDefinitionParser::readDefaults(Poco::XML::Element *defaults) {
  // Return without complaint, if there are no defaults
  if (!defaults)
    return;

  // Check whether spherical coordinates should be treated as offsets to parents
  // position
  std::string offsets;
  Element *offsetElement = defaults->getChildElement("offsets");
  if (offsetElement)
    offsets = offsetElement->getAttribute("spherical");
  if (offsets == "delta")
    m_deltaOffsets = true;

  // Check whether default facing is set
  Element *defaultFacingElement = defaults->getChildElement("components-are-facing");
  if (defaultFacingElement) {
    m_haveDefaultFacing = true;
    m_defaultFacing = parseFacingElementToV3D(defaultFacingElement);
  }

  // the default view is used by the instrument viewer to decide the angle to
  // display the instrument from on start up
  Element *defaultView = defaults->getChildElement("default-view");
  if (defaultView) {
    m_instrument->setDefaultViewAxis(defaultView->getAttribute("axis-view"));
    if (defaultView->hasAttribute("view")) {
      m_instrument->setDefaultView(defaultView->getAttribute("view"));
    }
  }

  // check if angle=radian has been set
  Element *angleUnit = defaults->getChildElement("angle");
  if (angleUnit) {
    if (angleUnit->getAttribute("unit") == "radian") {
      m_angleConvertConst = 180.0 / M_PI;
      std::map<std::string, std::string> &units = m_instrument->getLogfileUnit();
      units["angle"] = "radian";
    }
  }

  // Check if the IDF specifies that this is an indirect geometry instrument
  // that includes
  // both physical and 'neutronic' postions.
  // Any neutronic position tags will be ignored if this tag is missing
  if (defaults->getChildElement("indirect-neutronic-positions"))
    m_indirectPositions = true;

  /*
  Try to extract the reference frame information.
  */
  // Get the target xml element.
  Element *referenceFrameElement = defaults->getChildElement("reference-frame");
  // Extract if available
  if (referenceFrameElement) {
    using Poco::XML::XMLString;
    // Get raw xml values
    Element *upElement = referenceFrameElement->getChildElement("pointing-up");
    Element *alongElement = referenceFrameElement->getChildElement("along-beam");
    Element *handednessElement = referenceFrameElement->getChildElement("handedness");
    Element *originElement = referenceFrameElement->getChildElement("origin");
    Element *thetaSignElement = referenceFrameElement->getChildElement("theta-sign");

    // Defaults
    XMLString s_alongBeam("z");
    XMLString s_pointingUp("y");
    XMLString s_handedness("right");
    XMLString s_origin;

    // Make extractions from sub elements where possible.
    if (alongElement) {
      s_alongBeam = alongElement->getAttribute("axis");
    }
    if (upElement) {
      s_pointingUp = upElement->getAttribute("axis");
    }
    if (handednessElement) {
      s_handedness = handednessElement->getAttribute("val");
    }
    if (originElement) {
      s_origin = originElement->getAttribute("val");
    }

    // Extract theta sign axis if specified.
    XMLString s_thetaSign(s_pointingUp);
    if (thetaSignElement) {
      s_thetaSign = thetaSignElement->getAttribute("axis");
    }

    // Convert to input types
    PointingAlong alongBeam = axisNameToAxisType("along-beam", s_alongBeam);
    PointingAlong pointingUp = axisNameToAxisType("pointing-up", s_pointingUp);
    PointingAlong thetaSign = axisNameToAxisType("theta-sign", s_thetaSign);
    Handedness handedness = s_handedness == "right" ? Right : Left;

    // Overwrite the default reference frame.
    m_instrument->setReferenceFrame(
        std::make_shared<ReferenceFrame>(pointingUp, alongBeam, thetaSign, handedness, s_origin));
  }
}

std::vector<std::string> InstrumentDefinitionParser::buildExcludeList(const Poco::XML::Element *const location) {
  // check if <exclude> sub-elements for this location and create new exclude
  // list to pass on
  Poco::AutoPtr<NodeList> pNLexclude = location->getElementsByTagName("exclude");
  unsigned long numberExcludeEle = pNLexclude->length();
  std::vector<std::string> newExcludeList;
  for (unsigned long i = 0; i < numberExcludeEle; i++) {
    auto *pExElem = static_cast<Element *>(pNLexclude->item(i));
    if (pExElem->hasAttribute("sub-part"))
      newExcludeList.emplace_back(pExElem->getAttribute("sub-part"));
  }

  return newExcludeList;
}

//-----------------------------------------------------------------------------------------------------------------------
/** Assumes second argument is a XML location element and its parent is a
 *component element
 *  which is assigned to be an assembly. This method appends the parent
 *component element of the location element to the CompAssembly passed as the
 *1st arg. Note this method may call itself, i.e. it may act recursively.
 *
 *  @param parent :: CompAssembly to append new component to
 *  @param pLocElem ::  Poco::XML element that points to a location element in
 *an instrument description XML file, which optionally may be detached (meaning
 *it is not required to be part of the DOM tree of the IDF)
 *  @param pCompElem :: The Poco::XML \<component\> element that contains the
 *\<location\> element
 *  @param idList :: The current IDList
 */
void InstrumentDefinitionParser::appendAssembly(Geometry::ICompAssembly *parent, const Poco::XML::Element *pLocElem,
                                                const Poco::XML::Element *pCompElem, IdList &idList) {
  const std::string filename = m_xmlFile->getFileFullPathStr();
  // The location element is required to be a child of a component element. Get
  // this component element
  // Element* pCompElem =
  // InstrumentDefinitionParser::getParentComponent(pLocElem);

  // Read detector IDs into idlist if required
  // Note idlist may be defined for any component
  // Note any new idlist found will take precedence.

  if (pCompElem->hasAttribute("idlist")) {
    std::string idlist = pCompElem->getAttribute("idlist");

    if (idlist != idList.idname) {
      Element *pFound = pCompElem->ownerDocument()->getElementById(idlist, "idname");

      if (pFound == nullptr) {
        throw Kernel::Exception::InstrumentDefinitionError(
            "No <idlist> with name idname=\"" + idlist + "\" present in instrument definition file.", filename);
      }
      idList.reset();
      populateIdList(pFound, idList);
    }
  }

  // Create the assembly that will be appended into the parent.
  Geometry::ICompAssembly *ass;
  // The newly added component is required to have a type. Find out what this
  // type is and find all the location elements of this type. Finally loop over
  // these
  // location elements

  Element *pType = getTypeElement[pCompElem->getAttribute("type")];
  std::string category;
  if (pType->hasAttribute("is"))
    category = pType->getAttribute("is");
  if (category == "SamplePos" || category == "samplePos") {
    ass = new Geometry::CompAssembly(InstrumentDefinitionParser::getNameOfLocationElement(pLocElem, pCompElem), parent);
  } else if (pType->hasAttribute("outline") && pType->getAttribute("outline") != "no") {
    ass = new Geometry::ObjCompAssembly(InstrumentDefinitionParser::getNameOfLocationElement(pLocElem, pCompElem),
                                        parent);
  } else {
    ass = new Geometry::CompAssembly(InstrumentDefinitionParser::getNameOfLocationElement(pLocElem, pCompElem), parent);
  }

  // set location for this newly added comp and set facing if specified in
  // instrument def. file. Also
  // check if any logfiles are referred to through the <parameter> element.

  setLocation(ass, pLocElem, m_angleConvertConst, m_deltaOffsets);
  setSideBySideViewLocation(ass, pCompElem);
  setFacing(ass, pLocElem);
  setLogfile(ass, pCompElem,
             m_instrument->getLogfileCache()); // params specified within <component>
  setLogfile(ass, pLocElem,
             m_instrument->getLogfileCache()); // params specified within specific <location>

  // check if special Component
  if (category == "SamplePos" || category == "samplePos") {
    m_instrument->markAsSamplePos(ass);
  }
  if (category == "Source" || category == "source") {
    m_instrument->markAsSource(ass);
  }

  // If enabled, check for a 'neutronic position' tag and add to cache if found
  if (m_indirectPositions) {
    Element *neutronic = pLocElem->getChildElement("neutronic");
    if (neutronic)
      m_neutronicPos[ass] = neutronic;
  }

  // Check for <exclude> tags for this location
  const std::vector<std::string> excludeList = buildExcludeList(pLocElem);

  NodeIterator it(pType, NodeFilter::SHOW_ELEMENT);

  Node *pNode = it.nextNode();
  while (pNode) {
    if (pNode->nodeName() == "location") {
      // pLocElem is the location of a type. This type is here an assembly and
      // pElem below is a <location> within this type
      const Element *pElem = static_cast<Element *>(pNode);

      // get the parent of pElem, i.e. a pointer to the <component> element that
      // contains pElem
      const Element *pParentElem = InstrumentDefinitionParser::getParentComponent(pElem);

      // check if this location is in the exclude list
      auto inExcluded = find(excludeList.cbegin(), excludeList.cend(),
                             InstrumentDefinitionParser::getNameOfLocationElement(pElem, pParentElem));
      if (inExcluded == excludeList.end()) {

        std::string typeName = (InstrumentDefinitionParser::getParentComponent(pElem))->getAttribute("type");

        if (isAssembly(typeName)) {
          appendAssembly(ass, pElem, pParentElem, idList);
        } else {
          appendLeaf(ass, pElem, pParentElem, idList);
        }
      }
    }
    if (pNode->nodeName() == "locations") {
      const Element *pLocationsElems = static_cast<Element *>(pNode);
      const Element *pParentLocationsElem = InstrumentDefinitionParser::getParentComponent(pLocationsElems);

      // append <locations> elements in <locations>
      appendLocations(ass, pLocationsElems, pParentLocationsElem, idList);
    }
    pNode = it.nextNode();
  }

  // create outline object for the assembly
  if (pType->hasAttribute("outline") && pType->getAttribute("outline") != "no") {
    auto *objAss = dynamic_cast<Geometry::ObjCompAssembly *>(ass);
    if (!objAss) {
      throw std::logic_error("Failed to cast ICompAssembly object to ObjCompAssembly");
    }
    if (pType->getAttribute("object_created") == "no") {
      pType->setAttribute("object_created", "yes");
      std::shared_ptr<Geometry::IObject> obj = objAss->createOutline();
      if (obj) {
        mapTypeNameToShape[pType->getAttribute("name")] = obj;
      } else { // object failed to be created
        pType->setAttribute("outline", "no");
        g_log.warning() << "Failed to create outline object for assembly " << pType->getAttribute("name") << '\n';
      }
    } else {
      objAss->setOutline(mapTypeNameToShape[pType->getAttribute("name")]);
    }
  }
}

void InstrumentDefinitionParser::createDetectorOrMonitor(Geometry::ICompAssembly *parent,
                                                         const Poco::XML::Element *pLocElem,
                                                         const Poco::XML::Element *pCompElem,
                                                         const std::string &filename, IdList &idList,
                                                         const std::string &category) {

  //-------------- Create a Detector
  //------------------------------------------------
  std::string name = InstrumentDefinitionParser::getNameOfLocationElement(pLocElem, pCompElem);

  // before setting detector ID check that the IDF satisfies the following

  if (idList.counted >= static_cast<int>(idList.vec.size())) {
    std::stringstream ss1, ss2;
    ss1 << idList.vec.size();
    ss2 << idList.counted;
    if (idList.idname.empty()) {
      g_log.error("No list of detector IDs found for location element " + name);
      throw Kernel::Exception::InstrumentDefinitionError("Detector location element " + name + " has no idlist.",
                                                         filename);
    } else if (idList.vec.empty()) {
      g_log.error("No detector IDs found for detectors in list " + idList.idname);
    } else {
      g_log.error("The number of detector IDs listed in idlist named " + idList.idname +
                  " is less then the number of detectors");
    }
    throw Kernel::Exception::InstrumentDefinitionError(
        "Number of IDs listed in idlist (=" + ss1.str() + ") is less than the number of detectors.", filename);
  }

  std::string typeName = pCompElem->getAttribute("type");

  // Create detector and increment id. Finally add the detector to the parent
  Geometry::Detector *detector =
      new Geometry::Detector(name, idList.vec[idList.counted], mapTypeNameToShape[typeName], parent);
  idList.counted++;
  parent->add(detector);

  // set location for this newly added comp and set facing if specified in
  // instrument def. file. Also
  // check if any logfiles are referred to through the <parameter> element.
  setLocation(detector, pLocElem, m_angleConvertConst, m_deltaOffsets);
  setFacing(detector, pLocElem);
  setLogfile(detector, pCompElem,
             m_instrument->getLogfileCache()); // params specified within <component>
  setLogfile(detector, pLocElem,
             m_instrument->getLogfileCache()); // params specified within specific <location>

  // If enabled, check for a 'neutronic position' tag and add to cache
  // (null pointer added INTENTIONALLY if not found)
  if (m_indirectPositions) {
    m_neutronicPos[detector] = pLocElem->getChildElement("neutronic");
  }

  // mark-as is a deprecated attribute used before is="monitor" was introduced
  if (pCompElem->hasAttribute("mark-as") || pLocElem->hasAttribute("mark-as")) {
    g_log.warning() << "Attribute 'mark-as' is a deprecated attribute in "
                       "Instrument Definition File."
                    << " Please see the deprecated section of "
                       "docs.mantidproject.org/concepts/InstrumentDefinitionFile for how to remove this "
                       "warning message\n";
  }

  try {
    if (category == "Monitor" || category == "monitor")
      m_instrument->markAsMonitor(detector);
    else {
      // for backwards compatibility look for mark-as="monitor"
      if ((pCompElem->hasAttribute("mark-as") && pCompElem->getAttribute("mark-as") == "monitor") ||
          (pLocElem->hasAttribute("mark-as") && pLocElem->getAttribute("mark-as") == "monitor")) {
        m_instrument->markAsMonitor(detector);
      } else
        m_instrument->markAsDetectorIncomplete(detector);
    }

  } catch (Kernel::Exception::ExistsError &) {
    std::stringstream convert;
    convert << detector->getID();
    throw Kernel::Exception::InstrumentDefinitionError(
        "Detector with ID = " + convert.str() + " present more then once in XML instrument file", filename);
  }

  // Add all monitors and detectors to 'facing component' container. This is
  // only used if the
  // "facing" elements are defined in the instrument definition file
  m_facingComponent.emplace_back(detector);

  setSideBySideViewLocation(detector, pCompElem);
}

void InstrumentDefinitionParser::createGridDetector(Geometry::ICompAssembly *parent, const Poco::XML::Element *pLocElem,
                                                    const Poco::XML::Element *pCompElem, const std::string &filename,
                                                    const Poco::XML::Element *pType) {

  //-------------- Create a GridDetector
  //------------------------------------------------
  std::string name = InstrumentDefinitionParser::getNameOfLocationElement(pLocElem, pCompElem);

  // Create the bank with the given parent.
  auto bank = new Geometry::GridDetector(name, parent);

  // set location for this newly added comp and set facing if specified in
  // instrument def. file. Also
  // check if any logfiles are referred to through the <parameter> element.
  setLocation(bank, pLocElem, m_angleConvertConst, m_deltaOffsets);
  setFacing(bank, pLocElem);
  setLogfile(bank, pCompElem,
             m_instrument->getLogfileCache()); // params specified within <component>
  setLogfile(bank, pLocElem,
             m_instrument->getLogfileCache()); // params specified within specific <location>

  // Extract all the parameters from the XML attributes
  int xpixels = 0;
  int ypixels = 0;
  int zpixels = 0;
  int idstart = 0;
  std::string idfillorder;
  int idstepbyrow = 0;
  int idstep = 1;

  // The shape!
  // Given that this leaf component is actually an assembly, its constituent
  // component detector shapes comes from its type attribute.
  const std::string shapeType = pType->getAttribute("type");
  std::shared_ptr<Geometry::IObject> shape = mapTypeNameToShape[shapeType];
  // These parameters are in the TYPE defining RectangularDetector
  if (pType->hasAttribute("xpixels"))
    xpixels = std::stoi(pType->getAttribute("xpixels"));
  double xstart = attrToDouble(pType, "xstart");
  double xstep = attrToDouble(pType, "xstep");

  if (pType->hasAttribute("ypixels"))
    ypixels = std::stoi(pType->getAttribute("ypixels"));
  double ystart = attrToDouble(pType, "ystart");
  double ystep = attrToDouble(pType, "ystep");

  if (pType->hasAttribute("zpixels"))
    zpixels = std::stoi(pType->getAttribute("zpixels"));
  double zstart = attrToDouble(pType, "zstart");
  double zstep = attrToDouble(pType, "zstep");

  // THESE parameters are in the INSTANCE of this type - since they will
  // change.
  if (pCompElem->hasAttribute("idstart"))
    idstart = std::stoi(pCompElem->getAttribute("idstart"));
  if (pCompElem->hasAttribute("idfillorder"))
    idfillorder = pCompElem->getAttribute("idfillorder");
  // Default ID row step size
  if (!idfillorder.empty() && idfillorder[0] == 'x')
    idstepbyrow = xpixels;
  else if (!idfillorder.empty() && idfillorder[0] == 'y')
    idstepbyrow = ypixels;
  else
    idstepbyrow = zpixels;

  if (pCompElem->hasAttribute("idstepbyrow")) {
    idstepbyrow = std::stoi(pCompElem->getAttribute("idstepbyrow"));
  }
  // Default ID row step size
  if (pCompElem->hasAttribute("idstep"))
    idstep = std::stoi(pCompElem->getAttribute("idstep"));

  setSideBySideViewLocation(bank, pCompElem);

  // Now, initialize all the pixels in the bank
  bank->initialize(shape, xpixels, xstart, xstep, ypixels, ystart, ystep, zpixels, zstart, zstep, idstart, idfillorder,
                   idstepbyrow, idstep);

  // Loop through all detectors in the newly created bank and mark those in
  // the instrument.
  try {
    for (int z = 0; z < bank->nelements(); ++z) {
      auto zLayer = std::dynamic_pointer_cast<Geometry::ICompAssembly>((*bank)[z]);
      for (int x = 0; x < zLayer->nelements(); ++x) {
        auto xColumn = std::dynamic_pointer_cast<Geometry::ICompAssembly>((*zLayer)[x]);
        for (int y = 0; y < xColumn->nelements(); ++y) {
          std::shared_ptr<Geometry::Detector> detector = std::dynamic_pointer_cast<Geometry::Detector>((*xColumn)[y]);
          if (detector) {
            // Make default facing for the pixel
            auto *comp = static_cast<IComponent *>(detector.get());
            if (m_haveDefaultFacing)
              makeXYplaneFaceComponent(comp, m_defaultFacing);
            // Mark it as a detector (add to the instrument cache)
            m_instrument->markAsDetectorIncomplete(detector.get());
          }
        }
      }
    }
  } catch (Kernel::Exception::ExistsError &) {
    throw Kernel::Exception::InstrumentDefinitionError("Duplicate detector ID found when adding GridDetector " + name +
                                                       " in XML instrument file" + filename);
  }
}

void InstrumentDefinitionParser::createRectangularDetector(Geometry::ICompAssembly *parent,
                                                           const Poco::XML::Element *pLocElem,
                                                           const Poco::XML::Element *pCompElem,
                                                           const std::string &filename,
                                                           const Poco::XML::Element *pType) {
  //-------------- Create a RectangularDetector
  //------------------------------------------------
  std::string name = InstrumentDefinitionParser::getNameOfLocationElement(pLocElem, pCompElem);

  // Create the bank with the given parent.
  auto bank = new Geometry::RectangularDetector(name, parent);

  // set location for this newly added comp and set facing if specified in
  // instrument def. file. Also
  // check if any logfiles are referred to through the <parameter> element.
  setLocation(bank, pLocElem, m_angleConvertConst, m_deltaOffsets);
  setFacing(bank, pLocElem);
  setLogfile(bank, pCompElem,
             m_instrument->getLogfileCache()); // params specified within <component>
  setLogfile(bank, pLocElem,
             m_instrument->getLogfileCache()); // params specified within specific <location>

  // Extract all the parameters from the XML attributes
  int xpixels = 0;
  int ypixels = 0;
  int idstart = 0;
  bool idfillbyfirst_y = true;
  int idstepbyrow = 0;
  int idstep = 1;

  // The shape!
  // Given that this leaf component is actually an assembly, its constituent
  // component detector shapes comes from its type attribute.
  const std::string shapeType = pType->getAttribute("type");
  std::shared_ptr<Geometry::IObject> shape = mapTypeNameToShape[shapeType];

  // These parameters are in the TYPE defining RectangularDetector
  if (pType->hasAttribute("xpixels"))
    xpixels = std::stoi(pType->getAttribute("xpixels"));
  double xstart = attrToDouble(pType, "xstart");
  double xstep = attrToDouble(pType, "xstep");

  if (pType->hasAttribute("ypixels"))
    ypixels = std::stoi(pType->getAttribute("ypixels"));
  double ystart = attrToDouble(pType, "ystart");
  double ystep = attrToDouble(pType, "ystep");

  // THESE parameters are in the INSTANCE of this type - since they will
  // change.
  if (pCompElem->hasAttribute("idstart"))
    idstart = std::stoi(pCompElem->getAttribute("idstart"));
  if (pCompElem->hasAttribute("idfillbyfirst"))
    idfillbyfirst_y = (pCompElem->getAttribute("idfillbyfirst") == "y");
  // Default ID row step size
  if (idfillbyfirst_y)
    idstepbyrow = ypixels;
  else
    idstepbyrow = xpixels;
  if (pCompElem->hasAttribute("idstepbyrow")) {
    idstepbyrow = std::stoi(pCompElem->getAttribute("idstepbyrow"));
  }
  // Default ID row step size
  if (pCompElem->hasAttribute("idstep"))
    idstep = std::stoi(pCompElem->getAttribute("idstep"));

  setSideBySideViewLocation(bank, pCompElem);

  // Now, initialize all the pixels in the bank
  bank->initialize(shape, xpixels, xstart, xstep, ypixels, ystart, ystep, idstart, idfillbyfirst_y, idstepbyrow,
                   idstep);

  // Loop through all detectors in the newly created bank and mark those in
  // the instrument.
  try {
    for (int x = 0; x < bank->nelements(); x++) {
      std::shared_ptr<Geometry::ICompAssembly> xColumn = std::dynamic_pointer_cast<Geometry::ICompAssembly>((*bank)[x]);
      for (int y = 0; y < xColumn->nelements(); y++) {
        std::shared_ptr<Geometry::Detector> detector = std::dynamic_pointer_cast<Geometry::Detector>((*xColumn)[y]);
        if (detector) {
          // Make default facing for the pixel
          auto *comp = static_cast<IComponent *>(detector.get());
          if (m_haveDefaultFacing)
            makeXYplaneFaceComponent(comp, m_defaultFacing);
          // Mark it as a detector (add to the instrument cache)
          m_instrument->markAsDetectorIncomplete(detector.get());
        }
      }
    }
  } catch (Kernel::Exception::ExistsError &) {
    throw Kernel::Exception::InstrumentDefinitionError("Duplicate detector ID found when adding RectangularDetector " +
                                                       name + " in XML instrument file" + filename);
  }
}

void InstrumentDefinitionParser::createStructuredDetector(Geometry::ICompAssembly *parent,
                                                          const Poco::XML::Element *pLocElem,
                                                          const Poco::XML::Element *pCompElem,
                                                          const std::string &filename,
                                                          const Poco::XML::Element *pType) {
  //-------------- Create a StructuredDetector
  //------------------------------------------------
  std::string name = InstrumentDefinitionParser::getNameOfLocationElement(pLocElem, pCompElem);

  // Create the bank with the given parent.
  auto bank = new Geometry::StructuredDetector(name, parent);

  // set location for this newly added comp and set facing if specified in
  // instrument def. file. Also
  // check if any logfiles are referred to through the <parameter> element.
  setLocation(bank, pLocElem, m_angleConvertConst, m_deltaOffsets);
  setLogfile(bank, pCompElem,
             m_instrument->getLogfileCache()); // params specified within <component>
  setLogfile(bank, pLocElem,
             m_instrument->getLogfileCache()); // params specified within specific <location>

  // Extract all the parameters from the XML attributes
  int xpixels = 0;
  int ypixels = 0;
  int idstart = 0;
  bool idfillbyfirst_y = true;
  int idstepbyrow = 0;
  int idstep = 1;
  std::vector<double> xValues;
  std::vector<double> yValues;

  std::string typeName = pType->getAttribute("name");
  // These parameters are in the TYPE defining StructuredDetector
  if (pType->hasAttribute("xpixels"))
    xpixels = std::stoi(pType->getAttribute("xpixels"));
  if (pType->hasAttribute("ypixels"))
    ypixels = std::stoi(pType->getAttribute("ypixels"));

  // THESE parameters are in the INSTANCE of this type - since they will
  // change.
  if (pCompElem->hasAttribute("idstart"))
    idstart = std::stoi(pCompElem->getAttribute("idstart"));
  if (pCompElem->hasAttribute("idfillbyfirst"))
    idfillbyfirst_y = (pCompElem->getAttribute("idfillbyfirst") == "y");
  // Default ID row step size
  if (idfillbyfirst_y)
    idstepbyrow = ypixels;
  else
    idstepbyrow = xpixels;
  if (pCompElem->hasAttribute("idstepbyrow")) {
    idstepbyrow = std::stoi(pCompElem->getAttribute("idstepbyrow"));
  }
  // Default ID row step size
  if (pCompElem->hasAttribute("idstep"))
    idstep = std::stoi(pCompElem->getAttribute("idstep"));

  // Access type element which defines structured detecor vertices
  Element *pElem = nullptr;
  NodeIterator tags(pCompElem->ownerDocument(), NodeFilter::SHOW_ELEMENT);
  Node *pNode = tags.nextNode();

  while (pNode) {
    auto *check = static_cast<Element *>(pNode);
    if (pNode->nodeName() == "type" && check->hasAttribute("is")) {
      std::string is = check->getAttribute("is");
      if (StructuredDetector::compareName(is) && typeName == check->getAttribute("name")) {
        pElem = check;
        break;
      }
    }

    pNode = tags.nextNode();
  }

  if (pElem == nullptr)
    throw Kernel::Exception::InstrumentDefinitionError("No <type> with attribute is=\"StructuredDetector\"", filename);

  // Ensure vertices are present within the IDF
  Poco::AutoPtr<NodeList> pNL = pElem->getElementsByTagName("vertex");
  if (pNL->length() == 0)
    throw Kernel::Exception::InstrumentDefinitionError("StructuredDetector must contain vertices.", filename);

  NodeIterator it(pElem, NodeFilter::SHOW_ELEMENT);

  pNode = it.nextNode();

  while (pNode) {
    if (pNode->nodeName() == "vertex") {
      auto *pVertElem = static_cast<Element *>(pNode);

      if (pVertElem->hasAttribute("x"))
        xValues.emplace_back(attrToDouble(pVertElem, "x"));
      if (pVertElem->hasAttribute("y"))
        yValues.emplace_back(attrToDouble(pVertElem, "y"));
    }

    pNode = it.nextNode();
  }

  V3D zVector(0, 0, 1); // Z aligned beam
  bool isZBeam = m_instrument->getReferenceFrame()->isVectorPointingAlongBeam(zVector);
  // Now, initialize all the pixels in the bank
  bank->initialize(xpixels, ypixels, std::move(xValues), std::move(yValues), isZBeam, idstart, idfillbyfirst_y,
                   idstepbyrow, idstep);

  // Loop through all detectors in the newly created bank and mark those in
  // the instrument.
  try {
    for (int x = 0; x < bank->nelements(); x++) {
      std::shared_ptr<Geometry::ICompAssembly> xColumn = std::dynamic_pointer_cast<Geometry::ICompAssembly>((*bank)[x]);
      for (int y = 0; y < xColumn->nelements(); y++) {
        std::shared_ptr<Geometry::Detector> detector = std::dynamic_pointer_cast<Geometry::Detector>((*xColumn)[y]);
        if (detector) {
          // Make default facing for the pixel
          auto *comp = static_cast<IComponent *>(detector.get());
          if (m_haveDefaultFacing)
            makeXYplaneFaceComponent(comp, m_defaultFacing);
          // Mark it as a detector (add to the instrument cache)
          m_instrument->markAsDetectorIncomplete(detector.get());
        }
      }
    }
  } catch (Kernel::Exception::ExistsError &) {
    throw Kernel::Exception::InstrumentDefinitionError("Duplicate detector ID found when adding StructuredDetector " +
                                                       name + " in XML instrument file" + filename);
  }
}

//-----------------------------------------------------------------------------------------------------------------------
/** Assumes second argument is pointing to a leaf, which here means the
 *location
 *element (indirectly
 *  representing a component element) that contains no sub-components. This
 *component is appended
 *  to the parent (1st argument).
 *
 *  @param parent :: CompAssembly to append component to
 *  @param pLocElem ::  Poco::XML element that points to the element in the XML
 *doc we want to add, which optionally may be detached (meaning it is not
 *required to be part of the DOM tree of the IDF)
 *  @param pCompElem :: The Poco::XML \<component\> element that contains the
 *\<location\> element, which may optionally be detached from the DOM tree also
 *  @param idList :: The current IDList
 *
 *  @throw InstrumentDefinitionError Thrown if issues with the content of XML
 *instrument file
 */
void InstrumentDefinitionParser::appendLeaf(Geometry::ICompAssembly *parent, const Poco::XML::Element *pLocElem,
                                            const Poco::XML::Element *pCompElem, IdList &idList) {
  const std::string filename = m_xmlFile->getFileFullPathStr();

  //--- Get the detector's X/Y pixel sizes (optional) ---
  // Read detector IDs into idlist if required
  // Note idlist may be defined for any component
  // Note any new idlist found will take precedence.

  if (pCompElem->hasAttribute("idlist")) {
    std::string idlist = pCompElem->getAttribute("idlist");

    if (idlist != idList.idname) {
      Element *pFound = pCompElem->ownerDocument()->getElementById(idlist, "idname");

      if (pFound == nullptr) {
        throw Kernel::Exception::InstrumentDefinitionError(
            "No <idlist> with name idname=\"" + idlist + "\" present in instrument definition file.", filename);
      }

      idList.reset();
      populateIdList(pFound, idList);
    }
  }

  // get the type element of the component element in order to determine if
  // the
  // type
  // belong to the category: "detector", "SamplePos or "Source".

  std::string typeName = pCompElem->getAttribute("type");
  Element *pType = getTypeElement[typeName];

  std::string category;
  if (pType->hasAttribute("is"))
    category = pType->getAttribute("is");

  static const boost::regex exp("Detector|detector|Monitor|monitor");

  // do stuff a bit differently depending on which category the type belong to
  if (GridDetector::compareName(category)) {
    createGridDetector(parent, pLocElem, pCompElem, filename, pType);
  } else if (RectangularDetector::compareName(category)) {
    createRectangularDetector(parent, pLocElem, pCompElem, filename, pType);
  } else if (StructuredDetector::compareName(category)) {
    createStructuredDetector(parent, pLocElem, pCompElem, filename, pType);
  } else if (boost::regex_match(category, exp)) {
    createDetectorOrMonitor(parent, pLocElem, pCompElem, filename, idList, category);
  } else {
    //-------------- Not a Detector, RectangularDetector or Structured Detector
    //------------------------------
    IComponent *comp;
    if (category == "SamplePos" || category == "samplePos") {
      // check if special SamplePos Component
      std::string name = InstrumentDefinitionParser::getNameOfLocationElement(pLocElem, pCompElem);
      comp = new Geometry::Component(name, parent);
      m_instrument->markAsSamplePos(comp);
    } else {
      std::string name = InstrumentDefinitionParser::getNameOfLocationElement(pLocElem, pCompElem);

      comp = new Geometry::ObjComponent(name, mapTypeNameToShape[typeName], parent);
    }
    parent->add(comp);

    // check if special Source Component
    if (category == "Source" || category == "source") {
      m_instrument->markAsSource(comp);
    }

    // set location for this newly added comp and set facing if specified in
    // instrument def. file. Also
    // check if any logfiles are referred to through the <parameter> element.

    setLocation(comp, pLocElem, m_angleConvertConst, m_deltaOffsets);
    setFacing(comp, pLocElem);
    setLogfile(comp, pCompElem,
               m_instrument->getLogfileCache()); // params specified within <component>
    setLogfile(comp, pLocElem,
               m_instrument->getLogfileCache()); // params specified within
                                                 // specific <location>
  }
}

//-----------------------------------------------------------------------------------------------------------------------
/** Method for populating IdList.
 *
 *  @param pE ::  Poco::XML element that points to an \<idlist\>
 *  @param idList :: The structure to populate with detector ID numbers
 *
 *  @throw logic_error Thrown if argument is not a child of component element
 *  @throw InstrumentDefinitionError Thrown if issues with the content of XML
 *instrument file
 */
void InstrumentDefinitionParser::populateIdList(Poco::XML::Element *pE, IdList &idList) {
  const std::string filename = m_xmlFile->getFileFullPathStr();

  if ((pE->tagName()) != "idlist") {
    g_log.error("Argument to function createIdList must be a pointer to an XML "
                "element with tag name idlist.");
    throw std::logic_error("Argument to function createIdList must be a "
                           "pointer to an XML element with tag name idlist.");
  }

  // set name of idlist

  idList.idname = pE->getAttribute("idname");

  // If idname element has start and end attributes then just use those to
  // populate idlist.
  // Otherwise id sub-elements

  if (pE->hasAttribute("start")) {
    int startID = std::stoi(pE->getAttribute("start"));

    int endID;
    if (pE->hasAttribute("end"))
      endID = std::stoi(pE->getAttribute("end"));
    else
      endID = startID;

    int increment = 1;
    if (pE->hasAttribute("step"))
      increment = std::stoi(pE->getAttribute("step"));

    if (0 == increment) {
      std::stringstream ss;
      ss << "The step element cannot be zero, got start: " << startID << ", end: " << endID << ", step: " << increment;
      throw Kernel::Exception::InstrumentDefinitionError(ss.str(), filename);
    }

    // check the start end and increment values are sensible
    int steps = (endID - startID) / increment;
    if (steps < 0) {
      std::stringstream ss;
      ss << "The start, end, and step elements do not allow a single id in "
            "the "
            "idlist entry - ";
      ss << "start: " << startID << ",  end: " << endID << ", step: " << increment;

      throw Kernel::Exception::InstrumentDefinitionError(ss.str(), filename);
    }

    idList.vec.reserve(steps);
    for (int i = startID; i != endID + increment; i += increment) {
      idList.vec.emplace_back(i);
    }
  } else {
    // test first if any <id> elements

    Poco::AutoPtr<NodeList> pNL = pE->getElementsByTagName("id");

    if (pNL->length() == 0) {
      throw Kernel::Exception::InstrumentDefinitionError("No id subelement of idlist element in XML instrument file",
                                                         filename);
    }

    // get id numbers

    NodeIterator it(pE, NodeFilter::SHOW_ELEMENT);

    Node *pNode = it.nextNode();
    while (pNode) {
      if (pNode->nodeName() == "id") {
        auto *pIDElem = static_cast<Element *>(pNode);

        if (pIDElem->hasAttribute("val")) {
          int valID = std::stoi(pIDElem->getAttribute("val"));
          idList.vec.emplace_back(valID);
        } else if (pIDElem->hasAttribute("start")) {
          int startID = std::stoi(pIDElem->getAttribute("start"));

          int endID;
          if (pIDElem->hasAttribute("end"))
            endID = std::stoi(pIDElem->getAttribute("end"));
          else
            endID = startID;

          int increment = 1;
          if (pIDElem->hasAttribute("step"))
            increment = std::stoi(pIDElem->getAttribute("step"));

          // check the start end and increment values are sensible
          if (0 == increment) {
            std::stringstream ss;
            ss << "The step element cannot be zero, found step: " << increment;

            throw Kernel::Exception::InstrumentDefinitionError(ss.str(), filename);
          }
          int numSteps = (endID - startID) / increment;
          if (numSteps < 0) {
            std::stringstream ss;
            ss << "The start, end, and step elements do not allow a single "
                  "id "
                  "in the idlist entry - ";
            ss << "start: " << startID << ",  end: " << endID << ", step: " << increment;

            throw Kernel::Exception::InstrumentDefinitionError(ss.str(), filename);
          }

          idList.vec.reserve(numSteps);
          for (int i = startID; i != endID + increment; i += increment) {
            idList.vec.emplace_back(i);
          }
        } else {
          throw Kernel::Exception::InstrumentDefinitionError(
              "id subelement of idlist " + std::string("element wrongly specified in XML instrument file"), filename);
        }
      }

      pNode = it.nextNode();
    } // end while loop
  }
}

//-----------------------------------------------------------------------------------------------------------------------
/** Returns True if the (string) type given is an assembly.
 *
 *  @param type ::  name of the type of a component in XML instrument
 *definition
 *  @return True if the type is an assembly
 *  @throw InstrumentDefinitionError Thrown if type not defined in XML
 *definition
 */
bool InstrumentDefinitionParser::isAssembly(const std::string &type) const {
  const std::string filename = m_xmlFile->getFileFullPathStr();
  auto it = isTypeAssembly.find(type);

  if (it == isTypeAssembly.end()) {
    throw Kernel::Exception::InstrumentDefinitionError("type with name = " + type + " not defined.", filename);
  }

  return it->second;
}

//-----------------------------------------------------------------------------------------------------------------------
/** Make the shape defined in 1st argument face the component in the second
 *argument,
 *  by rotating the z-axis of the component passed in 1st argument so that it
 *points in the
 *  direction: from the component as specified 2nd argument to the component as
 *specified in 1st argument.
 *
 *  @param in ::  Component to be rotated
 *  @param facing :: Object to face
 */
void InstrumentDefinitionParser::makeXYplaneFaceComponent(Geometry::IComponent *&in,
                                                          const Geometry::ObjComponent *facing) {
  makeXYplaneFaceComponent(in, facing->getPos());
}

//-----------------------------------------------------------------------------------------------------------------------
/** Make the shape defined in 1st argument face the position in the second
 * argument, by rotating the z-axis of the component passed in 1st argument so
 * that it points in the direction: from the position (as specified 2nd
 * argument) to the component (1st argument).
 *
 *  @param in ::  Component to be rotated
 *  @param facingPoint :: position to face
 */
void InstrumentDefinitionParser::makeXYplaneFaceComponent(Geometry::IComponent *&in, const Kernel::V3D &facingPoint) {
  Kernel::V3D pos = in->getPos();

  // vector from facing object to component we want to rotate
  Kernel::V3D facingDirection = pos - facingPoint;
  const auto facingDirLength = facingDirection.norm();
  if (facingDirLength == 0.0)
    return;
  facingDirection /= facingDirLength;

  // now aim to rotate shape such that the z-axis of of the object we want to
  // rotate points in the direction of facingDirection. That way the XY plane
  // faces the 'facing object'.
  constexpr Kernel::V3D z(0, 0, 1);
  Kernel::Quat R = in->getRotation();
  R.inverse();
  R.rotate(facingDirection);

  Kernel::V3D normal = facingDirection.cross_prod(z);
  const auto normalLength = normal.norm();
  if (normalLength == 0.) {
    normal = normalize(-facingDirection);
  } else {
    normal /= normalLength;
  }
  double theta = (180.0 / M_PI) * facingDirection.angle(z);

  if (normal.norm() > 0.0)
    in->rotate(Kernel::Quat(-theta, normal));
  else {
    // To take into account the case where the facing direction is in the
    // (0,0,1) or (0,0,-1) direction.
    in->rotate(Kernel::Quat(-theta, Kernel::V3D(0, 1, 0)));
  }
}

//-----------------------------------------------------------------------------------------------------------------------
/** Parse position of facing element to V3D
 *
 *  @param pElem ::  Facing type element to parse
 *  @return Return parsed position as a V3D
 */
Kernel::V3D InstrumentDefinitionParser::parseFacingElementToV3D(Poco::XML::Element *pElem) {
  Kernel::V3D retV3D;

  // Polar coordinates can be labelled as (r,t,p) or (R,theta,phi)
  if (pElem->hasAttribute("r") || pElem->hasAttribute("t") || pElem->hasAttribute("p") || pElem->hasAttribute("R") ||
      pElem->hasAttribute("theta") || pElem->hasAttribute("phi")) {
    double R = attrToDouble(pElem, "r");
    double theta = m_angleConvertConst * attrToDouble(pElem, "t");
    double phi = m_angleConvertConst * attrToDouble(pElem, "p");

    if (pElem->hasAttribute("R"))
      R = attrToDouble(pElem, "R");
    if (pElem->hasAttribute("theta"))
      theta = m_angleConvertConst * attrToDouble(pElem, "theta");
    if (pElem->hasAttribute("phi"))
      phi = m_angleConvertConst * attrToDouble(pElem, "phi");

    retV3D.spherical(R, theta, phi);
  } else {
    double x = attrToDouble(pElem, "x");
    double y = attrToDouble(pElem, "y");
    double z = attrToDouble(pElem, "z");

    retV3D(x, y, z);
  }

  return retV3D;
}

//-----------------------------------------------------------------------------------------------------------------------
/** Set facing of comp as specified in XML facing element (which must be
 *sub-element of a location element).
 *
 *  @param comp :: To set facing of
 *  @param pElem ::  Poco::XML element that points a \<location\> element,
 *which
 *optionally may be detached (meaning it is not required to be part of the DOM
 *tree of the IDF)
 *
 *  @throw logic_error Thrown if second argument is not a pointer to a
 *'location'
 *XML element
 */
void InstrumentDefinitionParser::setFacing(Geometry::IComponent *comp, const Poco::XML::Element *pElem) {
  // Require that pElem points to an element with tag name 'location'

  if ((pElem->tagName()) != "location") {
    g_log.error("Second argument to function setLocation must be a pointer to "
                "an XML element with tag name location.");
    throw std::logic_error("Second argument to function setLocation must be a "
                           "pointer to an XML element with tag name location.");
  }

  Element *facingElem = pElem->getChildElement("facing");
  if (facingElem) {
    // check if user want to rotate about z-axis before potentially applying
    // facing

    if (facingElem->hasAttribute("rot")) {
      double rotAngle = m_angleConvertConst * attrToDouble(facingElem, "rot"); // assumed to be in degrees
      comp->rotate(Kernel::Quat(rotAngle, Kernel::V3D(0, 0, 1)));
    }

    // For now assume that if has val attribute it means facing = none. This
    // option only has an
    // effect when a default facing setting is set. In which case this then
    // means "ignore the
    // default facing setting" for this component

    if (facingElem->hasAttribute("val"))
      return;

    // Face the component, i.e. rotate the z-axis of the component such that
    // it
    // points in the direction from
    // the point x,y,z (or r,t,p) specified by the <facing> xml element
    // towards
    // the component

    makeXYplaneFaceComponent(comp, parseFacingElementToV3D(facingElem));

  } else // so if no facing element associated with location element apply
         // default facing if set
    if (m_haveDefaultFacing)
      makeXYplaneFaceComponent(comp, m_defaultFacing);
}

//-----------------------------------------------------------------------------------------------------------------------
/** Set parameter/logfile info (if any) associated with component
 *
 *  @param comp :: Some component
 *  @param pElem ::  Poco::XML element that may hold \<parameter\> elements
 *  @param logfileCache :: Cache to add information about parameter to
 *  @param requestedDate :: Reference date to check the validity of the
 * parameter against
 *  @throw InstrumentDefinitionError Thrown if issues with the content of XML
 *instrument file
 */
void InstrumentDefinitionParser::setLogfile(const Geometry::IComponent *comp, const Poco::XML::Element *pElem,
                                            InstrumentParameterCache &logfileCache, const std::string &requestedDate) {
  const std::string filename = m_xmlFile->getFileFullPathStr();

  // The purpose below is to have a quicker way to judge if pElem contains a
  // parameter, see
  // defintion of m_hasParameterElement for more info
  if (m_hasParameterElement_beenSet)
    if (m_hasParameterElement.end() == std::find(m_hasParameterElement.begin(), m_hasParameterElement.end(), pElem))
      return;

  Poco::AutoPtr<NodeList> pNL_comp = pElem->childNodes(); // here get all child nodes
  unsigned long pNL_comp_length = pNL_comp->length();

  for (unsigned long i = 0; i < pNL_comp_length; i++) {
    // we are only interest in the top level parameter elements hence
    // the reason for the if statement below
    if (!((pNL_comp->item(i))->nodeType() == Node::ELEMENT_NODE && ((pNL_comp->item(i))->nodeName()) == "parameter"))
      continue;

    auto *pParamElem = static_cast<Element *>(pNL_comp->item(i));

    if (!pParamElem->hasAttribute("name"))
      throw Kernel::Exception::InstrumentDefinitionError(
          "XML element with name or type = " + comp->getName() +
              " contain <parameter> element with no name attribute in XML "
              "instrument file",
          filename);

    std::string paramName = pParamElem->getAttribute("name");

    if (paramName == "rot" || paramName == "pos") {
      g_log.error() << "XML element with name or type = " << comp->getName()
                    << " contains <parameter> element with name=\"" << paramName << "\"."
                    << " This is a reserved Mantid keyword. Please use other name, "
                    << "and see docs.mantidproject.org/concepts/InstrumentDefinitionFile for list of reserved "
                       "keywords."
                    << " This parameter is ignored";
      continue;
    }

    std::string visible = "true";
    if (pParamElem->hasAttribute("visible")) {
      visible = pParamElem->getAttribute("visible");
    }

    DateAndTime validityDate;

    if (requestedDate.empty()) {
      validityDate = DateAndTime::getCurrentTime();
    } else {
      validityDate.setFromISO8601(requestedDate);
    }

    std::string logfileID;
    std::string value;

    DateAndTime validFrom;
    DateAndTime validTo;

    std::string type = "double";               // default
    std::string extractSingleValueAs = "mean"; // default
    std::string eq;

    Poco::AutoPtr<NodeList> pNLvalue = pParamElem->getElementsByTagName("value");
    size_t numberValueEle = pNLvalue->length();
    Element *pValueElem;

    Poco::AutoPtr<NodeList> pNLlogfile = pParamElem->getElementsByTagName("logfile");
    size_t numberLogfileEle = pNLlogfile->length();
    Element *pLogfileElem;

    Poco::AutoPtr<NodeList> pNLLookUp = pParamElem->getElementsByTagName("lookuptable");
    size_t numberLookUp = pNLLookUp->length();

    Poco::AutoPtr<NodeList> pNLFormula = pParamElem->getElementsByTagName("formula");
    size_t numberFormula = pNLFormula->length();

    if ((numberValueEle > 0 && numberLogfileEle + numberLookUp + numberFormula > 0) ||
        (numberValueEle == 0 && numberLogfileEle + numberLookUp + numberFormula > 1)) {
      g_log.warning() << "XML element with name or type = " << comp->getName()
                      << " contains <parameter> element where the value of the "
                      << "parameter has been specified more than once. See "
                      << "docs.mantidproject.org/concepts/InstrumentDefinitionFile for how the value of the "
                      << "parameter is set in this case.";
    }

    if (numberValueEle + numberLogfileEle + numberLookUp + numberFormula == 0) {
      g_log.error() << "XML element with name or type = " << comp->getName()
                    << " contains <parameter> for which no value is specified."
                    << " See docs.mantidproject.org/concepts/InstrumentDefinitionFile for how to set the value"
                    << " of a parameter. This parameter is ignored.";
      continue;
    }

    DateAndTime currentValidFrom;
    DateAndTime currentValidTo;
    currentValidFrom.setToMinimum();
    currentValidTo.setToMaximum();

    // if more than one <value> specified for a parameter, check the validity
    // range
    if (numberValueEle >= 1) {
      bool hasValue = false;

      for (unsigned long j = 0; j < numberValueEle; ++j) {
        pValueElem = static_cast<Element *>(pNLvalue->item(j));

        if (!pValueElem->hasAttribute(("val")))
          continue;

        validFrom.setToMinimum();
        if (pValueElem->hasAttribute("valid-from"))
          validFrom.setFromISO8601(pValueElem->getAttribute("valid-from"));

        validTo.setToMaximum();
        if (pValueElem->hasAttribute("valid-to"))
          validTo.setFromISO8601(pValueElem->getAttribute("valid-to"));

        if (validFrom <= validityDate && validityDate <= validTo &&
            (validFrom > currentValidFrom || (validFrom == currentValidFrom && validTo <= currentValidTo))) {

          currentValidFrom = validFrom;
          currentValidTo = validTo;
        } else
          continue;
        hasValue = true;
        value = pValueElem->getAttribute("val");
      }

      if (!hasValue) {
        throw Kernel::Exception::InstrumentDefinitionError(
            "XML element with name or type = " + comp->getName() +
                " contains <parameter> element with invalid syntax for its "
                "subelement <value>. Correct syntax is <value val=\"\"/>",
            filename);
      }

    } else if (numberLogfileEle >= 1) {
      // <logfile > tag was used at least once.
      pLogfileElem = static_cast<Element *>(pNLlogfile->item(0));
      if (!pLogfileElem->hasAttribute("id"))
        throw Kernel::Exception::InstrumentDefinitionError(
            "XML element with name or type = " + comp->getName() +
                " contains <parameter> element with invalid syntax for its "
                "subelement logfile>." +
                " Correct syntax is <logfile id=\"\"/>",
            filename);
      logfileID = pLogfileElem->getAttribute("id");

      if (pLogfileElem->hasAttribute("eq"))
        eq = pLogfileElem->getAttribute("eq");
      if (pLogfileElem->hasAttribute("extract-single-value-as"))
        extractSingleValueAs = pLogfileElem->getAttribute("extract-single-value-as");
    }

    if (pParamElem->hasAttribute("type"))
      type = pParamElem->getAttribute("type");

    // check if <fixed /> element present

    bool fixed = false;
    Poco::AutoPtr<NodeList> pNLFixed = pParamElem->getElementsByTagName("fixed");
    size_t numberFixed = pNLFixed->length();
    if (numberFixed >= 1) {
      fixed = true;
    }

    // some processing

    std::string fittingFunction;
    std::string tie;

    if (type == "fitting") {
      size_t found = paramName.find(':');
      if (found != std::string::npos) {
        // check that only one : in name
        size_t index = paramName.find(':', found + 1);
        if (index != std::string::npos) {
          g_log.error() << "Fitting <parameter> in instrument definition file defined "
                           "with"
                        << " more than one column character :. One must used.\n";
        } else {
          fittingFunction = paramName.substr(0, found);
          paramName = paramName.substr(found + 1, paramName.size());
        }
      }
    }

    if (fixed) {
      std::ostringstream str;
      str << paramName << "=" << value;
      tie = str.str();
    }

    // check if <min> or <max> elements present

    std::vector<std::string> constraint(2, "");

    Poco::AutoPtr<NodeList> pNLMin = pParamElem->getElementsByTagName("min");
    size_t numberMin = pNLMin->length();
    Poco::AutoPtr<NodeList> pNLMax = pParamElem->getElementsByTagName("max");
    size_t numberMax = pNLMax->length();

    if (numberMin >= 1) {
      auto *pMin = static_cast<Element *>(pNLMin->item(0));
      constraint[0] = pMin->getAttribute("val");
    }
    if (numberMax >= 1) {
      auto *pMax = static_cast<Element *>(pNLMax->item(0));
      constraint[1] = pMax->getAttribute("val");
    }

    // check if penalty-factor> elements present

    std::string penaltyFactor;

    Poco::AutoPtr<NodeList> pNL_penaltyFactor = pParamElem->getElementsByTagName("penalty-factor");
    size_t numberPenaltyFactor = pNL_penaltyFactor->length();

    if (numberPenaltyFactor >= 1) {
      auto *pPenaltyFactor = static_cast<Element *>(pNL_penaltyFactor->item(0));
      penaltyFactor = pPenaltyFactor->getAttribute("val");
    }

    // Check if look up table is specified

    std::vector<std::string> allowedUnits = UnitFactory::Instance().getKeys();

    std::shared_ptr<Interpolation> interpolation = std::make_shared<Interpolation>();

    if (numberLookUp >= 1) {
      auto *pLookUp = static_cast<Element *>(pNLLookUp->item(0));

      if (pLookUp->hasAttribute("interpolation"))
        interpolation->setMethod(pLookUp->getAttribute("interpolation"));
      if (pLookUp->hasAttribute("x-unit")) {
        std::vector<std::string>::iterator it;
        it = find(allowedUnits.begin(), allowedUnits.end(), pLookUp->getAttribute("x-unit"));
        if (it == allowedUnits.end()) {
          g_log.warning() << "x-unit used with interpolation table must be "
                             "one of the recognised units "
                          << " see http://docs.mantidproject.org/concepts/UnitFactory";
        } else
          interpolation->setXUnit(pLookUp->getAttribute("x-unit"));
      }
      if (pLookUp->hasAttribute("y-unit")) {
        std::vector<std::string>::iterator it;
        it = find(allowedUnits.begin(), allowedUnits.end(), pLookUp->getAttribute("y-unit"));
        if (it == allowedUnits.end()) {
          g_log.warning() << "y-unit used with interpolation table must be "
                             "one of the recognised units "
                          << " see http://docs.mantidproject.org/concepts/UnitFactory";
        } else
          interpolation->setYUnit(pLookUp->getAttribute("y-unit"));
      }

      Poco::AutoPtr<NodeList> pNLpoint = pLookUp->getElementsByTagName("point");
      unsigned long numberPoint = pNLpoint->length();

      for (unsigned long j = 0; j < numberPoint; j++) {
        auto const *pPoint = static_cast<Element *>(pNLpoint->item(j));
        double x = attrToDouble(pPoint, "x");
        double y = attrToDouble(pPoint, "y");
        interpolation->addPoint(x, y);
      }
    }

    // Check if formula is specified

    std::string formula;
    std::string formulaUnit;
    std::string resultUnit;

    if (numberFormula >= 1) {
      auto *pFormula = static_cast<Element *>(pNLFormula->item(0));
      formula = pFormula->getAttribute("eq");
      if (pFormula->hasAttribute("unit")) {
        std::vector<std::string>::iterator it;
        it = find(allowedUnits.begin(), allowedUnits.end(), pFormula->getAttribute("unit"));
        if (it == allowedUnits.end()) {
          g_log.warning() << "unit attribute used with formula must be one "
                             "of the recognized units "
                          << " see http://docs.mantidproject.org/concepts/UnitFactory";
        } else
          formulaUnit = pFormula->getAttribute("unit");
      }
      if (pFormula->hasAttribute("result-unit"))
        resultUnit = pFormula->getAttribute("result-unit");
    }
    // Check if parameter description is
    std::string description;

    Poco::AutoPtr<NodeList> pNLDescription = pParamElem->getElementsByTagName("description");
    size_t numberDescription = pNLDescription->length();

    if (numberDescription >= 1) {
      // use only first description from a list
      auto *pDescription = static_cast<Element *>(pNLDescription->item(0));
      description = pDescription->getAttribute("is");
    }

    auto cacheKey = std::make_pair(paramName, comp);
    auto cacheValue = std::make_shared<XMLInstrumentParameter>(
        logfileID, value, interpolation, formula, formulaUnit, resultUnit, paramName, type, tie, constraint,
        penaltyFactor, fittingFunction, extractSingleValueAs, eq, comp, m_angleConvertConst, description, visible);
    auto inserted = logfileCache.emplace(cacheKey, cacheValue);
    if (!inserted.second) {
      logfileCache[cacheKey] = cacheValue;
    }
  } // end element loop
}

//-----------------------------------------------------------------------------------------------------------------------
/** Apply parameters that may be specified in \<component-link\> XML elements.
 *  Input variable pRootElem may e.g. be the root element of an XML parameter
 *file or
 *  the root element of a IDF
 *
 *  @param instrument :: Instrument
 *  @param pRootElem ::  Associated Poco::XML element that may contain
 *\<component-link\> elements
 *  @param progress :: Optional progress object for reporting progress to an
 *algorithm
 * @param requestedDate :: Optional Date against which to check the validity of
 *an IPF parameter
 */
void InstrumentDefinitionParser::setComponentLinks(std::shared_ptr<Geometry::Instrument> &instrument,
                                                   Poco::XML::Element *pRootElem, Kernel::ProgressBase *progress,
                                                   const std::string &requestedDate) {
  // check if any logfile cache units set. As of this writing the only unit to
  // check is if "angle=radian"
  std::map<std::string, std::string> &units = instrument->getLogfileUnit();
  std::map<std::string, std::string>::iterator unit_it;
  unit_it = units.find("angle");
  if (unit_it != units.end())
    if (unit_it->second == "radian")
      m_angleConvertConst = 180.0 / M_PI;

  const std::string elemName = "component-link";
  Poco::AutoPtr<NodeList> pNL_link = pRootElem->getElementsByTagName(elemName);
  unsigned long numberLinks = pNL_link->length();

  if (progress)
    progress->resetNumSteps(static_cast<int64_t>(numberLinks), 0.0, 0.95);

  Node *curNode = pRootElem->firstChild();
  while (curNode) {
    if (curNode->nodeType() == Node::ELEMENT_NODE && curNode->nodeName() == elemName) {
      auto *curElem = static_cast<Element *>(curNode);

      if (progress) {
        if (progress->hasCancellationBeenRequested())
          return;
        progress->report("Loading parameters");
      }

      std::string id = curElem->getAttribute("id");
      std::string name = curElem->getAttribute("name");
      std::vector<std::shared_ptr<const Geometry::IComponent>> sharedIComp;

      // If available, use the detector id as it's the most specific.
      if (id.length() > 0) {
        int detid;
        std::stringstream(id) >> detid;
        std::shared_ptr<const Geometry::IComponent> detector = instrument->getDetector(static_cast<detid_t>(detid));

        // If we didn't find anything with the detector id, explain why to the
        // user, and throw an exception.
        if (!detector) {
          g_log.error() << "Error whilst loading parameters. No detector "
                           "found with id '"
                        << detid << "'\n";
          g_log.error() << "Please check that your detectors' ids are correct.\n";
          throw Kernel::Exception::InstrumentDefinitionError("Invalid detector id in component-link tag.");
        }

        sharedIComp.emplace_back(detector);

        // If the user also supplied a name, make sure it's consistent with
        // the
        // detector id.
        if (name.length() > 0) {
          auto comp = std::dynamic_pointer_cast<const IComponent>(detector);
          if (comp) {
            bool consistent = (comp->getFullName() == name || comp->getName() == name);
            if (!consistent) {
              g_log.warning() << "Error whilst loading parameters. Name '" << name << "' does not match id '" << detid
                              << "'.\n";
              g_log.warning() << "Parameters have been applied to detector with id '" << detid
                              << "'. Please check the name is correct.\n";
            }
          }
        }
      } else {
        // No detector id given, fall back to using the name

        if (name.find('/', 0) == std::string::npos) { // Simple name, look for
          // all components of that
          // name.
          sharedIComp = instrument->getAllComponentsWithName(name);
        } else { // Pathname given. Assume it is unique.
          std::shared_ptr<const Geometry::IComponent> shared = instrument->getComponentByName(name);
          sharedIComp.emplace_back(shared);
        }
      }

      for (auto &ptr : sharedIComp) {
        std::shared_ptr<const Geometry::Component> sharedComp =
            std::dynamic_pointer_cast<const Geometry::Component>(ptr);
        if (sharedComp) {
          // Not empty Component
          if (sharedComp->isParametrized()) {
            setLogfile(sharedComp->base(), curElem, instrument->getLogfileCache(), requestedDate);
          } else {
            setLogfile(ptr.get(), curElem, instrument->getLogfileCache(), requestedDate);
          }
        }
      }
    }
    curNode = curNode->nextSibling();
  }
}

/**
Apply the cache.
@param cacheToApply : Cache file object to use the geometries.
*/
void InstrumentDefinitionParser::applyCache(const IDFObject_const_sptr &cacheToApply) {
  const std::string cacheFullPath = cacheToApply->getFileFullPathStr();
  g_log.information("Loading geometry cache from " + cacheFullPath);
  // create a vtk reader
  std::map<std::string, std::shared_ptr<Geometry::IObject>>::iterator objItr;
  std::shared_ptr<Mantid::Geometry::vtkGeometryCacheReader> reader(
      new Mantid::Geometry::vtkGeometryCacheReader(cacheFullPath));
  for (objItr = mapTypeNameToShape.begin(); objItr != mapTypeNameToShape.end(); ++objItr) {
    // caching only applies to CSGObject
    if (auto csgObj = std::dynamic_pointer_cast<CSGObject>(((*objItr).second))) {
      csgObj->setVtkGeometryCacheReader(reader);
    }
  }
}

/**
Write the cache file from the IDF file and apply it.
@param firstChoiceCache : File location for a first choice cache.
@param fallBackCache : File location for a fallback cache if required.
*/
InstrumentDefinitionParser::CachingOption
InstrumentDefinitionParser::writeAndApplyCache(IDFObject_const_sptr firstChoiceCache,
                                               IDFObject_const_sptr fallBackCache) {
  IDFObject_const_sptr usedCache = std::move(firstChoiceCache);
  auto cachingOption = WroteGeomCache;

  g_log.notice("Geometry cache is not available");
  try {
    Poco::File dir = usedCache->getParentDirectory();
    if (dir.path().empty() || !dir.exists() || !dir.canWrite()) {
      usedCache = std::move(fallBackCache);
      cachingOption = WroteCacheTemp;
      g_log.information() << "Geometrycache directory is read only, writing cache "
                             "to system temp.\n";
    }
  } catch (Poco::FileNotFoundException &) {
    g_log.error() << "Unable to find instrument definition while attempting to "
                     "write cache.\n";
    throw std::runtime_error("Unable to find instrument definition while "
                             "attempting to write cache.\n");
  }
  const std::string cacheFullPath = usedCache->getFileFullPathStr();
  g_log.notice() << "Creating cache in " << cacheFullPath << "\n";
  // create a vtk writer
  std::map<std::string, std::shared_ptr<Geometry::IObject>>::iterator objItr;
  std::shared_ptr<Mantid::Geometry::vtkGeometryCacheWriter> writer(
      new Mantid::Geometry::vtkGeometryCacheWriter(cacheFullPath));
  for (objItr = mapTypeNameToShape.begin(); objItr != mapTypeNameToShape.end(); ++objItr) {
    // caching only applies to CSGObject
    if (auto csgObj = std::dynamic_pointer_cast<CSGObject>(((*objItr).second))) {
      csgObj->setVtkGeometryCacheWriter(writer);
    }
  }
  writer->write();
  return cachingOption;
}

/** Reads in or creates the geometry cache ('vtp') file
@return CachingOption selected.
*/
InstrumentDefinitionParser::CachingOption InstrumentDefinitionParser::setupGeometryCache() {
  // Get cached file name
  // If the instrument directory is writable, put them there else use
  // temporary
  // directory.
  IDFObject_const_sptr fallBackCache = std::make_shared<const IDFObject>(
      Poco::Path(ConfigService::Instance().getTempDir()).append(this->getMangledName() + ".vtp").toString());
  CachingOption cachingOption = NoneApplied;
  if (m_cacheFile->exists()) {
    applyCache(m_cacheFile);
    cachingOption = ReadGeomCache;
  } else if (fallBackCache->exists()) {
    applyCache(fallBackCache);
    cachingOption = ReadFallBack;
  } else {
    cachingOption = writeAndApplyCache(m_cacheFile, fallBackCache);
  }
  return cachingOption;
}

/**
Getter for the applied caching option.
@return selected caching.
*/
InstrumentDefinitionParser::CachingOption InstrumentDefinitionParser::getAppliedCachingOption() const {
  return m_cachingOption;
}

void InstrumentDefinitionParser::createNeutronicInstrument() {
  // Create a copy of the instrument
  auto physical = std::make_unique<Instrument>(*m_instrument);
  // Store the physical instrument 'inside' the neutronic instrument
  m_instrument->setPhysicalInstrument(std::move(physical));

  // Now we manipulate the original instrument (m_instrument) to hold
  // neutronic positions
  for (const auto &component : m_neutronicPos) {
    if (component.second) {
      setLocation(component.first, component.second, m_angleConvertConst, m_deltaOffsets);
      // TODO: Do we need to deal with 'facing'???

      // Check for a 'type' attribute, indicating that we want to set the
      // neutronic shape
      if (component.second->hasAttribute("type") && dynamic_cast<ObjComponent *>(component.first)) {
        const Poco::XML::XMLString shapeName = component.second->getAttribute("type");
        auto shapeIt = mapTypeNameToShape.find(shapeName);
        if (shapeIt != mapTypeNameToShape.end()) {
          // Change the shape on the current component to the one requested
          auto objCmpt = dynamic_cast<ObjComponent *>(component.first);
          if (objCmpt)
            objCmpt->setShape(shapeIt->second);
        } else {
          throw Exception::InstrumentDefinitionError("Requested type " + shapeName + " not defined in IDF");
        }
      }
    } else // We have a null Element*, which signals a detector with no
           // neutronic position
    {
      // This should only happen for detectors
      auto *det = dynamic_cast<Detector *>(component.first);
      if (det)
        m_instrument->removeDetector(det);
    }
  }
}

/**
 * Takes as input a \<type\> element containing a
 * <combine-components-into-one-shape>, and
 * adjust the \<type\> element by replacing its containing \<component\>
 * elements with \<cuboid\>'s
 * (note for now this will only work for \<cuboid\>'s and when necessary this
 * can be extended).
 *
 *  @param pElem ::  Poco::XML \<type\> element that we want to adjust
 *  @param isTypeAssembly [in] :: tell whether any other type, but the special
 * one treated here, is assembly or not
 *  @param getTypeElement [in] :: contain pointers to all types but the onces
 * treated here
 *
 *  @throw InstrumentDefinitionError Thrown if issues with the content of XML
 * instrument file
 */
void InstrumentDefinitionParser::adjust(Poco::XML::Element *pElem, const std::map<std::string, bool> &isTypeAssembly,
                                        std::map<std::string, Poco::XML::Element *> &getTypeElement) {
  UNUSED_ARG(isTypeAssembly)
  // check if pElem is an element with tag name 'type'
  if (pElem->tagName() != "type")
    throw Exception::InstrumentDefinitionError("Argument to function adjust() "
                                               "must be a pointer to an XML "
                                               "element with tag name type.");

  // check that there is a <combine-components-into-one-shape> element in type
  Poco::AutoPtr<NodeList> pNLccioh = pElem->getElementsByTagName("combine-components-into-one-shape");
  if (pNLccioh->length() == 0) {
    throw Exception::InstrumentDefinitionError(std::string("Argument to function adjust() must be a pointer to an XML "
                                                           "element with tag name type,") +
                                               " which contain a <combine-components-into-one-shape> element.");
  }

  // check that there is a <algebra> element in type
  Poco::AutoPtr<NodeList> pNLalg = pElem->getElementsByTagName("algebra");
  if (pNLalg->length() == 0) {
    throw Exception::InstrumentDefinitionError(std::string("An <algebra> element must be part of a <type>, which") +
                                               " includes a <combine-components-into-one-shape> element. See "
                                               "docs.mantidproject.org/concepts/InstrumentDefinitionFile.");
  }

  // check that there is a <location> element in type
  Poco::AutoPtr<NodeList> pNL = pElem->getElementsByTagName("location");
  unsigned long numLocation = pNL->length();
  if (numLocation == 0) {
    throw Exception::InstrumentDefinitionError(std::string("At least one <location> element must be part of a "
                                                           "<type>, which") +
                                               " includes a <combine-components-into-one-shape> element. See "
                                               "docs.mantidproject.org/concepts/InstrumentDefinitionFile.");
  }

  // check if a <translate-rotate-combined-shape-to> is defined
  Poco::AutoPtr<NodeList> pNL_TransRot = pElem->getElementsByTagName("translate-rotate-combined-shape-to");
  const Element *pTransRot = nullptr;
  if (pNL_TransRot->length() == 1) {
    pTransRot = static_cast<Element *>(pNL_TransRot->item(0));
  }

  // to convert all <component>'s in type into <cuboid> elements, which are
  // added
  // to pElem, and these <component>'s are deleted after loop

  std::unordered_set<Element *> allComponentInType; // used to hold <component>'s found
  std::vector<std::string> allLocationName;         // used to check if loc names unique
  for (unsigned long i = 0; i < numLocation; i++) {
    auto *pLoc = static_cast<Element *>(pNL->item(i));

    // The location element is required to be a child of a component element.
    // Get this component element
    Element *pCompElem = InstrumentDefinitionParser::getParentComponent(pLoc);

    // get the name given to the <location> element in focus
    // note these names are required to be unique for the purpose of
    // constructing the <algebra>
    std::string locationElementName = pLoc->getAttribute("name");
    if (std::find(allLocationName.begin(), allLocationName.end(), locationElementName) == allLocationName.end())
      allLocationName.emplace_back(locationElementName);
    else
      throw Exception::InstrumentDefinitionError(
          std::string("Names in a <type> element containing ") +
          "a <combine-components-into-one-shape> element must be unique. " + "Here error is that " +
          locationElementName +
          " appears at least twice. See docs.mantidproject.org/concepts/InstrumentDefinitionFile.");

    // create dummy component to hold coord. sys. of cuboid
    auto baseCoor = std::make_unique<CompAssembly>("base"); // dummy assembly used to get to end assembly if nested
    ICompAssembly *endComponent = nullptr;                  // end assembly, its purpose is to
                                                            // hold the shape coordinate system
    // get shape coordinate system, returned as endComponent, as defined by
    // pLoc
    // and nested <location> elements
    // of pLoc
    std::string shapeTypeName = getShapeCoorSysComp(baseCoor.get(), pLoc, getTypeElement, endComponent);

    // translate and rotate cuboid according to shape coordinate system in
    // endComponent
    std::string cuboidStr = translateRotateXMLcuboid(endComponent, getTypeElement[shapeTypeName], locationElementName);

    // if <translate-rotate-combined-shape-to> is specified
    if (pTransRot) {
      baseCoor = std::make_unique<CompAssembly>("base");

      setLocation(baseCoor.get(), pTransRot, m_angleConvertConst);

      // Translate and rotate shape xml string according to
      // <translate-rotate-combined-shape-to>
      cuboidStr = translateRotateXMLcuboid(baseCoor.get(), cuboidStr, locationElementName);
    }

    DOMParser pParser;
    Poco::AutoPtr<Document> pDoc;
    try {
      pDoc = pParser.parseString(cuboidStr);
    } catch (...) {
      throw Exception::InstrumentDefinitionError(std::string("Unable to parse XML string ") + cuboidStr);
    }
    // Get pointer to root element and add this element to pElem
    Element *pCuboid = pDoc->documentElement();
    Poco::AutoPtr<Node> fisse = (pElem->ownerDocument())->importNode(pCuboid, true);
    pElem->appendChild(fisse);

    allComponentInType.insert(pCompElem);
  }

  // delete all <component> found in pElem
  for (const auto &component : allComponentInType)
    pElem->removeChild(component);
}

/// Returns a translated and rotated \<cuboid\> element with "id" attribute
/// equal cuboidName
/// @param comp coordinate system to translate and rotate cuboid to
/// @param cuboidEle Input \<cuboid\> element
/// @param cuboidName What the "id" attribute of the returned \<coboid\> will
/// be
/// set to
/// @return XML string of translated and rotated \<cuboid\>
std::string InstrumentDefinitionParser::translateRotateXMLcuboid(ICompAssembly *comp,
                                                                 const Poco::XML::Element *cuboidEle,
                                                                 const std::string &cuboidName) {
  Element *pElem_lfb = getShapeElement(cuboidEle, "left-front-bottom-point");
  Element *pElem_lft = getShapeElement(cuboidEle, "left-front-top-point");
  Element *pElem_lbb = getShapeElement(cuboidEle, "left-back-bottom-point");
  Element *pElem_rfb = getShapeElement(cuboidEle, "right-front-bottom-point");

  V3D lfb = parsePosition(pElem_lfb); // left front bottom
  V3D lft = parsePosition(pElem_lft); // left front top
  V3D lbb = parsePosition(pElem_lbb); // left back bottom
  V3D rfb = parsePosition(pElem_rfb); // right front bottom

  // translate and rotate cuboid according to coord. sys. of comp
  V3D p_lfb = getAbsolutPositionInCompCoorSys(comp, lfb);
  V3D p_lft = getAbsolutPositionInCompCoorSys(comp, lft);
  V3D p_lbb = getAbsolutPositionInCompCoorSys(comp, lbb);
  V3D p_rfb = getAbsolutPositionInCompCoorSys(comp, rfb);

  // create output cuboid XML string
  std::ostringstream obj_str;

  obj_str << "<cuboid id=\"" << cuboidName << "\">";
  obj_str << "<left-front-bottom-point ";
  obj_str << "x=\"" << p_lfb.X();
  obj_str << "\" y=\"" << p_lfb.Y();
  obj_str << "\" z=\"" << p_lfb.Z();
  obj_str << "\"  />";
  obj_str << "<left-front-top-point ";
  obj_str << "x=\"" << p_lft.X();
  obj_str << "\" y=\"" << p_lft.Y();
  obj_str << "\" z=\"" << p_lft.Z();
  obj_str << "\"  />";
  obj_str << "<left-back-bottom-point ";
  obj_str << "x=\"" << p_lbb.X();
  obj_str << "\" y=\"" << p_lbb.Y();
  obj_str << "\" z=\"" << p_lbb.Z();
  obj_str << "\"  />";
  obj_str << "<right-front-bottom-point ";
  obj_str << "x=\"" << p_rfb.X();
  obj_str << "\" y=\"" << p_rfb.Y();
  obj_str << "\" z=\"" << p_rfb.Z();
  obj_str << "\"  />";
  obj_str << "</cuboid>";

  return obj_str.str();
}

/// return absolute position of point which is set relative to the
/// coordinate system of the input component
/// @param comp Reference coordinate system
/// @param pos A position relative to the coord. sys. of comp
/// @return absolute position
V3D InstrumentDefinitionParser::getAbsolutPositionInCompCoorSys(ICompAssembly *comp, V3D pos) {
  Component *dummyComp = new Component("dummy", comp);
  comp->add(dummyComp);

  dummyComp->setPos(pos); // set pos relative to comp coord. sys.

  V3D retVal = dummyComp->getPos(); // get absolute position

  return retVal;
}

/// Returns a translated and rotated \<cuboid\> element with "id" attribute
/// equal cuboidName
/// @param comp coordinate system to translate and rotate cuboid to
/// @param cuboidXML Input \<cuboid\> xml string
/// @param cuboidName What the "id" attribute of the returned \<coboid\> will
/// be
/// set to
/// @return XML string of translated and rotated \<cuboid\>
std::string InstrumentDefinitionParser::translateRotateXMLcuboid(ICompAssembly *comp, const std::string &cuboidXML,
                                                                 const std::string &cuboidName) {
  DOMParser pParser;
  Poco::AutoPtr<Document> pDoc;
  try {
    pDoc = pParser.parseString(cuboidXML);
  } catch (...) {
    throw Exception::InstrumentDefinitionError(std::string("Unable to parse XML string ") + cuboidXML);
  }

  Element *pCuboid = pDoc->documentElement();

  std::string retVal = translateRotateXMLcuboid(comp, pCuboid, cuboidName);

  return retVal;
}

/// Take as input a \<locations\> element. Such an element is a short-hand
/// notation for a sequence of \<location\> elements.
/// This method return this sequence as a xml string
/// @param pElem Input \<locations\> element
/// @return XML document containing \<location\> elements
/// @throw InstrumentDefinitionError Thrown if issues with the content of XML
/// instrument file
Poco::AutoPtr<Poco::XML::Document>
InstrumentDefinitionParser::convertLocationsElement(const Poco::XML::Element *pElem) {
  // Number of <location> this <locations> element is shorthand for
  size_t nElements(0);
  if (pElem->hasAttribute("n-elements")) {
    auto n = boost::lexical_cast<int>(Strings::strip(pElem->getAttribute("n-elements")));

    if (n <= 0) {
      throw Exception::InstrumentDefinitionError("n-elements must be positive");
    } else {
      nElements = static_cast<size_t>(n);
    }
  } else {
    throw Exception::InstrumentDefinitionError("When using <locations> n-elements attribute is required. See "
                                               "docs.mantidproject.org/concepts/InstrumentDefinitionFile.");
  }

  std::string name;
  if (pElem->hasAttribute("name")) {
    name = pElem->getAttribute("name");
  }

  int nameCountStart(0);
  if (pElem->hasAttribute("name-count-start")) {
    nameCountStart = boost::lexical_cast<int>(Strings::strip(pElem->getAttribute("name-count-start")));
  }

  int nameCountIncrement(1);
  if (pElem->hasAttribute("name-count-increment")) {
    nameCountIncrement = boost::lexical_cast<int>(Strings::strip(pElem->getAttribute("name-count-increment")));

    if (nameCountIncrement <= 0)
      throw Exception::InstrumentDefinitionError("name-count-increment must be greater than zero.");
  }

  // A list of numeric attributes which are allowed to have corresponding -end
  std::set<std::string> rangeAttrs = {"x", "y", "z", "r", "t", "p", "rot"};

  // Numeric attributes related to rotation. Doesn't make sense to have -end
  // for
  // those
  std::set<std::string> rotAttrs = {"axis-x", "axis-y", "axis-z"};

  // A set of all numeric attributes for convenience
  std::set<std::string> allAttrs;
  allAttrs.insert(rangeAttrs.begin(), rangeAttrs.end());
  allAttrs.insert(rotAttrs.begin(), rotAttrs.end());

  // Attribute values as read from <locations>. If the attribute doesn't have
  // a
  // value here, it
  // means that it wasn't set
  std::map<std::string, double> attrValues;

  // Read all the set attribute values
  for (const auto &attr : allAttrs) {
    if (pElem->hasAttribute(attr)) {
      attrValues[attr] = boost::lexical_cast<double>(Strings::strip(pElem->getAttribute(attr)));
    }
  }

  // Range attribute steps
  std::map<std::string, double> rangeAttrSteps;

  // Find *-end for range attributes and calculate steps
  for (const auto &rangeAttr : rangeAttrs) {
    std::string endAttr = rangeAttr + "-end";
    if (pElem->hasAttribute(endAttr)) {
      if (attrValues.find(rangeAttr) == attrValues.end()) {
        throw Exception::InstrumentDefinitionError("*-end attribute without corresponding * attribute.");
      }

      double from = attrValues[rangeAttr];
      auto to = boost::lexical_cast<double>(Strings::strip(pElem->getAttribute(endAttr)));

      rangeAttrSteps[rangeAttr] = (to - from) / (static_cast<double>(nElements) - 1);
    }
  }

  Poco::AutoPtr<Document> pDoc = new Document;
  Poco::AutoPtr<Element> pRoot = pDoc->createElement("expansion-of-locations-element");
  pDoc->appendChild(pRoot);

  for (size_t i = 0; i < nElements; ++i) {
    Poco::AutoPtr<Element> pLoc = pDoc->createElement("location");

    if (!name.empty()) {
      // Add name with appropriate numeric postfix
      pLoc->setAttribute("name", name + std::to_string(nameCountStart + (i * nameCountIncrement)));
    }

    // Copy values of all the attributes set
    for (auto &attrValue : attrValues) {
      pLoc->setAttribute(attrValue.first, boost::lexical_cast<std::string>(attrValue.second));

      // If attribute has a step, increase the value by the step
      if (rangeAttrSteps.find(attrValue.first) != rangeAttrSteps.end()) {
        attrValue.second += rangeAttrSteps[attrValue.first];
      }
    }

    pRoot->appendChild(pLoc);
  }

  return pDoc;
}

/** Generates a vtp filename from a xml filename
 *
 *  @return The vtp filename
 *
 */
const std::string InstrumentDefinitionParser::createVTPFileName() {
  std::string retVal;
  std::string filename = getMangledName();
  if (!filename.empty()) {
    Poco::Path path(ConfigService::Instance().getVTPFileDirectory());
    path.makeDirectory();
    path.append(filename + ".vtp");
    retVal = path.toString();
  }
  return retVal;
}

/** Return a subelement of an XML element, but also checks that there exist
 *exactly one entry
 *  of this subelement.
 *
 *  @param pElem :: XML from instrument def. file
 *  @param name :: Name of subelement
 *  @return The subelement
 *
 *  @throw std::invalid_argument Thrown if issues with XML string
 */
Poco::XML::Element *InstrumentDefinitionParser::getShapeElement(const Poco::XML::Element *pElem,
                                                                const std::string &name) {
  // check if this shape element contain an element with name specified by the
  // 2nd function argument
  Poco::AutoPtr<NodeList> pNL = pElem->getElementsByTagName(name);
  if (pNL->length() != 1) {
    throw std::invalid_argument("XML element: <" + pElem->tagName() +
                                "> must contain exactly one sub-element with name: <" + name + ">.");
  }
  auto *retVal = static_cast<Element *>(pNL->item(0));
  return retVal;
}

/** Get position coordinates from XML element
 *
 *  @param pElem :: XML element whose attributes contain position coordinates
 *  @return Position coordinates in the form of a V3D object
 */
V3D InstrumentDefinitionParser::parsePosition(Poco::XML::Element *pElem) {
  V3D retVal;

  if (pElem->hasAttribute("R") || pElem->hasAttribute("theta") || pElem->hasAttribute("phi")) {
    double R = attrToDouble(pElem, "R");
    double theta = attrToDouble(pElem, "theta");
    double phi = attrToDouble(pElem, "phi");

    retVal.spherical(R, theta, phi);
  } else if (pElem->hasAttribute("r") || pElem->hasAttribute("t") || pElem->hasAttribute("p"))
  // This is alternative way a user may specify spherical coordinates
  // which may be preferred in the long run to the more verbose of
  // using R, theta and phi.
  {
    double R = attrToDouble(pElem, "r");
    double theta = attrToDouble(pElem, "t");
    double phi = attrToDouble(pElem, "p");

    retVal.spherical(R, theta, phi);
  } else {

    double x = attrToDouble(pElem, "x");
    double y = attrToDouble(pElem, "y");
    double z = attrToDouble(pElem, "z");

    retVal(x, y, z);
  }

  return retVal;
}

//-----------------------------------------------------------------------------------------------------------------------
/** Adds component with coordinate system as defined by the input \<location\>
element to
    the input parent component. Nested \<location\> elements are allowed and
this method
    is recursive. As this method is running recursively it will eventually
return a leaf
    component (in endAssembly) and the name of the \<type\> of this leaf
*
*  @param parent :: CompAssembly Parent component
*  @param pLocElem ::  Poco::XML element that points to a location element
*  @param getTypeElement :: contain pointers to all \<type\>s
*  @param endAssembly :: Output child component, which coor. sys. modified
according to pLocElem
*  @return Returns \<type\> name
*  @throw InstrumentDefinitionError Thrown if issues with the content of XML
instrument file
*/
std::string InstrumentDefinitionParser::getShapeCoorSysComp(Geometry::ICompAssembly *parent,
                                                            const Poco::XML::Element *pLocElem,
                                                            std::map<std::string, Poco::XML::Element *> &getTypeElement,
                                                            Geometry::ICompAssembly *&endAssembly) {
  // The location element is required to be a child of a component element.
  // Get
  // this component element
  Element *pCompElem = InstrumentDefinitionParser::getParentComponent(pLocElem);

  // Create the assembly that will be appended into the parent.
  Geometry::ICompAssembly *ass;

  // The newly added component is required to have a type. Find out what this
  // type is and find all the location elements of this type. Finally loop
  // over
  // these
  // location elements

  Element *pType = getTypeElement[pCompElem->getAttribute("type")];

  ass = new Geometry::CompAssembly(InstrumentDefinitionParser::getNameOfLocationElement(pLocElem, pCompElem), parent);
  endAssembly = ass;

  // set location for this newly added comp
  setLocation(ass, pLocElem, m_angleConvertConst);

  Poco::AutoPtr<NodeList> pNL = pType->getElementsByTagName("location");
  if (pNL->length() == 0) {
    return pType->getAttribute("name");
  } else if (pNL->length() == 1) {
    auto const *pElem = static_cast<Element *>(pNL->item(0));
    return getShapeCoorSysComp(ass, pElem, getTypeElement, endAssembly);
  } else {
    throw Exception::InstrumentDefinitionError(
        std::string("When using <combine-components-into-one-shape> ") +
        " the containing component elements are not allowed to contain "
        "multiple nested components. See docs.mantidproject.org/concepts/InstrumentDefinitionFile.");
  }
}

} // namespace Mantid::Geometry
