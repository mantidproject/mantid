/*WIKI* 

This algorithm allows instrument parameters to be specified in a separate file from the [[InstrumentDefinitionFile|IDF]]. The required format for this file is identical to that used for defining parameters through <component-link>s in an IDF. Below is an example of how to define a parameter named 'test' to be associated with a component named 'bank_90degnew' defined in the IDF of the HRPD instrument:
<div style="border:1pt dashed black; background:#f9f9f9;padding: 1em 0;">
<source lang="xml">
<?xml version="1.0" encoding="UTF-8" ?>
<parameter-file instrument="HRPD" valid-from="YYYY-MM-DD HH:MM:SS">

<component-link name="bank_90degnew" >
  <parameter name="test"> <value val="50.0" /> </parameter>
</component-link>

</parameter-file>
</source></div>


*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadParameterFile.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidGeometry/Instrument.h"
#include "MantidAPI/InstrumentDataService.h"
#include "MantidGeometry/Instrument/XMLlogfile.h"
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


namespace Mantid
{
namespace DataHandling
{

DECLARE_ALGORITHM(LoadParameterFile)

/// Sets documentation strings for this algorithm
void LoadParameterFile::initDocs()
{
  this->setWikiSummary("Loads instrument parameters into a [[workspace]]. where these parameters are associated component names as defined in Instrument Definition File ([[InstrumentDefinitionFile|IDF]]) or a string consisting of the contents of such.."); 
  this->setOptionalMessage("Loads instrument parameters into a workspace. where these parameters are associated component names as defined in Instrument Definition File (IDF) or a string consisting of the contents of such.");
}


using namespace Kernel;
using namespace API;
using Geometry::Instrument;
using Geometry::Instrument_sptr;

/// Empty default constructor
LoadParameterFile::LoadParameterFile() : Algorithm()
{}

/// Initialisation method.
void LoadParameterFile::init()
{
  // When used as a Child Algorithm the workspace name is not used - hence the "Anonymous" to satisfy the validator
  declareProperty(
    new WorkspaceProperty<MatrixWorkspace>("Workspace","Anonymous",Direction::InOut),
    "The name of the workspace to load the instrument parameters into." );
  declareProperty(new FileProperty("Filename","", FileProperty::OptionalLoad, ".xml"),
                  "The filename (including its full or relative path) of a parameter definition file. The file extension must either be .xml or .XML.");
  declareProperty("ParameterXML","","The parameter definition XML as a string.");
}

/** Executes the algorithm. Reading in the file and creating and populating
 *  the output workspace
 *
 *  @throw FileError Thrown if unable to parse XML file
 *  @throw InstrumentDefinitionError Thrown if issues with the content of XML instrument file
 */
void LoadParameterFile::exec()
{
  // Retrieve the filename from the properties
  std::string filename = getPropertyValue("Filename");

  // Retrieve the parameter XML string from the properties
  std::string parameterXML = getPropertyValue("ParameterXML");

  // Get the input workspace
  const MatrixWorkspace_sptr localWorkspace = getProperty("Workspace");

  execManually(false, filename, parameterXML, localWorkspace);
}

void LoadParameterFile::execManually(bool useString, std::string filename, std::string parameterXML,  Mantid::API::ExperimentInfo_sptr localWorkspace)
{
  // TODO: Refactor to remove the need for the const cast
  Instrument_sptr instrument = boost::const_pointer_cast<Instrument>(localWorkspace->getInstrument()->baseInstrument());

  // Set up the DOM parser and parse xml file
  DOMParser pParser;
  AutoPtr<Document> pDoc;
  try
  {
    pDoc = pParser.parse(filename);
  }
  catch(Poco::Exception& exc)
  {
    throw Kernel::Exception::FileError(exc.displayText() + ". Unable to parse File:", filename);
  }
  catch(...)
  {
    throw Kernel::Exception::FileError("Unable to parse File:" , filename);
  }

  // Get pointer to root element
  AutoPtr<Element> pRootElem = pDoc->documentElement();
  if ( !pRootElem->hasChildNodes() )
  {
    throw Kernel::Exception::InstrumentDefinitionError("No root element in XML Parameter file", filename);
  }

  // 
  InstrumentDefinitionParser loadInstr;
  loadInstr.setComponentLinks(instrument, pRootElem);

  // populate parameter map of workspace 
  localWorkspace->populateInstrumentParameters();

}

} // namespace DataHandling
} // namespace Mantid
