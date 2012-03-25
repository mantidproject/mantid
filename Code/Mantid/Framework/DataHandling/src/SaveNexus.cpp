/*WIKI* 


The algorithm SaveNexus will write a Nexus data file from the named workspace.
The file name can be an absolute or relative path and should have the extension
.nxs, .nx5 or .xml.
Warning - using XML format can be extremely slow for large data sets and generate very large files.
Both the extensions nxs and nx5 will generate HDF5 files.

The optional parameters can be used to control which spectra are saved into the file (not yet implemented).
If spectrum_min and spectrum_max are given, then only that range to data will be loaded.

A Mantid Nexus file may contain several workspace entries each labelled with an integer starting at 1.
If the file already contains n workspaces, the new one will be labelled n+1.

In the future it may be possible to write other Nexus file types than the one supported by SaveNexusProcessed.


===Time series data===
TimeSeriesProperty data within the workspace will be saved as NXlog sections in the Nexus file.
Only floating point logs are stored and loaded at present.

===Subalgorithms used===

[[SaveNexusProcessed]]






*WIKI*/
// SaveNeXus
// @author Freddie Akeroyd, STFC ISIS Faility
// @author Ronald Fowler, STFC eScience. Modified to fit with SaveNexusProcessed
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/SaveNexus.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/BoundedValidator.h"

#include <cmath>
#include <boost/shared_ptr.hpp>
#include <Poco/File.h>

