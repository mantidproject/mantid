//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadParameterFile.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidGeometry/Instrument.h"
#include "MantidAPI/InstrumentDataService.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ArrayProperty.h"

#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/DOM/NodeFilter.h>
#include <Poco/DOM/AutoPtr.h>
#include <Poco/File.h>
#include <sstream>
#include "MantidGeometry/Instrument/InstrumentDefinitionParser.h"

using Poco::XML::DOMParser;
using Poco::XML::Document;
using Poco::XML::Element;
using Poco::XML::Node;
using Poco::XML::NodeList;
using Poco::XML::NodeIterator;
using Poco::XML::NodeFilter;
using Poco::XML::AutoPtr;
using Mantid::Geometry::InstrumentDefinitionParser;

namespace Mantid {
namespace DataHandling {

DECLARE_ALGORITHM(LoadParameterFile)

using namespace Kernel;
using namespace API;
using Geometry::Instrument;
using Geometry::Instrument_sptr;

/// Empty default constructor
LoadParameterFile::LoadParameterFile() : Algorithm() {}

/// Initialisation method.
void LoadParameterFile::init() {
  // When used as a Child Algorithm the workspace name is not used - hence the
  // "Anonymous" to satisfy the validator
  declareProperty(
      new WorkspaceProperty<MatrixWorkspace>("Workspace", "Anonymous",
                                             Direction::InOut),
      "The name of the workspace to load the instrument parameters into.");
  declareProperty(
      new FileProperty("Filename", "", FileProperty::OptionalLoad, ".xml"),
      "The filename (including its full or relative path) of a parameter "
      "definition file. The file extension must either be .xml or .XML.");
  declareProperty("ParameterXML", "",
                  "The parameter definition XML as a string.");
}

/** Executes the algorithm. Reading in the file and creating and populating
 *  the output workspace
 *
 *  @throw FileError Thrown if unable to parse XML file
 *  @throw InstrumentDefinitionError Thrown if issues with the content of XML
 *instrument file
 */
void LoadParameterFile::exec() {
  // Retrieve the filename from the properties
  const std::string filename = getPropertyValue("Filename");

  // Retrieve the parameter XML string from the properties
  const Property *const parameterXMLProperty =
      getProperty("ParameterXML"); // to check whether it is default
  const std::string parameterXML = getPropertyValue("ParameterXML");

  // Check the two properties (at least one must be set)
  if (filename.empty() && parameterXMLProperty->isDefault()) {
    throw Kernel::Exception::FileError("Either the Filename or ParameterXML "
                                       "property of LoadParameterFile most be "
                                       "specified to load an IDF",
                                       filename);
  }

  // Get the input workspace
  const MatrixWorkspace_sptr localWorkspace = getProperty("Workspace");

  // TODO: Refactor to remove the need for the const cast (ticket #8521)
  Instrument_sptr instrument = boost::const_pointer_cast<Instrument>(
      localWorkspace->getInstrument()->baseInstrument());

  // Set up the DOM parser and parse xml file
  DOMParser pParser;
  AutoPtr<Document> pDoc;

  // Progress reporting object
  Progress prog(this, 0.0, 1.0, 100);

  prog.report("Parsing XML");
  // If we've been given an XML string parse that instead
  if (!parameterXMLProperty->isDefault()) {
    try {
      pDoc = pParser.parseString(parameterXML);
    } catch (Poco::Exception &exc) {
      throw Kernel::Exception::FileError(
          exc.displayText() + ". Unable to parse parameter XML string",
          "ParameterXML");
    } catch (...) {
      throw Kernel::Exception::FileError("Unable to parse parameter XML string",
                                         "ParameterXML");
    }
  } else {
    try {
      pDoc = pParser.parse(filename);
    } catch (Poco::Exception &exc) {
      throw Kernel::Exception::FileError(
          exc.displayText() + ". Unable to parse File:", filename);
    } catch (...) {
      throw Kernel::Exception::FileError("Unable to parse File:", filename);
    }
  }

  // Get pointer to root element
  Element *pRootElem = pDoc->documentElement();
  if (!pRootElem->hasChildNodes()) {
    throw Kernel::Exception::InstrumentDefinitionError(
        "No root element in XML Parameter file", filename);
  }

  // Set all parameters that specified in all component-link elements of
  // pRootElem
  InstrumentDefinitionParser loadInstr;
  loadInstr.setComponentLinks(instrument, pRootElem, &prog);

  // populate parameter map of workspace
  localWorkspace->populateInstrumentParameters();
  if (!filename.empty()) {
    localWorkspace->instrumentParameters().addParameterFilename(filename);
  }

  prog.resetNumSteps(1, 0.0, 1.0);
  prog.report("Done");
}

} // namespace DataHandling
} // namespace Mantid
