//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadParameterFile.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidGeometry/Instrument/Instrument.h"
#include "MantidAPI/InstrumentDataService.h"
#include "MantidGeometry/Instrument/XMLlogfile.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/FileProperty.h"

#include "Poco/DOM/DOMParser.h"
#include "Poco/DOM/Document.h"
#include "Poco/DOM/Element.h"
#include "Poco/DOM/NodeList.h"
#include "Poco/DOM/NodeIterator.h"
#include "Poco/DOM/NodeFilter.h"
#include "Poco/File.h"
#include "MantidKernel/ArrayProperty.h"
#include <sstream>

using Poco::XML::DOMParser;
using Poco::XML::Document;
using Poco::XML::Element;
using Poco::XML::Node;
using Poco::XML::NodeList;
using Poco::XML::NodeIterator;
using Poco::XML::NodeFilter;


namespace Mantid
{
namespace DataHandling
{

DECLARE_ALGORITHM(LoadParameterFile)

using namespace Kernel;
using namespace API;
using Geometry::Instrument;

/// Empty default constructor
LoadParameterFile::LoadParameterFile() : Algorithm()
{}

/// Initialisation method.
void LoadParameterFile::init()
{
  // When used as a sub-algorithm the workspace name is not used - hence the "Anonymous" to satisfy the validator
  declareProperty(
    new WorkspaceProperty<MatrixWorkspace>("Workspace","Anonymous",Direction::InOut),
    "The name of the workspace to load the instrument parameters into" );
  declareProperty(new FileProperty("Filename","", FileProperty::Load, ".xml"),
      "The filename (including its full or relative path) of an parameter\n"
      "definition file");
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

  // Get the input workspace
  const MatrixWorkspace_sptr localWorkspace = getProperty("Workspace");

	boost::shared_ptr<Instrument> instrument = localWorkspace->getBaseInstrument();


  // Set up the DOM parser and parse xml file
  DOMParser pParser;
  Document* pDoc;
  try
  {
    pDoc = pParser.parse(filename);
  }
  catch(...)
  {
    g_log.error("Unable to parse file " + filename);
    throw Kernel::Exception::FileError("Unable to parse File:" , filename);
  }
  // Get pointer to root element
  Element* pRootElem = pDoc->documentElement();
  if ( !pRootElem->hasChildNodes() )
  {
    g_log.error("XML file: " + filename + "contains no root element.");
    throw Kernel::Exception::InstrumentDefinitionError("No root element in XML instrument file", filename);
  }

  // 
  LoadInstrument loadInstr;
  loadInstr.setComponentLinks(instrument, pRootElem);

  // populate parameter map of workspace 
  localWorkspace->populateInstrumentParameters();

  pDoc->release();

}

} // namespace DataHandling
} // namespace Mantid