namespace Mantid
{
namespace DataHandling
{

// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(SaveNexus)

/// Sets documentation strings for this algorithm
void SaveNexus::initDocs()
{
  this->setWikiSummary("The SaveNexus algorithm will write the given Mantid workspace to a NeXus file. SaveNexus currently just invokes [[SaveNexusProcessed]]. ");
  this->setOptionalMessage("The SaveNexus algorithm will write the given Mantid workspace to a NeXus file. SaveNexus currently just invokes SaveNexusProcessed.");
}


using namespace Kernel;
using namespace API;
using namespace DataObjects;

/// Empty default constructor
SaveNexus::SaveNexus() : Algorithm() {}

/** Initialisation method.
 *
 */
void SaveNexus::init()
{
  // Declare required parameters, filename with ext {.nx,.nx5,xml} and input workspac
  std::vector<std::string> exts;
  exts.push_back(".nxs");
  exts.push_back(".nx5");
  exts.push_back(".xml");
  declareProperty(new FileProperty("Filename", "", FileProperty::Save, exts),
		    "The name of the Nexus file to write, as a full or relative\n"
		    "path");
  declareProperty(new WorkspaceProperty<Workspace> ("InputWorkspace", "", Direction::Input),
      "Name of the workspace to be saved");
  //
  // Declare optional input parameters
  // These are:
  // Title       - string to describe data
  // EntryNumber - integer >0 to be used in entry name "mantid_workspace_<n>"
  //                          Within a file the entries will be sequential from 1.
  //                          This option should allow overwrite of existing entry,
  //                          *not* addition of out-of-sequence entry numbers.
  // spectrum_min, spectrum_max - range of "spectra" numbers to write
  // spectrum_list            list of spectra values to write
  //
  declareProperty("Title", "", boost::make_shared<NullValidator>() ,
      "A title to describe the saved workspace");
  
  auto mustBePositive = boost::make_shared<BoundedValidator<int> >();
  mustBePositive->setLower(0);
  // declareProperty("EntryNumber", Mantid::EMPTY_INT(), mustBePositive,
  //  "(Not implemented yet) The index number of the workspace within the Nexus file\n"
  // "(default leave unchanged)" );
  declareProperty("WorkspaceIndexMin", 0, mustBePositive,
      "Number of first WorkspaceIndex to read, only for single period data.\n"
        "Not yet implemented");
  declareProperty("WorkspaceIndexMax", Mantid::EMPTY_INT(), mustBePositive,
      "Number of last WorkspaceIndex to read, only for single period data.\n"
        "Not yet implemented.");
  declareProperty(new ArrayProperty<int> ("WorkspaceIndexList"),
      "List of WorkspaceIndex numbers to read, only for single period data.\n"
        "Not yet implemented");
  declareProperty("Append", false, "Determines whether .nxs file needs to be\n"
    "over written or appended");
  // option which might be required in future - should be a choice e.g. MantidProcessed/Muon1
  // declareProperty("Filetype","",new NullValidator<std::string>);
}

/** Execute the algorithm. Currently just calls SaveNexusProcessed but could
 *  call write other formats if support added
 *
 *  @throw runtime_error Thrown if algorithm cannot execute
 */
void SaveNexus::exec()
{
  // Retrieve the filename from the properties
  m_filename = getPropertyValue("FileName");
  m_inputWorkspace = getPropertyValue("InputWorkspace");
  //retrieve the append property
  bool bAppend = getProperty("Append");
  // if bAppend is default (false) overwrite (delete )the .nxs file 
  if (!bAppend)
  {
	  Poco::File file(m_filename);
	  if (file.exists())
	  { file.remove();
	  }
  }

  m_filetype = "NexusProcessed";

  if (m_filetype == "NexusProcessed")
  {
    runSaveNexusProcessed();
  }
  else
  {
    throw Exception::NotImplementedError("SaveNexus passed invalid filetype.");
  }

  return;
}
/** virtual method to set the non workspace properties for this algorithm
 *  @param alg :: pointer to the algorithm
 *  @param propertyName :: name of the property
 *  @param propertyValue :: value  of the property
 *  @param perioidNum :: period number
 */
void SaveNexus::setOtherProperties(IAlgorithm* alg,const std::string& propertyName,const std::string& propertyValue,int perioidNum)
{	
	if(!propertyName.compare("Append"))
	{	if(perioidNum!=1)
		{ alg->setPropertyValue(propertyName,"1");
		}
		else alg->setPropertyValue(propertyName,propertyValue);
	}
	else
		Algorithm::setOtherProperties(alg,propertyName,propertyValue,perioidNum);
 }
void SaveNexus::runSaveNexusProcessed()
{
  IAlgorithm_sptr saveNexusPro = createSubAlgorithm("SaveNexusProcessed", 0.0, 1.0, true);
  // Pass through the same output filename
  saveNexusPro->setPropertyValue("Filename", m_filename);
  // Set the workspace property
  std::string inputWorkspace = "inputWorkspace";
  saveNexusPro->setPropertyValue(inputWorkspace, m_inputWorkspace);
  //
  std::vector<int> specList = getProperty("WorkspaceIndexList");
  if (!specList.empty())
    saveNexusPro->setPropertyValue("WorkspaceIndexList", getPropertyValue("WorkspaceIndexList"));
  //
  int specMax = getProperty("WorkspaceIndexMax");
  if (specMax != Mantid::EMPTY_INT())
  {
    saveNexusPro->setPropertyValue("WorkspaceIndexMax", getPropertyValue("WorkspaceIndexMax"));
    saveNexusPro->setPropertyValue("WorkspaceIndexMin", getPropertyValue("WorkspaceIndexMin"));
  }
  std::string title = getProperty("Title");
  if (!title.empty())
    saveNexusPro->setPropertyValue("Title", getPropertyValue("Title"));
 
  // Pass through the append property
  saveNexusPro->setProperty<bool>("Append",getProperty("Append"));

  // Now execute the sub-algorithm. Catch and log any error, but don't stop.
  try
  {
    saveNexusPro->execute();
  } catch (std::runtime_error&)
  {
    g_log.error("Unable to successfully run SaveNexusprocessed sub-algorithm");
  }
  if (!saveNexusPro->isExecuted())
    g_log.error("Unable to successfully run SaveNexusProcessed sub-algorithm");
  //
  progress(1);
}
} // namespace DataHandling
} // namespace Mantid
