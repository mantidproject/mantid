#include <fstream>
#include <sstream>

#include "MantidGeometry/Instrument/InstrumentDefinitionParser.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/ObjCompAssembly.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Instrument/XMLInstrumentParameter.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidGeometry/Rendering/vtkGeometryCacheReader.h"
#include "MantidGeometry/Rendering/vtkGeometryCacheWriter.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/ProgressBase.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/Strings.h"

#include <Poco/DOM/Document.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/DOMWriter.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeFilter.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/SAX/AttributesImpl.h>

#include <boost/make_shared.hpp>
#include <boost/assign/list_of.hpp>

using namespace Mantid;
using namespace Mantid::Kernel;
using Poco::XML::DOMParser;
using Poco::XML::Document;
using Poco::XML::Element;
using Poco::XML::Node;
using Poco::XML::NodeList;
using Poco::XML::NodeIterator;
using Poco::XML::NodeFilter;

namespace Mantid {
namespace Geometry {
namespace {
// initialize the static logger
Kernel::Logger g_log("InstrumentDefinitionParser");
}

//----------------------------------------------------------------------------------------------
/** Constructor
 */
InstrumentDefinitionParser::InstrumentDefinitionParser()
    : m_xmlFile(boost::make_shared<NullIDFObject>()),
      m_cacheFile(boost::make_shared<NullIDFObject>()), pDoc(NULL),
      pRootElem(NULL), m_hasParameterElement_beenSet(false),
      m_haveDefaultFacing(false), m_deltaOffsets(false),
      m_angleConvertConst(1.0), m_indirectPositions(false),
      m_cachingOption(NoneApplied) {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
InstrumentDefinitionParser::~InstrumentDefinitionParser() {}

//----------------------------------------------------------------------------------------------
/** Initialize the XML parser based on an IDF xml file path.
 *
 *  Note that this convenience initialize method actually translates the inputs
 *into the other initialize method.
 *
 * @param filename :: IDF .xml path (full). This is needed mostly to find the
 *instrument geometry cache.
 * @param instName :: name of the instrument
 * @param xmlText :: XML contents of IDF
 */
void InstrumentDefinitionParser::initialize(const std::string &filename,
                                            const std::string &instName,
                                            const std::string &xmlText) {
  IDFObject_const_sptr xmlFile = boost::make_shared<const IDFObject>(filename);
  // Use the filename to construct the cachefile name so that there is a 1:1 map
  // between a definition file & cache
  std::string idfExt = xmlFile->getExtension();
  std::string vtpFilename = filename;
  static const char *vtpExt = ".vtp";
  if (idfExt.empty()) {
    vtpFilename += vtpExt;
  } else {
    boost::replace_last(vtpFilename, idfExt, vtpExt);
  }
  IDFObject_const_sptr vtpFile =
      boost::make_shared<const IDFObject>(vtpFilename);

  this->initialize(xmlFile, vtpFile, instName, xmlText);
}

//----------------------------------------------------------------------------------------------
/** Initialize the XML parser based on an IDF xml and cached vtp file objects.
 *
 * @param xmlFile :: The xml file, here wrapped in a IDFObject
 * @param expectedCacheFile :: Expected vtp cache file
 * @param instName :: Instrument name
 * @param xmlText :: XML contents of IDF
 */
void InstrumentDefinitionParser::initialize(
    const IDFObject_const_sptr xmlFile,
    const IDFObject_const_sptr expectedCacheFile, const std::string &instName,
    const std::string &xmlText) {

  // Handle the parameters
  const std::string filename = xmlFile->getFileFullPathStr();
  m_instName = instName;
  m_xmlFile = xmlFile;
  m_cacheFile = expectedCacheFile;

  // Set up the DOM parser and parse xml file
  DOMParser pParser;
  try {
    pDoc = pParser.parseString(xmlText);
  } catch (Poco::Exception &exc) {
    throw Kernel::Exception::FileError(
        exc.displayText() + ". Unable to parse XML", filename);
  } catch (...) {
    throw Kernel::Exception::FileError("Unable to parse XML", filename);
  }
  // Get pointer to root element
  pRootElem = pDoc->documentElement();
  if (!pRootElem->hasChildNodes()) {
    g_log.error("XML file: " + filename + "contains no root element.");
    throw Kernel::Exception::InstrumentDefinitionError(
        "No root element in XML instrument file", filename);
  }

  // Create our new instrument
  // We don't want the instrument name taken out of the XML file itself, it
  // should come from the filename (or the property)
  m_instrument = boost::make_shared<Instrument>(m_instName);

  // Save the XML file path and contents
  m_instrument->setFilename(filename);
  m_instrument->setXmlText(xmlText);
}

//----------------------------------------------------------------------------------------------
/**
 * Handle used in the singleton constructor for instrument file should append
 *the value
 * of the last-modified tag inside the file to determine if it is already in
 *memory so that
 * changes to the instrument file will cause file to be reloaded.
 *
 * @return a mangled name combining the filename and the "last-modified"
 *attribute of the XML contents
 * */
std::string InstrumentDefinitionParser::getMangledName() {
  // Use the file in preference if possible.
  if (this->m_xmlFile->exists()) {
    return m_xmlFile->getMangledName();
  } else if (!pDoc.isNull()) {
    std::string lastModified = pRootElem->getAttribute("last-modified");
    if (lastModified.empty()) {
      g_log.warning() << "The IDF that you are using doesn't contain a "
                         "'last-modified' field. ";
      g_log.warning() << "You may not get the correct definition file loaded."
                      << std::endl;
    }
    return m_instName + lastModified;
  } else {
    throw std::runtime_error(
        "Call InstrumentDefinitionParser::initialize() before getMangledName.");
  }
}

//----------------------------------------------------------------------------------------------
/** Fully parse the IDF XML contents and returns the instrument thus created
 *
 * @param prog :: Optional Progress reporter object. If NULL, no progress
 *reporting.
 * @return the instrument that was created
 */
Instrument_sptr
InstrumentDefinitionParser::parseXML(Kernel::ProgressBase *prog) {
  if (!pDoc)
    throw std::runtime_error(
        "Call InstrumentDefinitionParser::initialize() before parseXML.");

  setValidityRange(pRootElem);
  readDefaults(pRootElem->getChildElement("defaults"));
  // create maps: isTypeAssembly and mapTypeNameToShape
  Geometry::ShapeFactory shapeCreator;

  const std::string filename = m_xmlFile->getFileFullPathStr();

  Poco::AutoPtr<NodeList> pNL_type = pRootElem->getElementsByTagName("type");
  if (pNL_type->length() == 0) {
    g_log.error("XML file: " + filename + "contains no type elements.");
    throw Kernel::Exception::InstrumentDefinitionError(
        "No type elements in XML instrument file", filename);
  }

  // Collect some information about types for later use including:
  //  * populate directory getTypeElement
  //  * populate directory isTypeAssemply
  //  * create shapes for all none assemply components and store in
  //  mapTyepNameToShape
  //  * If 'Outline' attribute set for assemply add attribute object_created=no
  //  to tell
  //    create shape for such assemply also later
  unsigned long numberTypes = pNL_type->length();
  for (unsigned long iType = 0; iType < numberTypes; iType++) {
    Element *pTypeElem = static_cast<Element *>(pNL_type->item(iType));
    std::string typeName = pTypeElem->getAttribute("name");

    // check if contain <combine-components-into-one-shape>. If this then such
    // types are adjusted after this loop has completed
    Poco::AutoPtr<NodeList> pNL_type_combine_into_one_shape =
        pTypeElem->getElementsByTagName("combine-components-into-one-shape");
    if (pNL_type_combine_into_one_shape->length() > 0) {
      continue;
    }

    // Each type in the IDF must be uniquely named, hence return error if type
    // has already been defined
    if (getTypeElement.find(typeName) != getTypeElement.end()) {
      g_log.error("XML file: " + filename +
                  "contains more than one type element named " + typeName);
      throw Kernel::Exception::InstrumentDefinitionError(
          "XML instrument file contains more than one type element named " +
              typeName,
          filename);
    }
    getTypeElement[typeName] = pTypeElem;

    // identify for now a type to be an assemble by it containing elements
    // with tag name 'component'
    Poco::AutoPtr<NodeList> pNL_local =
        pTypeElem->getElementsByTagName("component");
    if (pNL_local->length() == 0) {
      isTypeAssembly[typeName] = false;

      // for now try to create a geometry shape associated with every type
      // that does not contain any component elements
      mapTypeNameToShape[typeName] = shapeCreator.createShape(pTypeElem);
      mapTypeNameToShape[typeName]->setName(static_cast<int>(iType));
    } else {
      isTypeAssembly[typeName] = true;
      if (pTypeElem->hasAttribute("outline")) {
        pTypeElem->setAttribute("object_created", "no");
      }
    }
  }

  // Deal with adjusting types containing <combine-components-into-one-shape>
  for (unsigned long iType = 0; iType < numberTypes; iType++) {
    Element *pTypeElem = static_cast<Element *>(pNL_type->item(iType));
    std::string typeName = pTypeElem->getAttribute("name");

    // In this loop only interested in types containing
    // <combine-components-into-one-shape>
    Poco::AutoPtr<NodeList> pNL_type_combine_into_one_shape =
        pTypeElem->getElementsByTagName("combine-components-into-one-shape");
    const unsigned long nelements = pNL_type_combine_into_one_shape->length();
    if (nelements == 0)
      continue;

    // Each type in the IDF must be uniquely named, hence return error if type
    // has already been defined
    if (getTypeElement.find(typeName) != getTypeElement.end()) {
      g_log.error("XML file: " + filename +
                  "contains more than one type element named " + typeName);
      throw Kernel::Exception::InstrumentDefinitionError(
          "XML instrument file contains more than one type element named " +
              typeName,
          filename);
    }
    getTypeElement[typeName] = pTypeElem;

    InstrumentDefinitionParser helper;
    helper.adjust(pTypeElem, isTypeAssembly, getTypeElement);

    isTypeAssembly[typeName] = false;

    mapTypeNameToShape[typeName] = shapeCreator.createShape(pTypeElem);
    mapTypeNameToShape[typeName]->setName(static_cast<int>(iType));
  }

  // create m_hasParameterElement
  Poco::AutoPtr<NodeList> pNL_parameter =
      pRootElem->getElementsByTagName("parameter");

  unsigned long numParameter = pNL_parameter->length();
  m_hasParameterElement.reserve(numParameter);

  // It turns out that looping over all nodes and checking if their nodeName is
  // equal
  // to "parameter" is much quicker than looping over the pNL_parameter
  // NodeList.
  Poco::XML::NodeIterator it(pRootElem, Poco::XML::NodeFilter::SHOW_ELEMENT);
  Poco::XML::Node *pNode = it.nextNode();
  while (pNode) {
    if (pNode->nodeName() == "parameter") {
      Element *pParameterElem = static_cast<Element *>(pNode);
      m_hasParameterElement.push_back(
          static_cast<Element *>(pParameterElem->parentNode()));
    }
    pNode = it.nextNode();
  }

  m_hasParameterElement_beenSet = true;

  // See if any parameters set at instrument level
  setLogfile(m_instrument.get(), pRootElem, m_instrument->getLogfileCache());

  //
  // do analysis for each top level compoment element
  //
  Poco::AutoPtr<NodeList> pNL_comp =
      pRootElem->childNodes(); // here get all child nodes
  unsigned long pNL_comp_length = pNL_comp->length();

  if (prog)
    prog->resetNumSteps(pNL_comp_length, 0.0, 1.0);
  for (unsigned long i = 0; i < pNL_comp_length; i++) {
    if (prog)
      prog->report("Loading instrument Definition");

    // we are only interest in the top level component elements hence
    // the reason for the if statement below
    if ((pNL_comp->item(i))->nodeType() == Node::ELEMENT_NODE &&
        ((pNL_comp->item(i))->nodeName()).compare("component") == 0) {
      const Element *pElem = static_cast<Element *>(pNL_comp->item(i));

      IdList idList; // structure to possibly be populated with detector IDs

      // Get all <location> and <locations> elements contained in component
      // element
      // just for the purpose of a IDF syntax check
      Poco::AutoPtr<NodeList> pNL_location =
          pElem->getElementsByTagName("location");
      Poco::AutoPtr<NodeList> pNL_locations =
          pElem->getElementsByTagName("locations");
      // do a IDF syntax check
      if (pNL_location->length() == 0 && pNL_locations->length() == 0) {
        g_log.error(std::string("A component element must contain at least one "
                                "<location> or <locations> element") +
                    " even if it is just an empty location element of the form "
                    "<location />");
        throw Kernel::Exception::InstrumentDefinitionError(
            std::string("A component element must contain at least one "
                        "<location> or <locations> element") +
                " even if it is just an empty location element of the form "
                "<location />",
            filename);
      }

      // Loop through all <location> and <locations> elements of this component
      // by looping
      // all the child nodes and then see if any of these nodes are either
      // <location> or
      // <locations> elements. Done this way order these locations are processed
      // is the
      // order they are listed in the IDF. The latter needed to get detector IDs
      // assigned
      // as expected
      Poco::AutoPtr<NodeList> pNL_childs =
          pElem->childNodes(); // here get all child nodes
      unsigned long pNL_childs_length = pNL_childs->length();
      for (unsigned long iLoc = 0; iLoc < pNL_childs_length; iLoc++) {
        if ((pNL_childs->item(iLoc))->nodeType() == Node::ELEMENT_NODE &&
            (((pNL_childs->item(iLoc))->nodeName()).compare("location") == 0 ||
             ((pNL_childs->item(iLoc))->nodeName()).compare("locations") ==
                 0)) {
          // if a <location> element
          if (((pNL_childs->item(iLoc))->nodeName()).compare("location") == 0) {
            const Element *pLocElem =
                static_cast<Element *>(pNL_childs->item(iLoc));
            // process differently depending on whether component is and
            // assembly or leaf
            if (isAssembly(pElem->getAttribute("type"))) {
              appendAssembly(m_instrument.get(), pLocElem, pElem, idList);
            } else {
              appendLeaf(m_instrument.get(), pLocElem, pElem, idList);
            }
          }

          // if a <locations> element
          if (((pNL_childs->item(iLoc))->nodeName()).compare("locations") ==
              0) {
            const Element *pLocElems =
                static_cast<Element *>(pNL_childs->item(iLoc));

            // append <locations> elements in <locations>
            appendLocations(m_instrument.get(), pLocElems, pElem, idList);
          }
        }
      } // finished looping over all childs of this component

      // A check
      if (idList.counted != static_cast<int>(idList.vec.size())) {
        std::stringstream ss1, ss2;
        ss1 << idList.vec.size();
        ss2 << idList.counted;
        if (!pElem->hasAttribute("idlist")) {
          g_log.error("No detector ID list found for detectors of type " +
                      pElem->getAttribute("type"));
        } else if (idList.vec.size() == 0) {
          g_log.error("No detector IDs found for detectors in list " +
                      pElem->getAttribute("idlist") + "for detectors of type" +
                      pElem->getAttribute("type"));
        } else {
          g_log.error(
              "The number of detector IDs listed in idlist named " +
              pElem->getAttribute("idlist") +
              " is larger than the number of detectors listed in type = " +
              pElem->getAttribute("type"));
        }
        throw Kernel::Exception::InstrumentDefinitionError(
            "Number of IDs listed in idlist (=" + ss1.str() +
                ") is larger than the number of detectors listed in type = " +
                pElem->getAttribute("type") + " (=" + ss2.str() + ").",
            filename);
      }
      idList.reset();
    }
  }

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

  // And give back what we created
  return m_instrument;
}

//-----------------------------------------------------------------------------------------------------------------------
/** Assumes second argument is a XML location element and its parent is a
*component element
*  which is assigned to be an assembly. This method appends the parent component
*element of
*  the location element to the CompAssembly passed as the 1st arg. Note this
*method may call
*  itself, i.e. it may act recursively.
*
*  @param parent :: CompAssembly to append new component to
*  @param pLocElems ::  Poco::XML element that points to a locations element in
*an instrument description XML file, which optionally may be detached (meaning
*it is not required to be part of the DOM tree of the IDF)
*  @param pCompElem :: The Poco::XML \<component\> element that contains the
*\<locations\> element
*  @param idList :: The current IDList
*/
void InstrumentDefinitionParser::appendLocations(
    Geometry::ICompAssembly *parent, const Poco::XML::Element *pLocElems,
    const Poco::XML::Element *pCompElem, IdList &idList) {
  // create detached <location> elements from <locations> element
  const std::string xmlLocation = convertLocationsElement(pLocElems);

  // parse converted <locations> output
  DOMParser pLocationsParser;
  Poco::AutoPtr<Document> pLocationsDoc;
  try {
    pLocationsDoc = pLocationsParser.parseString(xmlLocation);
  } catch (...) {
    throw Kernel::Exception::InstrumentDefinitionError(
        "Unable to parse XML string", xmlLocation);
  }

  // Get pointer to root element
  const Element *pRootLocationsElem = pLocationsDoc->documentElement();
  if (!pRootLocationsElem->hasChildNodes()) {
    throw Kernel::Exception::InstrumentDefinitionError(
        "No root element in XML string", xmlLocation);
  }

  Poco::AutoPtr<NodeList> pNL_locInLocs =
      pRootLocationsElem->getElementsByTagName("location");
  unsigned long pNL_locInLocs_length = pNL_locInLocs->length();
  for (unsigned long iInLocs = 0; iInLocs < pNL_locInLocs_length; iInLocs++) {
    const Element *pLocInLocsElem =
        static_cast<Element *>(pNL_locInLocs->item(iInLocs));
    if (isAssembly(pCompElem->getAttribute("type"))) {
      appendAssembly(parent, pLocInLocsElem, pCompElem, idList);
    } else {
      appendLeaf(parent, pLocInLocsElem, pCompElem, idList);
    }
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
void InstrumentDefinitionParser::saveDOM_Tree(std::string &outFilename) {
  Poco::XML::DOMWriter writer;
  writer.setNewLine("\n");
  writer.setOptions(Poco::XML::XMLWriter::PRETTY_PRINT);

  std::ofstream outFile(outFilename.c_str());
  writer.writeNode(outFile, pDoc);
  outFile.close();
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
void InstrumentDefinitionParser::setLocation(Geometry::IComponent *comp,
                                             const Poco::XML::Element *pElem,
                                             const double angleConvertConst,
                                             const bool deltaOffsets) {
  comp->setPos(
      getRelativeTranslation(comp, pElem, angleConvertConst, deltaOffsets));

  // Rotate coordinate system of this component
  if (pElem->hasAttribute("rot")) {
    double rotAngle =
        angleConvertConst *
        atof((pElem->getAttribute("rot")).c_str()); // assumed to be in degrees

    double axis_x = 0.0;
    double axis_y = 0.0;
    double axis_z = 1.0;

    if (pElem->hasAttribute("axis-x"))
      axis_x = atof((pElem->getAttribute("axis-x")).c_str());
    if (pElem->hasAttribute("axis-y"))
      axis_y = atof((pElem->getAttribute("axis-y")).c_str());
    if (pElem->hasAttribute("axis-z"))
      axis_z = atof((pElem->getAttribute("axis-z")).c_str());

    comp->rotate(Kernel::Quat(rotAngle, Kernel::V3D(axis_x, axis_y, axis_z)));
  }

  // Check if sub-elements <trans> or <rot> of present - for now ignore these if
  // m_deltaOffset = true

  Element *pRecursive = NULL;
  Element *tElem = pElem->getChildElement("trans");
  Element *rElem = pElem->getChildElement("rot");
  bool stillTransElement = true;
  bool firstRound =
      true; // during first round below pRecursive has not been set up front
  while (stillTransElement) {
    // figure out if child element is <trans> or <rot> or none of these

    if (firstRound) {
      firstRound = false;
    } else {
      tElem = pRecursive->getChildElement("trans");
      rElem = pRecursive->getChildElement("rot");
    }

    if (tElem && rElem) {
      // if both a <trans> and <rot> child element present. Ignore <rot> element
      rElem = NULL;
    }

    if (!tElem && !rElem) {
      stillTransElement = false;
    }

    Kernel::V3D posTrans;

    if (tElem) {
      posTrans =
          getRelativeTranslation(comp, tElem, angleConvertConst, deltaOffsets);

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
      double rotAngle =
          angleConvertConst * atof((rElem->getAttribute("val"))
                                       .c_str()); // assumed to be in degrees

      double axis_x = 0.0;
      double axis_y = 0.0;
      double axis_z = 1.0;

      if (rElem->hasAttribute("axis-x"))
        axis_x = atof((rElem->getAttribute("axis-x")).c_str());
      if (rElem->hasAttribute("axis-y"))
        axis_y = atof((rElem->getAttribute("axis-y")).c_str());
      if (rElem->hasAttribute("axis-z"))
        axis_z = atof((rElem->getAttribute("axis-z")).c_str());

      comp->rotate(Kernel::Quat(rotAngle, Kernel::V3D(axis_x, axis_y, axis_z)));

      // for recursive action
      pRecursive = rElem;
    }

  } // end while
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
Kernel::V3D InstrumentDefinitionParser::getRelativeTranslation(
    const Geometry::IComponent *comp, const Poco::XML::Element *pElem,
    const double angleConvertConst, const bool deltaOffsets) {
  Kernel::V3D retVal; // position relative to parent

  // Polar coordinates can be labelled as (r,t,p) or (R,theta,phi)
  if (pElem->hasAttribute("r") || pElem->hasAttribute("t") ||
      pElem->hasAttribute("p") || pElem->hasAttribute("R") ||
      pElem->hasAttribute("theta") || pElem->hasAttribute("phi")) {
    double R = 0.0, theta = 0.0, phi = 0.0;

    if (pElem->hasAttribute("r"))
      R = atof((pElem->getAttribute("r")).c_str());
    if (pElem->hasAttribute("t"))
      theta = angleConvertConst * atof((pElem->getAttribute("t")).c_str());
    if (pElem->hasAttribute("p"))
      phi = angleConvertConst * atof((pElem->getAttribute("p")).c_str());

    if (pElem->hasAttribute("R"))
      R = atof((pElem->getAttribute("R")).c_str());
    if (pElem->hasAttribute("theta"))
      theta = angleConvertConst * atof((pElem->getAttribute("theta")).c_str());
    if (pElem->hasAttribute("phi"))
      phi = angleConvertConst * atof((pElem->getAttribute("phi")).c_str());

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
    double x = 0.0, y = 0.0, z = 0.0;

    if (pElem->hasAttribute("x"))
      x = atof((pElem->getAttribute("x")).c_str());
    if (pElem->hasAttribute("y"))
      y = atof((pElem->getAttribute("y")).c_str());
    if (pElem->hasAttribute("z"))
      z = atof((pElem->getAttribute("z")).c_str());

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
Poco::XML::Element *InstrumentDefinitionParser::getParentComponent(
    const Poco::XML::Element *pLocElem) {
  if ((pLocElem->tagName()).compare("location") &&
      (pLocElem->tagName()).compare("locations")) {
    std::string tagname = pLocElem->tagName();
    g_log.error("Argument to function getParentComponent must be a pointer to "
                "an XML element with tag name location or locations.");
    throw std::logic_error(
        std::string("Argument to function getParentComponent must be a pointer "
                    "to an XML element") +
        "with tag name location or locations." + " The tag name is " + tagname);
  }

  // The location element is required to be a child of a component element. Get
  // this component element

  Node *pCompNode = pLocElem->parentNode();

  Element *pCompElem;
  if (pCompNode->nodeType() == 1) {
    pCompElem = static_cast<Element *>(pCompNode);
    if ((pCompElem->tagName()).compare("component")) {
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
std::string InstrumentDefinitionParser::getNameOfLocationElement(
    const Poco::XML::Element *pElem, const Poco::XML::Element *pCompElem) {
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
void InstrumentDefinitionParser::setValidityRange(
    const Poco::XML::Element *pRootElem) {
  const std::string filename = m_xmlFile->getFileFullPathStr();
  // check if IDF has valid-from and valid-to tags defined
  if (!pRootElem->hasAttribute("valid-from")) {
    throw Kernel::Exception::InstrumentDefinitionError(
        "<instrument> element must contain a valid-from tag", filename);
  } else {
    try {
      DateAndTime d(pRootElem->getAttribute("valid-from"));
      m_instrument->setValidFromDate(d);
    } catch (...) {
      throw Kernel::Exception::InstrumentDefinitionError(
          "The valid-from <instrument> tag must be a ISO8601 string", filename);
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
      throw Kernel::Exception::InstrumentDefinitionError(
          "The valid-to <instrument> tag must be a ISO8601 string", filename);
    }
  }
}

PointingAlong axisNameToAxisType(std::string &input) {
  PointingAlong direction;
  if (input.compare("x") == 0) {
    direction = X;
  } else if (input.compare("y") == 0) {
    direction = Y;
  } else {
    direction = Z;
  }
  return direction;
}

//-----------------------------------------------------------------------------------------------------------------------
/** Reads the contents of the \<defaults\> element to set member variables,
*  requires m_instrument to be already set
*  @param defaults :: points to the data read from the \<defaults\> element, can
* be null.
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
  Element *defaultFacingElement =
      defaults->getChildElement("components-are-facing");
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
      std::map<std::string, std::string> &units =
          m_instrument->getLogfileUnit();
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
    Element *alongElement =
        referenceFrameElement->getChildElement("along-beam");
    Element *handednessElement =
        referenceFrameElement->getChildElement("handedness");
    Element *originElement = referenceFrameElement->getChildElement("origin");

    // Defaults
    XMLString s_alongBeam("z");
    XMLString s_pointingUp("y");
    XMLString s_handedness("right");
    XMLString s_origin("");

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

    // Convert to input types
    PointingAlong alongBeam = axisNameToAxisType(s_alongBeam);
    PointingAlong pointingUp = axisNameToAxisType(s_pointingUp);
    Handedness handedness = s_handedness.compare("right") == 0 ? Right : Left;

    // Overwrite the default reference frame.
    m_instrument->setReferenceFrame(boost::shared_ptr<ReferenceFrame>(
        new ReferenceFrame(pointingUp, alongBeam, handedness, s_origin)));
  }
}

std::vector<std::string> InstrumentDefinitionParser::buildExcludeList(
    const Poco::XML::Element *const location) {
  // check if <exclude> sub-elements for this location and create new exclude
  // list to pass on
  Poco::AutoPtr<NodeList> pNLexclude =
      location->getElementsByTagName("exclude");
  unsigned long numberExcludeEle = pNLexclude->length();
  std::vector<std::string> newExcludeList;
  for (unsigned long i = 0; i < numberExcludeEle; i++) {
    Element *pExElem = static_cast<Element *>(pNLexclude->item(i));
    if (pExElem->hasAttribute("sub-part"))
      newExcludeList.push_back(pExElem->getAttribute("sub-part"));
  }

  return newExcludeList;
}

//-----------------------------------------------------------------------------------------------------------------------
/** Assumes second argument is a XML location element and its parent is a
*component element
*  which is assigned to be an assembly. This method appends the parent component
*element of
*  the location element to the CompAssembly passed as the 1st arg. Note this
*method may call
*  itself, i.e. it may act recursively.
*
*  @param parent :: CompAssembly to append new component to
*  @param pLocElem ::  Poco::XML element that points to a location element in an
*instrument description XML file, which optionally may be detached (meaning it
*is not required to be part of the DOM tree of the IDF)
*  @param pCompElem :: The Poco::XML \<component\> element that contains the
*\<location\> element
*  @param idList :: The current IDList
*/
void InstrumentDefinitionParser::appendAssembly(
    Geometry::ICompAssembly *parent, const Poco::XML::Element *pLocElem,
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

    if (idlist.compare(idList.idname)) {
      Element *pFound =
          pCompElem->ownerDocument()->getElementById(idlist, "idname");

      if (pFound == NULL) {
        throw Kernel::Exception::InstrumentDefinitionError(
            "No <idlist> with name idname=\"" + idlist +
                "\" present in instrument definition file.",
            filename);
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
  if (pType->hasAttribute("outline") &&
      pType->getAttribute("outline") != "no") {
    ass = new Geometry::ObjCompAssembly(
        InstrumentDefinitionParser::getNameOfLocationElement(pLocElem,
                                                             pCompElem),
        parent);
  } else {
    ass = new Geometry::CompAssembly(
        InstrumentDefinitionParser::getNameOfLocationElement(pLocElem,
                                                             pCompElem),
        parent);
  }

  // set location for this newly added comp and set facing if specified in
  // instrument def. file. Also
  // check if any logfiles are referred to through the <parameter> element.

  setLocation(ass, pLocElem, m_angleConvertConst, m_deltaOffsets);
  setFacing(ass, pLocElem);
  setLogfile(
      ass, pCompElem,
      m_instrument->getLogfileCache()); // params specified within <component>
  setLogfile(
      ass, pLocElem,
      m_instrument
          ->getLogfileCache()); // params specified within specific <location>

  std::string category = "";
  if (pType->hasAttribute("is"))
    category = pType->getAttribute("is");

  // check if special Component
  if (category.compare("SamplePos") == 0 ||
      category.compare("samplePos") == 0) {
    m_instrument->markAsSamplePos(ass);
  }
  if (category.compare("Source") == 0 || category.compare("source") == 0) {
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
    if (pNode->nodeName().compare("location") == 0) {
      // pLocElem is the location of a type. This type is here an assembly and
      // pElem below is a <location> within this type
      const Element *pElem = static_cast<Element *>(pNode);

      // get the parent of pElem, i.e. a pointer to the <component> element that
      // contains pElem
      const Element *pParentElem =
          InstrumentDefinitionParser::getParentComponent(pElem);

      // check if this location is in the exclude list
      std::vector<std::string>::const_iterator it =
          find(excludeList.begin(), excludeList.end(),
               InstrumentDefinitionParser::getNameOfLocationElement(
                   pElem, pParentElem));
      if (it == excludeList.end()) {

        std::string typeName =
            (InstrumentDefinitionParser::getParentComponent(pElem))
                ->getAttribute("type");

        if (isAssembly(typeName)) {
          appendAssembly(ass, pElem, pParentElem, idList);
        } else {
          appendLeaf(ass, pElem, pParentElem, idList);
        }
      }
    }
    if (pNode->nodeName().compare("locations") == 0) {
      const Element *pLocationsElems = static_cast<Element *>(pNode);
      const Element *pParentLocationsElem =
          InstrumentDefinitionParser::getParentComponent(pLocationsElems);

      // append <locations> elements in <locations>
      appendLocations(ass, pLocationsElems, pParentLocationsElem, idList);
    }
    pNode = it.nextNode();
  }

  // create outline object for the assembly
  if (pType->hasAttribute("outline") &&
      pType->getAttribute("outline") != "no") {
    Geometry::ObjCompAssembly *objAss =
        dynamic_cast<Geometry::ObjCompAssembly *>(ass);
    if (pType->getAttribute("object_created") == "no") {
      pType->setAttribute("object_created", "yes");
      boost::shared_ptr<Geometry::Object> obj = objAss->createOutline();
      if (obj) {
        mapTypeNameToShape[pType->getAttribute("name")] = obj;
      } else { // object failed to be created
        pType->setAttribute("outline", "no");
        g_log.warning() << "Failed to create outline object for assembly "
                        << pType->getAttribute("name") << '\n';
      }
    } else {
      objAss->setOutline(mapTypeNameToShape[pType->getAttribute("name")]);
    }
  }
}

//-----------------------------------------------------------------------------------------------------------------------
/** Assumes second argument is pointing to a leaf, which here means the location
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
void InstrumentDefinitionParser::appendLeaf(Geometry::ICompAssembly *parent,
                                            const Poco::XML::Element *pLocElem,
                                            const Poco::XML::Element *pCompElem,
                                            IdList &idList) {
  const std::string filename = m_xmlFile->getFileFullPathStr();

  //--- Get the detector's X/Y pixel sizes (optional) ---
  // Read detector IDs into idlist if required
  // Note idlist may be defined for any component
  // Note any new idlist found will take precedence.

  if (pCompElem->hasAttribute("idlist")) {
    std::string idlist = pCompElem->getAttribute("idlist");

    if (idlist.compare(idList.idname)) {
      Element *pFound =
          pCompElem->ownerDocument()->getElementById(idlist, "idname");

      if (pFound == NULL) {
        throw Kernel::Exception::InstrumentDefinitionError(
            "No <idlist> with name idname=\"" + idlist +
                "\" present in instrument definition file.",
            filename);
      }

      idList.reset();
      populateIdList(pFound, idList);
    }
  }

  // get the type element of the component element in order to determine if the
  // type
  // belong to the category: "detector", "SamplePos or "Source".

  std::string typeName = pCompElem->getAttribute("type");
  Element *pType = getTypeElement[typeName];

  std::string category = "";
  if (pType->hasAttribute("is"))
    category = pType->getAttribute("is");

  // do stuff a bit differently depending on which category the type belong to
  if (category.compare("RectangularDetector") == 0 ||
      category.compare("rectangularDetector") == 0 ||
      category.compare("rectangulardetector") == 0 ||
      category.compare("rectangular_detector") == 0) {
    //-------------- Create a RectangularDetector
    //------------------------------------------------
    std::string name = InstrumentDefinitionParser::getNameOfLocationElement(
        pLocElem, pCompElem);

    // Create the bank with the given parent.
    Geometry::RectangularDetector *bank =
        new Geometry::RectangularDetector(name, parent);

    // set location for this newly added comp and set facing if specified in
    // instrument def. file. Also
    // check if any logfiles are referred to through the <parameter> element.
    setLocation(bank, pLocElem, m_angleConvertConst, m_deltaOffsets);
    setFacing(bank, pLocElem);
    setLogfile(
        bank, pCompElem,
        m_instrument->getLogfileCache()); // params specified within <component>
    setLogfile(
        bank, pLocElem,
        m_instrument
            ->getLogfileCache()); // params specified within specific <location>

    // Extract all the parameters from the XML attributes
    int xpixels = 0;
    double xstart = 0.;
    double xstep = 0.;
    int ypixels = 0;
    double ystart = 0.;
    double ystep = 0.;
    int idstart = 0;
    bool idfillbyfirst_y = true;
    int idstepbyrow = 0;
    int idstep = 1;

    // The shape!
    // Given that this leaf component is actually an assembly, its constituent
    // component detector shapes comes from its type attribute.
    const std::string shapeType = pType->getAttribute("type");
    boost::shared_ptr<Geometry::Object> shape = mapTypeNameToShape[shapeType];

    // These parameters are in the TYPE defining RectangularDetector
    if (pType->hasAttribute("xpixels"))
      xpixels = atoi((pType->getAttribute("xpixels")).c_str());
    if (pType->hasAttribute("xstart"))
      xstart = atof((pType->getAttribute("xstart")).c_str());
    if (pType->hasAttribute("xstep"))
      xstep = atof((pType->getAttribute("xstep")).c_str());
    if (pType->hasAttribute("ypixels"))
      ypixels = atoi((pType->getAttribute("ypixels")).c_str());
    if (pType->hasAttribute("ystart"))
      ystart = atof((pType->getAttribute("ystart")).c_str());
    if (pType->hasAttribute("ystep"))
      ystep = atof((pType->getAttribute("ystep")).c_str());

    // THESE parameters are in the INSTANCE of this type - since they will
    // change.
    if (pCompElem->hasAttribute("idstart"))
      idstart = atoi((pCompElem->getAttribute("idstart")).c_str());
    if (pCompElem->hasAttribute("idfillbyfirst"))
      idfillbyfirst_y = (pCompElem->getAttribute("idfillbyfirst") == "y");
    // Default ID row step size
    if (idfillbyfirst_y)
      idstepbyrow = ypixels;
    else
      idstepbyrow = xpixels;
    if (pCompElem->hasAttribute("idstepbyrow")) {
      idstepbyrow = atoi((pCompElem->getAttribute("idstepbyrow")).c_str());
    }
    // Default ID row step size
    if (pCompElem->hasAttribute("idstep"))
      idstep = atoi((pCompElem->getAttribute("idstep")).c_str());

    // Now, initialize all the pixels in the bank
    bank->initialize(shape, xpixels, xstart, xstep, ypixels, ystart, ystep,
                     idstart, idfillbyfirst_y, idstepbyrow, idstep);

    // Loop through all detectors in the newly created bank and mark those in
    // the instrument.
    try {
      for (int x = 0; x < bank->nelements(); x++) {
        boost::shared_ptr<Geometry::ICompAssembly> xColumn =
            boost::dynamic_pointer_cast<Geometry::ICompAssembly>((*bank)[x]);
        for (int y = 0; y < xColumn->nelements(); y++) {
          boost::shared_ptr<Geometry::Detector> detector =
              boost::dynamic_pointer_cast<Geometry::Detector>((*xColumn)[y]);
          if (detector) {
            // Make default facing for the pixel
            Geometry::IComponent *comp = (Geometry::IComponent *)detector.get();
            if (m_haveDefaultFacing)
              makeXYplaneFaceComponent(comp, m_defaultFacing);
            // Mark it as a detector (add to the instrument cache)
            m_instrument->markAsDetector(detector.get());
          }
        }
      }
    } catch (Kernel::Exception::ExistsError &) {
      throw Kernel::Exception::InstrumentDefinitionError(
          "Duplicate detector ID found when adding RectangularDetector " +
          name + " in XML instrument file" + filename);
    }
  } else if (category.compare("Detector") == 0 ||
             category.compare("detector") == 0 ||
             category.compare("Monitor") == 0 ||
             category.compare("monitor") == 0) {
    //-------------- Create a Detector
    //------------------------------------------------
    std::string name = InstrumentDefinitionParser::getNameOfLocationElement(
        pLocElem, pCompElem);

    // before setting detector ID check that the IDF satisfies the following

    if (idList.counted >= static_cast<int>(idList.vec.size())) {
      std::stringstream ss1, ss2;
      ss1 << idList.vec.size();
      ss2 << idList.counted;
      if (idList.idname == "") {
        g_log.error("No list of detector IDs found for location element " +
                    name);
        throw Kernel::Exception::InstrumentDefinitionError(
            "Detector location element " + name + " has no idlist.", filename);
      } else if (idList.vec.size() == 0) {
        g_log.error("No detector IDs found for detectors in list " +
                    idList.idname);
      } else {
        g_log.error("The number of detector IDs listed in idlist named " +
                    idList.idname + " is less then the number of detectors");
      }
      throw Kernel::Exception::InstrumentDefinitionError(
          "Number of IDs listed in idlist (=" + ss1.str() +
              ") is less than the number of detectors.",
          filename);
    }

    // Create detector and increment id. Finally add the detector to the parent
    Geometry::Detector *detector = new Geometry::Detector(
        name, idList.vec[idList.counted], mapTypeNameToShape[typeName], parent);
    idList.counted++;
    parent->add(detector);

    // set location for this newly added comp and set facing if specified in
    // instrument def. file. Also
    // check if any logfiles are referred to through the <parameter> element.
    setLocation(detector, pLocElem, m_angleConvertConst, m_deltaOffsets);
    setFacing(detector, pLocElem);
    setLogfile(
        detector, pCompElem,
        m_instrument->getLogfileCache()); // params specified within <component>
    setLogfile(
        detector, pLocElem,
        m_instrument
            ->getLogfileCache()); // params specified within specific <location>

    // If enabled, check for a 'neutronic position' tag and add to cache
    // (null pointer added INTENTIONALLY if not found)
    if (m_indirectPositions) {
      m_neutronicPos[detector] = pLocElem->getChildElement("neutronic");
    }

    // mark-as is a depricated attribute used before is="monitor" was introduced
    if (pCompElem->hasAttribute("mark-as") ||
        pLocElem->hasAttribute("mark-as")) {
      g_log.warning() << "Attribute 'mark-as' is a depricated attribute in "
                         "Instrument Definition File."
                      << " Please see the deprecated section of "
                         "www.mantidproject.org/IDF for how to remove this "
                         "warning message\n";
    }

    try {
      if (category.compare("Monitor") == 0 || category.compare("monitor") == 0)
        m_instrument->markAsMonitor(detector);
      else {
        // for backwards compatebility look for mark-as="monitor"
        if ((pCompElem->hasAttribute("mark-as") &&
             pCompElem->getAttribute("mark-as").compare("monitor") == 0) ||
            (pLocElem->hasAttribute("mark-as") &&
             pLocElem->getAttribute("mark-as").compare("monitor") == 0)) {
          m_instrument->markAsMonitor(detector);
        } else
          m_instrument->markAsDetector(detector);
      }

    } catch (Kernel::Exception::ExistsError &) {
      std::stringstream convert;
      convert << detector->getID();
      throw Kernel::Exception::InstrumentDefinitionError(
          "Detector with ID = " + convert.str() +
              " present more then once in XML instrument file",
          filename);
    }

    // Add all monitors and detectors to 'facing component' container. This is
    // only used if the
    // "facing" elements are defined in the instrument definition file
    m_facingComponent.push_back(detector);
  } else {
    //-------------- Not a Detector nor a RectangularDetector
    //------------------------------
    std::string name = InstrumentDefinitionParser::getNameOfLocationElement(
        pLocElem, pCompElem);

    auto comp =
        new Geometry::ObjComponent(name, mapTypeNameToShape[typeName], parent);
    parent->add(comp);

    // check if special Source or SamplePos Component
    if (category.compare("Source") == 0 || category.compare("source") == 0) {
      m_instrument->markAsSource(comp);
    }
    if (category.compare("SamplePos") == 0 ||
        category.compare("samplePos") == 0) {
      m_instrument->markAsSamplePos(comp);
    }
    if (category.compare("ChopperPos") == 0 ||
        category.compare("chopperPos") == 0) {
      m_instrument->markAsChopperPoint(comp);
    }

    // set location for this newly added comp and set facing if specified in
    // instrument def. file. Also
    // check if any logfiles are referred to through the <parameter> element.

    setLocation(comp, pLocElem, m_angleConvertConst, m_deltaOffsets);
    setFacing(comp, pLocElem);
    setLogfile(
        comp, pCompElem,
        m_instrument->getLogfileCache()); // params specified within <component>
    setLogfile(
        comp, pLocElem,
        m_instrument
            ->getLogfileCache()); // params specified within specific <location>
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
void InstrumentDefinitionParser::populateIdList(Poco::XML::Element *pE,
                                                IdList &idList) {
  const std::string filename = m_xmlFile->getFileFullPathStr();

  if ((pE->tagName()).compare("idlist")) {
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
    int startID = atoi((pE->getAttribute("start")).c_str());

    int endID;
    if (pE->hasAttribute("end"))
      endID = atoi((pE->getAttribute("end")).c_str());
    else
      endID = startID;

    int increment = 1;
    if (pE->hasAttribute("step"))
      increment = atoi((pE->getAttribute("step")).c_str());

    // check the start end and increment values are sensible
    if (((endID - startID) / increment) < 0) {
      std::stringstream ss;
      ss << "The start, end, and step elements do not allow a single id in the "
            "idlist entry - ";
      ss << "start: " << startID << ",  end: " << endID
         << ", step: " << increment;

      throw Kernel::Exception::InstrumentDefinitionError(ss.str(), filename);
    }

    idList.vec.reserve((endID - startID) / increment);
    for (int i = startID; i != endID + increment; i += increment) {
      idList.vec.push_back(i);
    }
  } else {
    // test first if any <id> elements

    Poco::AutoPtr<NodeList> pNL = pE->getElementsByTagName("id");

    if (pNL->length() == 0) {
      throw Kernel::Exception::InstrumentDefinitionError(
          "No id subelement of idlist element in XML instrument file",
          filename);
    }

    // get id numbers

    NodeIterator it(pE, NodeFilter::SHOW_ELEMENT);

    Node *pNode = it.nextNode();
    while (pNode) {
      if (pNode->nodeName().compare("id") == 0) {
        Element *pIDElem = static_cast<Element *>(pNode);

        if (pIDElem->hasAttribute("val")) {
          int valID = atoi((pIDElem->getAttribute("val")).c_str());
          idList.vec.push_back(valID);
        } else if (pIDElem->hasAttribute("start")) {
          int startID = atoi((pIDElem->getAttribute("start")).c_str());

          int endID;
          if (pIDElem->hasAttribute("end"))
            endID = atoi((pIDElem->getAttribute("end")).c_str());
          else
            endID = startID;

          int increment = 1;
          if (pIDElem->hasAttribute("step"))
            increment = atoi((pIDElem->getAttribute("step")).c_str());

          // check the start end and increment values are sensible
          if (((endID - startID) / increment) < 0) {
            std::stringstream ss;
            ss << "The start, end, and step elements do not allow a single id "
                  "in the idlist entry - ";
            ss << "start: " << startID << ",  end: " << endID
               << ", step: " << increment;

            throw Kernel::Exception::InstrumentDefinitionError(ss.str(),
                                                               filename);
          }

          idList.vec.reserve((endID - startID) / increment);
          for (int i = startID; i != endID + increment; i += increment) {
            idList.vec.push_back(i);
          }
        } else {
          throw Kernel::Exception::InstrumentDefinitionError(
              "id subelement of idlist " +
                  std::string(
                      "element wrongly specified in XML instrument file"),
              filename);
        }
      }

      pNode = it.nextNode();
    } // end while loop
  }
}

//-----------------------------------------------------------------------------------------------------------------------
/** Returns True if the (string) type given is an assembly.
 *
 *  @param type ::  name of the type of a component in XML instrument definition
 *  @return True if the type is an assembly
 *  @throw InstrumentDefinitionError Thrown if type not defined in XML
 *definition
*/
bool InstrumentDefinitionParser::isAssembly(std::string type) const {
  const std::string filename = m_xmlFile->getFileFullPathStr();
  std::map<std::string, bool>::const_iterator it = isTypeAssembly.find(type);

  if (it == isTypeAssembly.end()) {
    throw Kernel::Exception::InstrumentDefinitionError(
        "type with name = " + type + " not defined.", filename);
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
void InstrumentDefinitionParser::makeXYplaneFaceComponent(
    Geometry::IComponent *&in, const Geometry::ObjComponent *facing) {
  makeXYplaneFaceComponent(in, facing->getPos());
}

//-----------------------------------------------------------------------------------------------------------------------
/** Make the shape defined in 1st argument face the position in the second
*argument,
*  by rotating the z-axis of the component passed in 1st argument so that it
*points in the
*  direction: from the position (as specified 2nd argument) to the component
*(1st argument).
*
*  @param in ::  Component to be rotated
*  @param facingPoint :: position to face
*/
void InstrumentDefinitionParser::makeXYplaneFaceComponent(
    Geometry::IComponent *&in, const Kernel::V3D &facingPoint) {
  Kernel::V3D pos = in->getPos();

  // vector from facing object to component we want to rotate
  Kernel::V3D facingDirection = pos - facingPoint;
  facingDirection.normalize();

  if (facingDirection.norm() == 0.0)
    return;

  // now aim to rotate shape such that the z-axis of of the object we want to
  // rotate
  // points in the direction of facingDirection. That way the XY plane faces the
  // 'facing object'.
  Kernel::V3D z = Kernel::V3D(0, 0, 1);
  Kernel::Quat R = in->getRotation();
  R.inverse();
  R.rotate(facingDirection);

  Kernel::V3D normal = facingDirection.cross_prod(z);
  normal.normalize();
  double theta = (180.0 / M_PI) * facingDirection.angle(z);

  if (normal.norm() > 0.0)
    in->rotate(Kernel::Quat(-theta, normal));
  else {
    // To take into account the case where the facing direction is in the
    // (0,0,1)
    // or (0,0,-1) direction.
    in->rotate(Kernel::Quat(-theta, Kernel::V3D(0, 1, 0)));
  }
}

//-----------------------------------------------------------------------------------------------------------------------
/** Parse position of facing element to V3D
*
*  @param pElem ::  Facing type element to parse
*  @return Return parsed position as a V3D
*/
Kernel::V3D
InstrumentDefinitionParser::parseFacingElementToV3D(Poco::XML::Element *pElem) {
  Kernel::V3D retV3D;

  // Polar coordinates can be labelled as (r,t,p) or (R,theta,phi)
  if (pElem->hasAttribute("r") || pElem->hasAttribute("t") ||
      pElem->hasAttribute("p") || pElem->hasAttribute("R") ||
      pElem->hasAttribute("theta") || pElem->hasAttribute("phi")) {
    double R = 0.0, theta = 0.0, phi = 0.0;

    if (pElem->hasAttribute("r"))
      R = atof((pElem->getAttribute("r")).c_str());
    if (pElem->hasAttribute("t"))
      theta = m_angleConvertConst * atof((pElem->getAttribute("t")).c_str());
    if (pElem->hasAttribute("p"))
      phi = m_angleConvertConst * atof((pElem->getAttribute("p")).c_str());

    if (pElem->hasAttribute("R"))
      R = atof((pElem->getAttribute("R")).c_str());
    if (pElem->hasAttribute("theta"))
      theta =
          m_angleConvertConst * atof((pElem->getAttribute("theta")).c_str());
    if (pElem->hasAttribute("phi"))
      phi = m_angleConvertConst * atof((pElem->getAttribute("phi")).c_str());

    retV3D.spherical(R, theta, phi);
  } else {
    double x = 0.0, y = 0.0, z = 0.0;

    if (pElem->hasAttribute("x"))
      x = atof((pElem->getAttribute("x")).c_str());
    if (pElem->hasAttribute("y"))
      y = atof((pElem->getAttribute("y")).c_str());
    if (pElem->hasAttribute("z"))
      z = atof((pElem->getAttribute("z")).c_str());

    retV3D(x, y, z);
  }

  return retV3D;
}

//-----------------------------------------------------------------------------------------------------------------------
/** Set facing of comp as specified in XML facing element (which must be
*sub-element of a location element).
*
*  @param comp :: To set facing of
*  @param pElem ::  Poco::XML element that points a \<location\> element, which
*optionally may be detached (meaning it is not required to be part of the DOM
*tree of the IDF)
*
*  @throw logic_error Thrown if second argument is not a pointer to a 'location'
*XML element
*/
void InstrumentDefinitionParser::setFacing(Geometry::IComponent *comp,
                                           const Poco::XML::Element *pElem) {
  // Require that pElem points to an element with tag name 'location'

  if ((pElem->tagName()).compare("location")) {
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
      double rotAngle =
          m_angleConvertConst * atof((facingElem->getAttribute("rot"))
                                         .c_str()); // assumed to be in degrees
      comp->rotate(Kernel::Quat(rotAngle, Kernel::V3D(0, 0, 1)));
    }

    // For now assume that if has val attribute it means facing = none. This
    // option only has an
    // effect when a default facing setting is set. In which case this then
    // means "ignore the
    // default facing setting" for this component

    if (facingElem->hasAttribute("val"))
      return;

    // Face the component, i.e. rotate the z-axis of the component such that it
    // points in the direction from
    // the point x,y,z (or r,t,p) specified by the <facing> xml element towards
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
*
*  @throw InstrumentDefinitionError Thrown if issues with the content of XML
*instrument file
*/
void
InstrumentDefinitionParser::setLogfile(const Geometry::IComponent *comp,
                                       const Poco::XML::Element *pElem,
                                       InstrumentParameterCache &logfileCache) {
  const std::string filename = m_xmlFile->getFileFullPathStr();

  // The purpose below is to have a quicker way to judge if pElem contains a
  // parameter, see
  // defintion of m_hasParameterElement for more info
  if (m_hasParameterElement_beenSet)
    if (m_hasParameterElement.end() == std::find(m_hasParameterElement.begin(),
                                                 m_hasParameterElement.end(),
                                                 pElem))
      return;

  Poco::AutoPtr<NodeList> pNL_comp =
      pElem->childNodes(); // here get all child nodes
  unsigned long pNL_comp_length = pNL_comp->length();

  for (unsigned long i = 0; i < pNL_comp_length; i++) {
    // we are only interest in the top level parameter elements hence
    // the reason for the if statement below
    if ((pNL_comp->item(i))->nodeType() == Node::ELEMENT_NODE &&
        ((pNL_comp->item(i))->nodeName()).compare("parameter") == 0) {
      Element *pParamElem = static_cast<Element *>(pNL_comp->item(i));

      if (!pParamElem->hasAttribute("name"))
        throw Kernel::Exception::InstrumentDefinitionError(
            "XML element with name or type = " + comp->getName() +
                " contain <parameter> element with no name attribute in XML "
                "instrument file",
            filename);

      std::string paramName = pParamElem->getAttribute("name");

      if (paramName.compare("rot") == 0 || paramName.compare("pos") == 0) {
        g_log.error()
            << "XML element with name or type = " << comp->getName()
            << " contains <parameter> element with name=\"" << paramName
            << "\"."
            << " This is a reserved Mantid keyword. Please use other name, "
            << "and see www.mantidproject.org/IDF for list of reserved "
               "keywords."
            << " This parameter is ignored";
        continue;
      }

      std::string logfileID = "";
      std::string value = "";

      std::string type = "double";               // default
      std::string extractSingleValueAs = "mean"; // default
      std::string eq = "";

      Poco::AutoPtr<NodeList> pNLvalue =
          pParamElem->getElementsByTagName("value");
      size_t numberValueEle = pNLvalue->length();
      Element *pValueElem;

      Poco::AutoPtr<NodeList> pNLlogfile =
          pParamElem->getElementsByTagName("logfile");
      size_t numberLogfileEle = pNLlogfile->length();
      Element *pLogfileElem;

      Poco::AutoPtr<NodeList> pNLLookUp =
          pParamElem->getElementsByTagName("lookuptable");
      size_t numberLookUp = pNLLookUp->length();

      Poco::AutoPtr<NodeList> pNLFormula =
          pParamElem->getElementsByTagName("formula");
      size_t numberFormula = pNLFormula->length();

      if (numberValueEle + numberLogfileEle + numberLookUp + numberFormula >
          1) {
        g_log.warning() << "XML element with name or type = " << comp->getName()
                        << " contains <parameter> element where the value of "
                           "the parameter has been specified"
                        << " more than once. See www.mantidproject.org/IDF for "
                           "how the value"
                        << " of the parameter is set in this case.";
      }

      if (numberValueEle + numberLogfileEle + numberLookUp + numberFormula ==
          0) {
        g_log.error()
            << "XML element with name or type = " << comp->getName()
            << " contains <parameter> for which no value is specified."
            << " See www.mantidproject.org/IDF for how to set the value"
            << " of a parameter. This parameter is ignored.";
        continue;
      }

      // if more than one <value> specified for a parameter use only the first
      // <value> element
      if (numberValueEle >= 1) {
        pValueElem = static_cast<Element *>(pNLvalue->item(0));
        if (!pValueElem->hasAttribute("val"))
          throw Kernel::Exception::InstrumentDefinitionError(
              "XML element with name or type = " + comp->getName() +
                  " contains <parameter> element with invalid syntax for its "
                  "subelement <value>." +
                  " Correct syntax is <value val=\"\"/>",
              filename);
        value = pValueElem->getAttribute("val");
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
          extractSingleValueAs =
              pLogfileElem->getAttribute("extract-single-value-as");
      }

      if (pParamElem->hasAttribute("type"))
        type = pParamElem->getAttribute("type");

      // check if <fixed /> element present

      bool fixed = false;
      Poco::AutoPtr<NodeList> pNLFixed =
          pParamElem->getElementsByTagName("fixed");
      size_t numberFixed = pNLFixed->length();
      if (numberFixed >= 1) {
        fixed = true;
      }

      // some processing

      std::string fittingFunction = "";
      std::string tie = "";

      if (type.compare("fitting") == 0) {
        size_t found = paramName.find(":");
        if (found != std::string::npos) {
          // check that only one : in name
          size_t index = paramName.find(":", found + 1);
          if (index != std::string::npos) {
            g_log.error()
                << "Fitting <parameter> in instrument definition file defined "
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
        Element *pMin = static_cast<Element *>(pNLMin->item(0));
        constraint[0] = pMin->getAttribute("val");
      }
      if (numberMax >= 1) {
        Element *pMax = static_cast<Element *>(pNLMax->item(0));
        constraint[1] = pMax->getAttribute("val");
      }

      // check if penalty-factor> elements present

      std::string penaltyFactor;

      Poco::AutoPtr<NodeList> pNL_penaltyFactor =
          pParamElem->getElementsByTagName("penalty-factor");
      size_t numberPenaltyFactor = pNL_penaltyFactor->length();

      if (numberPenaltyFactor >= 1) {
        Element *pPenaltyFactor =
            static_cast<Element *>(pNL_penaltyFactor->item(0));
        penaltyFactor = pPenaltyFactor->getAttribute("val");
      }

      // Check if look up table is specified

      std::vector<std::string> allowedUnits = UnitFactory::Instance().getKeys();

      boost::shared_ptr<Interpolation> interpolation(new Interpolation);

      if (numberLookUp >= 1) {
        Element *pLookUp = static_cast<Element *>(pNLLookUp->item(0));

        if (pLookUp->hasAttribute("interpolation"))
          interpolation->setMethod(pLookUp->getAttribute("interpolation"));
        if (pLookUp->hasAttribute("x-unit")) {
          std::vector<std::string>::iterator it;
          it = find(allowedUnits.begin(), allowedUnits.end(),
                    pLookUp->getAttribute("x-unit"));
          if (it == allowedUnits.end()) {
            g_log.warning() << "x-unit used with interpolation table must be "
                               "one of the recognised units "
                            << " see http://www.mantidproject.org/Unit_Factory";
          } else
            interpolation->setXUnit(pLookUp->getAttribute("x-unit"));
        }
        if (pLookUp->hasAttribute("y-unit")) {
          std::vector<std::string>::iterator it;
          it = find(allowedUnits.begin(), allowedUnits.end(),
                    pLookUp->getAttribute("y-unit"));
          if (it == allowedUnits.end()) {
            g_log.warning() << "y-unit used with interpolation table must be "
                               "one of the recognised units "
                            << " see http://www.mantidproject.org/Unit_Factory";
          } else
            interpolation->setYUnit(pLookUp->getAttribute("y-unit"));
        }

        Poco::AutoPtr<NodeList> pNLpoint =
            pLookUp->getElementsByTagName("point");
        unsigned long numberPoint = pNLpoint->length();

        for (unsigned long i = 0; i < numberPoint; i++) {
          Element *pPoint = static_cast<Element *>(pNLpoint->item(i));
          double x = atof(pPoint->getAttribute("x").c_str());
          double y = atof(pPoint->getAttribute("y").c_str());
          interpolation->addPoint(x, y);
        }
      }

      // Check if formula is specified

      std::string formula = "";
      std::string formulaUnit = "";
      std::string resultUnit = "";

      if (numberFormula >= 1) {
        Element *pFormula = static_cast<Element *>(pNLFormula->item(0));
        formula = pFormula->getAttribute("eq");
        if (pFormula->hasAttribute("unit")) {
          std::vector<std::string>::iterator it;
          it = find(allowedUnits.begin(), allowedUnits.end(),
                    pFormula->getAttribute("unit"));
          if (it == allowedUnits.end()) {
            g_log.warning() << "unit attribute used with formula must be one "
                               "of the recognised units "
                            << " see http://www.mantidproject.org/Unit_Factory";
          } else
            formulaUnit = pFormula->getAttribute("unit");
        }
        if (pFormula->hasAttribute("result-unit"))
          resultUnit = pFormula->getAttribute("result-unit");
      }

      auto cacheKey = std::make_pair(paramName, comp);
      auto cacheValue =
          boost::shared_ptr<XMLInstrumentParameter>(new XMLInstrumentParameter(
              logfileID, value, interpolation, formula, formulaUnit, resultUnit,
              paramName, type, tie, constraint, penaltyFactor, fittingFunction,
              extractSingleValueAs, eq, comp, m_angleConvertConst));
      auto inserted = logfileCache.insert(std::make_pair(cacheKey, cacheValue));
      if (!inserted.second) {
        logfileCache[cacheKey] = cacheValue;
      }
    } // end of if statement
  }
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
*/
void InstrumentDefinitionParser::setComponentLinks(
    boost::shared_ptr<Geometry::Instrument> &instrument,
    Poco::XML::Element *pRootElem, Kernel::ProgressBase *progress) {
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
    progress->resetNumSteps((int64_t)numberLinks, 0.0, 0.95);

  Node *curNode = pRootElem->firstChild();
  while (curNode) {
    if (curNode->nodeType() == Node::ELEMENT_NODE &&
        curNode->nodeName() == elemName) {
      Element *curElem = static_cast<Element *>(curNode);

      if (progress) {
        if (progress->hasCancellationBeenRequested())
          return;
        progress->report("Loading parameters");
      }

      std::string id = curElem->getAttribute("id");
      std::string name = curElem->getAttribute("name");
      std::vector<boost::shared_ptr<const Geometry::IComponent>> sharedIComp;

      // If available, use the detector id as it's the most specific.
      if (id.length() > 0) {
        int detid;
        std::stringstream(id) >> detid;
        boost::shared_ptr<const Geometry::IComponent> detector =
            instrument->getDetector((detid_t)detid);

        // If we didn't find anything with the detector id, explain why to the
        // user, and throw an exception.
        if (!detector) {
          g_log.error()
              << "Error whilst loading parameters. No detector found with id '"
              << detid << "'" << std::endl;
          g_log.error() << "Please check that your detectors' ids are correct."
                        << std::endl;
          throw Kernel::Exception::InstrumentDefinitionError(
              "Invalid detector id in component-link tag.");
        }

        sharedIComp.push_back(detector);

        // If the user also supplied a name, make sure it's consistent with the
        // detector id.
        if (name.length() > 0) {
          auto comp = boost::dynamic_pointer_cast<const IComponent>(detector);
          if (comp) {
            bool consistent =
                (comp->getFullName() == name || comp->getName() == name);
            if (!consistent) {
              g_log.warning() << "Error whilst loading parameters. Name '"
                              << name << "' does not match id '" << detid
                              << "'." << std::endl;
              g_log.warning()
                  << "Parameters have been applied to detector with id '"
                  << detid << "'. Please check the name is correct."
                  << std::endl;
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
          boost::shared_ptr<const Geometry::IComponent> shared =
              instrument->getComponentByName(name);
          sharedIComp.push_back(shared);
        }
      }

      for (size_t i = 0; i < sharedIComp.size(); i++) {
        boost::shared_ptr<const Geometry::Component> sharedComp =
            boost::dynamic_pointer_cast<const Geometry::Component>(
                sharedIComp[i]);
        if (sharedComp) {
          // Not empty Component
          if (sharedComp->isParametrized()) {
            setLogfile(sharedComp->base(), curElem,
                       instrument->getLogfileCache());
          } else {
            setLogfile(sharedIComp[i].get(), curElem,
                       instrument->getLogfileCache());
          }
        }
      }
    }
    curNode = curNode->nextSibling();
  }
}

/**
Check that the cache file does actually exist and that it was modified last
after the last modification to the xml def file. i.e. the vtp file contains the
most recent set of changes.
@param cacheCandiate : candiate cache file object to use the the geometries.
*/
bool InstrumentDefinitionParser::canUseProposedCacheFile(
    IDFObject_const_sptr cacheCandiate) const {
  return m_xmlFile->exists() && cacheCandiate->exists() &&
         (m_xmlFile->getLastModified() < cacheCandiate->getLastModified());
}

/**
Apply the cache.
@param cacheToApply : Cache file object to use the the geometries.
*/
void InstrumentDefinitionParser::applyCache(IDFObject_const_sptr cacheToApply) {
  const std::string cacheFullPath = cacheToApply->getFileFullPathStr();
  g_log.information("Loading geometry cache from " + cacheFullPath);
  // create a vtk reader
  std::map<std::string, boost::shared_ptr<Geometry::Object>>::iterator objItr;
  boost::shared_ptr<Mantid::Geometry::vtkGeometryCacheReader> reader(
      new Mantid::Geometry::vtkGeometryCacheReader(cacheFullPath));
  for (objItr = mapTypeNameToShape.begin(); objItr != mapTypeNameToShape.end();
       ++objItr) {
    ((*objItr).second)->setVtkGeometryCacheReader(reader);
  }
}

/**
Write the cache file from the IDF file and apply it.
@param fallBackCache : File location for a fallback cache if required.
*/
InstrumentDefinitionParser::CachingOption
InstrumentDefinitionParser::writeAndApplyCache(
    IDFObject_const_sptr fallBackCache) {
  IDFObject_const_sptr usedCache = m_cacheFile;
  InstrumentDefinitionParser::CachingOption cachingOption = WroteCacheAdjacent;

  g_log.information("Geometry cache is not available");
  try {
    Poco::File dir = m_xmlFile->getParentDirectory();
    if (!m_xmlFile->exists() || dir.path().empty() || !dir.exists() ||
        !dir.canWrite()) {
      usedCache = fallBackCache;
      cachingOption = WroteCacheTemp;
      g_log.information() << "Instrument directory is read only, writing cache "
                             "to system temp.\n";
    }
  } catch (Poco::FileNotFoundException &) {
    g_log.error() << "Unable to find instrument definition while attempting to "
                     "write cache.\n";
    throw std::runtime_error("Unable to find instrument definition while "
                             "attempting to write cache.\n");
  }
  const std::string cacheFullPath = usedCache->getFileFullPathStr();
  g_log.information() << "Creating cache in " << cacheFullPath << "\n";
  // create a vtk writer
  std::map<std::string, boost::shared_ptr<Geometry::Object>>::iterator objItr;
  boost::shared_ptr<Mantid::Geometry::vtkGeometryCacheWriter> writer(
      new Mantid::Geometry::vtkGeometryCacheWriter(cacheFullPath));
  for (objItr = mapTypeNameToShape.begin(); objItr != mapTypeNameToShape.end();
       ++objItr) {
    ((*objItr).second)->setVtkGeometryCacheWriter(writer);
  }
  writer->write();
  return cachingOption;
}

/** Reads in or creates the geometry cache ('vtp') file
@return CachingOption selected.
*/
InstrumentDefinitionParser::CachingOption
InstrumentDefinitionParser::setupGeometryCache() {
  // Get cached file name
  // If the instrument directory is writable, put them there else use temporary
  // directory.
  IDFObject_const_sptr fallBackCache = boost::make_shared<const IDFObject>(
      Poco::Path(ConfigService::Instance().getTempDir())
          .append(m_instName + ".vtp")
          .toString());
  CachingOption cachingOption = NoneApplied;
  if (canUseProposedCacheFile(m_cacheFile)) {
    applyCache(m_cacheFile);
    cachingOption = ReadAdjacent;
  } else if (canUseProposedCacheFile(fallBackCache)) {
    applyCache(fallBackCache);
    cachingOption = ReadFallBack;
  } else {
    cachingOption = writeAndApplyCache(fallBackCache);
  }
  return cachingOption;
}

/**
Getter for the applied caching option.
@return selected caching.
*/
InstrumentDefinitionParser::CachingOption
InstrumentDefinitionParser::getAppliedCachingOption() const {
  return m_cachingOption;
}

void InstrumentDefinitionParser::createNeutronicInstrument() {
  // Create a copy of the instrument
  Instrument_sptr physical(new Instrument(*m_instrument));
  // Store the physical instrument 'inside' the neutronic instrument
  m_instrument->setPhysicalInstrument(physical);

  // Now we manipulate the original instrument (m_instrument) to hold neutronic
  // positions
  std::map<IComponent *, Poco::XML::Element *>::const_iterator it;
  for (it = m_neutronicPos.begin(); it != m_neutronicPos.end(); ++it) {
    if (it->second) {
      setLocation(it->first, it->second, m_angleConvertConst, m_deltaOffsets);
      // TODO: Do we need to deal with 'facing'???

      // Check for a 'type' attribute, indicating that we want to set the
      // neutronic shape
      if (it->second->hasAttribute("type") &&
          dynamic_cast<ObjComponent *>(it->first)) {
        const Poco::XML::XMLString shapeName = it->second->getAttribute("type");
        std::map<std::string, Object_sptr>::const_iterator shapeIt =
            mapTypeNameToShape.find(shapeName);
        if (shapeIt != mapTypeNameToShape.end()) {
          // Change the shape on the current component to the one requested
          dynamic_cast<ObjComponent *>(it->first)->setShape(shapeIt->second);
        } else {
          throw Exception::InstrumentDefinitionError(
              "Requested type " + shapeName + " not defined in IDF");
        }
      }
    } else // We have a null Element*, which signals a detector with no
           // neutronic position
    {
      // This should only happen for detectors
      Detector *det = dynamic_cast<Detector *>(it->first);
      if (det)
        m_instrument->removeDetector(det);
    }
  }
}

/** Takes as input a \<type\> element containing a
*<combine-components-into-one-shape>, and
*   adjust the \<type\> element by replacing its containing \<component\>
*elements with \<cuboid\>'s
*   (note for now this will only work for \<cuboid\>'s and when necessary this
*can be extended).
*
*  @param pElem ::  Poco::XML \<type\> element that we want to adjust
*  @param isTypeAssembly [in] :: tell whether any other type, but the special
*one treated here, is assembly or not
*  @param getTypeElement [in] :: contain pointers to all types but the onces
*treated here
*
*  @throw InstrumentDefinitionError Thrown if issues with the content of XML
*instrument file
*/
void InstrumentDefinitionParser::adjust(
    Poco::XML::Element *pElem, std::map<std::string, bool> &isTypeAssembly,
    std::map<std::string, Poco::XML::Element *> &getTypeElement) {
  UNUSED_ARG(isTypeAssembly)
  // check if pElem is an element with tag name 'type'
  if ((pElem->tagName()).compare("type"))
    throw Exception::InstrumentDefinitionError("Argument to function adjust() "
                                               "must be a pointer to an XML "
                                               "element with tag name type.");

  // check that there is a <combine-components-into-one-shape> element in type
  Poco::AutoPtr<NodeList> pNLccioh =
      pElem->getElementsByTagName("combine-components-into-one-shape");
  if (pNLccioh->length() == 0) {
    throw Exception::InstrumentDefinitionError(
        std::string("Argument to function adjust() must be a pointer to an XML "
                    "element with tag name type,") +
        " which contain a <combine-components-into-one-shape> element.");
  }

  // check that there is a <algebra> element in type
  Poco::AutoPtr<NodeList> pNLalg = pElem->getElementsByTagName("algebra");
  if (pNLalg->length() == 0) {
    throw Exception::InstrumentDefinitionError(
        std::string("An <algebra> element must be part of a <type>, which") +
        " includes a <combine-components-into-one-shape> element. See "
        "www.mantidproject.org/IDF.");
  }

  // check that there is a <location> element in type
  Poco::AutoPtr<NodeList> pNL = pElem->getElementsByTagName("location");
  unsigned long numLocation = pNL->length();
  if (numLocation == 0) {
    throw Exception::InstrumentDefinitionError(
        std::string(
            "At least one <location> element must be part of a <type>, which") +
        " includes a <combine-components-into-one-shape> element. See "
        "www.mantidproject.org/IDF.");
  }

  // check if a <translate-rotate-combined-shape-to> is defined
  Poco::AutoPtr<NodeList> pNL_TransRot =
      pElem->getElementsByTagName("translate-rotate-combined-shape-to");
  Element *pTransRot = 0;
  if (pNL_TransRot->length() == 1) {
    pTransRot = static_cast<Element *>(pNL_TransRot->item(0));
  }

  // to convert all <component>'s in type into <cuboid> elements, which are
  // added
  // to pElem, and these <component>'s are deleted after loop

  std::set<Element *> allComponentInType;   // used to hold <component>'s found
  std::vector<std::string> allLocationName; // used to check if loc names unique
  for (unsigned long i = 0; i < numLocation; i++) {
    Element *pLoc = static_cast<Element *>(pNL->item(i));

    // The location element is required to be a child of a component element.
    // Get this component element
    Element *pCompElem = InstrumentDefinitionParser::getParentComponent(pLoc);

    // get the name given to the <location> element in focus
    // note these names are required to be unique for the purpose of
    // constructing the <algebra>
    std::string locationElementName = pLoc->getAttribute("name");
    if (std::find(allLocationName.begin(), allLocationName.end(),
                  locationElementName) == allLocationName.end())
      allLocationName.push_back(locationElementName);
    else
      throw Exception::InstrumentDefinitionError(
          std::string("Names in a <type> element containing ") +
          "a <combine-components-into-one-shape> element must be unique. " +
          "Here error is that " + locationElementName +
          " appears at least twice. See www.mantidproject.org/IDF.");

    // create dummy component to hold coord. sys. of cuboid
    CompAssembly *baseCoor = new CompAssembly(
        "base"); // dummy assembly used to get to end assembly if nested
    ICompAssembly *endComponent =
        0; // end assembly, its purpose is to hold the shape coordinate system
    // get shape coordinate system, returned as endComponent, as defined by pLoc
    // and nested <location> elements
    // of pLoc
    std::string shapeTypeName =
        getShapeCoorSysComp(baseCoor, pLoc, getTypeElement, endComponent);

    // translate and rotate cuboid according to shape coordinate system in
    // endComponent
    std::string cuboidStr = translateRotateXMLcuboid(
        endComponent, getTypeElement[shapeTypeName], locationElementName);

    delete baseCoor;

    // if <translate-rotate-combined-shape-to> is specified
    if (pTransRot) {
      baseCoor = new CompAssembly("base");

      setLocation(baseCoor, pTransRot, m_angleConvertConst);

      // Translate and rotate shape xml string according to
      // <translate-rotate-combined-shape-to>
      cuboidStr =
          translateRotateXMLcuboid(baseCoor, cuboidStr, locationElementName);

      delete baseCoor;
    }

    DOMParser pParser;
    Poco::AutoPtr<Document> pDoc;
    try {
      pDoc = pParser.parseString(cuboidStr);
    } catch (...) {
      throw Exception::InstrumentDefinitionError(
          std::string("Unable to parse XML string ") + cuboidStr);
    }
    // Get pointer to root element and add this element to pElem
    Element *pCuboid = pDoc->documentElement();
    Poco::AutoPtr<Node> fisse =
        (pElem->ownerDocument())->importNode(pCuboid, true);
    pElem->appendChild(fisse);

    allComponentInType.insert(pCompElem);
  }

  // delete all <component> found in pElem
  std::set<Element *>::iterator it;
  for (it = allComponentInType.begin(); it != allComponentInType.end(); ++it)
    pElem->removeChild(*it);
}

/// return absolute position of point which is set relative to the
/// coordinate system of the input component
/// @param comp Reference coordinate system
/// @param pos A position relative to the coord. sys. of comp
/// @return absolute position
V3D
InstrumentDefinitionParser::getAbsolutPositionInCompCoorSys(ICompAssembly *comp,
                                                            V3D pos) {
  Component *dummyComp = new Component("dummy", comp);
  comp->add(dummyComp);

  dummyComp->setPos(pos); // set pos relative to comp coord. sys.

  V3D retVal = dummyComp->getPos(); // get absolute position

  return retVal;
}

/// Returns a translated and rotated \<cuboid\> element with "id" attribute
/// equal cuboidName
/// @param comp coordinate system to translate and rotate cuboid to
/// @param cuboidEle Input \<cuboid\> element
/// @param cuboidName What the "id" attribute of the returned \<coboid\> will be
/// set to
/// @return XML string of translated and rotated \<cuboid\>
std::string InstrumentDefinitionParser::translateRotateXMLcuboid(
    ICompAssembly *comp, const Poco::XML::Element *cuboidEle,
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

/// Returns a translated and rotated \<cuboid\> element with "id" attribute
/// equal cuboidName
/// @param comp coordinate system to translate and rotate cuboid to
/// @param cuboidXML Input \<cuboid\> xml string
/// @param cuboidName What the "id" attribute of the returned \<coboid\> will be
/// set to
/// @return XML string of translated and rotated \<cuboid\>
std::string InstrumentDefinitionParser::translateRotateXMLcuboid(
    ICompAssembly *comp, const std::string &cuboidXML,
    const std::string &cuboidName) {
  DOMParser pParser;
  Poco::AutoPtr<Document> pDoc;
  try {
    pDoc = pParser.parseString(cuboidXML);
  } catch (...) {
    throw Exception::InstrumentDefinitionError(
        std::string("Unable to parse XML string ") + cuboidXML);
  }

  Element *pCuboid = pDoc->documentElement();

  std::string retVal = translateRotateXMLcuboid(comp, pCuboid, cuboidName);

  return retVal;
}

/// Take as input a \<locations\> element. Such an element is a short-hand
/// notation for a sequence of \<location\> elements.
/// This method return this sequence as a xml string
/// @param pElem Input \<locations\> element
/// @return XML string
/// @throw InstrumentDefinitionError Thrown if issues with the content of XML
/// instrument file
std::string InstrumentDefinitionParser::convertLocationsElement(
    const Poco::XML::Element *pElem) {
  // Number of <location> this <locations> element is shorthand for
  size_t nElements(0);
  if (pElem->hasAttribute("n-elements")) {
    int n = boost::lexical_cast<int>(
        Strings::strip(pElem->getAttribute("n-elements")));

    if (n <= 0) {
      throw Exception::InstrumentDefinitionError("n-elements must be positive");
    } else {
      nElements = static_cast<size_t>(n);
    }
  } else {
    throw Exception::InstrumentDefinitionError(
        "When using <locations> n-elements attribute is required. See "
        "www.mantidproject.org/IDF.");
  }

  std::string name("");
  if (pElem->hasAttribute("name")) {
    name = pElem->getAttribute("name");
  }

  int nameCountStart(0);
  if (pElem->hasAttribute("name-count-start")) {
    nameCountStart = boost::lexical_cast<int>(
        Strings::strip(pElem->getAttribute("name-count-start")));
  }

  // A list of numeric attributes which are allowed to have corresponding -end
  std::set<std::string> rangeAttrs =
      boost::assign::list_of("x")("y")("z")("r")("t")("p")("rot");

  // Numeric attributes related to rotation. Doesn't make sense to have -end for
  // those
  std::set<std::string> rotAttrs =
      boost::assign::list_of("axis-x")("axis-y")("axis-z");

  // A set of all numeric attributes for convenience
  std::set<std::string> allAttrs;
  allAttrs.insert(rangeAttrs.begin(), rangeAttrs.end());
  allAttrs.insert(rotAttrs.begin(), rotAttrs.end());

  // Attribute values as read from <locations>. If the attribute doesn't have a
  // value here, it
  // means that it wasn't set
  std::map<std::string, double> attrValues;

  // Read all the set attribute values
  for (auto it = allAttrs.begin(); it != allAttrs.end(); ++it) {
    if (pElem->hasAttribute(*it)) {
      attrValues[*it] =
          boost::lexical_cast<double>(Strings::strip(pElem->getAttribute(*it)));
    }
  }

  // Range attribute steps
  std::map<std::string, double> rangeAttrSteps;

  // Find *-end for range attributes and calculate steps
  for (auto it = rangeAttrs.begin(); it != rangeAttrs.end(); ++it) {
    std::string endAttr = *it + "-end";
    if (pElem->hasAttribute(endAttr)) {
      if (attrValues.find(*it) == attrValues.end()) {
        throw Exception::InstrumentDefinitionError(
            "*-end attribute without corresponding * attribute.");
      }

      double from = attrValues[*it];
      double to = boost::lexical_cast<double>(
          Strings::strip(pElem->getAttribute(endAttr)));

      rangeAttrSteps[*it] = (to - from) / (static_cast<double>(nElements) - 1);
    }
  }

  std::ostringstream xml;

  Poco::XML::XMLWriter writer(xml, Poco::XML::XMLWriter::CANONICAL);
  writer.startDocument();
  writer.startElement("", "", "expansion-of-locations-element");

  for (size_t i = 0; i < nElements; ++i) {
    Poco::XML::AttributesImpl attr;

    if (!name.empty()) {
      // Add name with appropriate numeric postfix
      attr.addAttribute(
          "", "", "name", "",
          name + boost::lexical_cast<std::string>(nameCountStart + i));
    }

    // Copy values of all the attributes set
    for (auto it = attrValues.begin(); it != attrValues.end(); ++it) {
      attr.addAttribute("", "", it->first, "",
                        boost::lexical_cast<std::string>(it->second));

      // If attribute has a step, increase the value by the step
      if (rangeAttrSteps.find(it->first) != rangeAttrSteps.end()) {
        it->second += rangeAttrSteps[it->first];
      }
    }

    writer.emptyElement("", "", "location", attr);
  }

  writer.endElement("", "", "expansion-of-locations-element");
  writer.endDocument();

  return xml.str();
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
Poco::XML::Element *
InstrumentDefinitionParser::getShapeElement(const Poco::XML::Element *pElem,
                                            const std::string &name) {
  // check if this shape element contain an element with name specified by the
  // 2nd function argument
  Poco::AutoPtr<NodeList> pNL = pElem->getElementsByTagName(name);
  if (pNL->length() != 1) {
    throw std::invalid_argument(
        "XML element: <" + pElem->tagName() +
        "> must contain exactly one sub-element with name: <" + name + ">.");
  }
  Element *retVal = static_cast<Element *>(pNL->item(0));
  return retVal;
}

/** Get position coordinates from XML element
 *
 *  @param pElem :: XML element whose attributes contain position coordinates
 *  @return Position coordinates in the form of a V3D object
 */
V3D InstrumentDefinitionParser::parsePosition(Poco::XML::Element *pElem) {
  V3D retVal;

  if (pElem->hasAttribute("R") || pElem->hasAttribute("theta") ||
      pElem->hasAttribute("phi")) {
    double R = 0.0, theta = 0.0, phi = 0.0;

    if (pElem->hasAttribute("R"))
      R = atof((pElem->getAttribute("R")).c_str());
    if (pElem->hasAttribute("theta"))
      theta = atof((pElem->getAttribute("theta")).c_str());
    if (pElem->hasAttribute("phi"))
      phi = atof((pElem->getAttribute("phi")).c_str());

    retVal.spherical(R, theta, phi);
  } else if (pElem->hasAttribute("r") || pElem->hasAttribute("t") ||
             pElem->hasAttribute("p"))
  // This is alternative way a user may specify spherical coordinates
  // which may be preferred in the long run to the more verbose of
  // using R, theta and phi.
  {
    double R = 0.0, theta = 0.0, phi = 0.0;

    if (pElem->hasAttribute("r"))
      R = atof((pElem->getAttribute("r")).c_str());
    if (pElem->hasAttribute("t"))
      theta = atof((pElem->getAttribute("t")).c_str());
    if (pElem->hasAttribute("p"))
      phi = atof((pElem->getAttribute("p")).c_str());

    retVal.spherical(R, theta, phi);
  } else {
    double x = 0.0, y = 0.0, z = 0.0;

    if (pElem->hasAttribute("x"))
      x = atof((pElem->getAttribute("x")).c_str());
    if (pElem->hasAttribute("y"))
      y = atof((pElem->getAttribute("y")).c_str());
    if (pElem->hasAttribute("z"))
      z = atof((pElem->getAttribute("z")).c_str());

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
std::string InstrumentDefinitionParser::getShapeCoorSysComp(
    Geometry::ICompAssembly *parent, Poco::XML::Element *pLocElem,
    std::map<std::string, Poco::XML::Element *> &getTypeElement,
    Geometry::ICompAssembly *&endAssembly) {
  // The location element is required to be a child of a component element. Get
  // this component element
  Element *pCompElem = InstrumentDefinitionParser::getParentComponent(pLocElem);

  // Create the assembly that will be appended into the parent.
  Geometry::ICompAssembly *ass;

  // The newly added component is required to have a type. Find out what this
  // type is and find all the location elements of this type. Finally loop over
  // these
  // location elements

  Element *pType = getTypeElement[pCompElem->getAttribute("type")];

  ass = new Geometry::CompAssembly(
      InstrumentDefinitionParser::getNameOfLocationElement(pLocElem, pCompElem),
      parent);
  endAssembly = ass;

  // set location for this newly added comp
  setLocation(ass, pLocElem, m_angleConvertConst);

  Poco::AutoPtr<NodeList> pNL = pType->getElementsByTagName("location");
  if (pNL->length() == 0) {
    return pType->getAttribute("name");
  } else if (pNL->length() == 1) {
    Element *pElem = static_cast<Element *>(pNL->item(0));
    return getShapeCoorSysComp(ass, pElem, getTypeElement, endAssembly);
  } else {
    throw Exception::InstrumentDefinitionError(
        std::string("When using <combine-components-into-one-shape> ") +
        " the containing component elements are not allowed to contain "
        "multiple nested components. See www.mantidproject.org/IDF.");
  }
}

} // namespace Mantid
} // namespace Geometry
