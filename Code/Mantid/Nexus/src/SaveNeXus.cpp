// SaveNeXus
// @author Freddie Akeroyd, STFC ISIS Faility
// @author Ronald Fowler, STFC eScience. Modified to fit with SaveNexusProcessed
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidNexus/SaveNeXus.h"
#include "MantidDataObjects/Workspace1D.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidNexus/NeXusUtils.h"
#include "MantidKernel/ArrayProperty.h"

#include <cmath>
#include <boost/shared_ptr.hpp>

namespace Mantid
{
namespace NeXus
{

  // Register the algorithm into the algorithm factory
  DECLARE_ALGORITHM(SaveNexus)

  using namespace Kernel;
  using namespace API;
  using namespace DataObjects;

  Logger& SaveNexus::g_log = Logger::get("SaveNexus");

  /// Empty default constructor
  SaveNexus::SaveNexus():Algorithm()
  {
  }

  /** Initialisation method.
   *
   */
  void SaveNexus::init()
  {
    // Declare required parameters, filename with ext {.nx,.nx5,xml} and input workspace
    std::vector<std::string> exts;
    exts.push_back("NXS");
    exts.push_back("nxs");
    exts.push_back("nx5");
    exts.push_back("NX5");
    exts.push_back("xml");
    exts.push_back("XML");
    declareProperty("FileName","",new FileValidator(exts,false));  
    declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input));
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
    declareProperty("Title","",new NullValidator<std::string>);
    BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
    mustBePositive->setLower(0);
    declareProperty("EntryNumber",0,mustBePositive);
    declareProperty("spectrum_min",0, mustBePositive->clone());
    declareProperty("spectrum_max",0, mustBePositive->clone());
    declareProperty(new ArrayProperty<int>("spectrum_list"));
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
    m_filetype="NexusProcessed";

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

  void SaveNexus::runSaveNexusProcessed()
  {
      IAlgorithm_sptr saveNexusPro = createSubAlgorithm("SaveNexusProcessed");
      // Pass through the same output filename
      saveNexusPro->setPropertyValue("Filename",m_filename);
      // Set the workspace property
      std::string inputWorkspace="inputWorkspace";
      saveNexusPro->setPropertyValue(inputWorkspace,m_inputWorkspace);
      //
      Property *specList = getProperty("spectrum_list");
      if( !(specList->isDefault()) )
         saveNexusPro->setPropertyValue("spectrum_list",getPropertyValue("spectrum_list"));
      //
      Property *specMax = getProperty("spectrum_max");
      if( !(specMax->isDefault()) )
      {
         saveNexusPro->setPropertyValue("spectrum_max",getPropertyValue("spectrum_max"));
         saveNexusPro->setPropertyValue("spectrum_min",getPropertyValue("spectrum_min"));
      }
      Property *title = getProperty("title");
      if( !(title->isDefault()) )
         saveNexusPro->setPropertyValue("title",getPropertyValue("title"));
      Property *entryNumber = getProperty("EntryNumber");
      if( !(entryNumber->isDefault()) )
         saveNexusPro->setPropertyValue("EntryNumber",getPropertyValue("EntryNumber"));

      // Now execute the sub-algorithm. Catch and log any error, but don't stop.
      try
      {
        saveNexusPro->execute();
      }
      catch (std::runtime_error& err)
      {
        g_log.error("Unable to successfully run SaveNexusprocessed sub-algorithm");
      }
      if ( ! saveNexusPro->isExecuted() ) g_log.error("Unable to successfully run SaveNexusProcessed sub-algorithm");
      //
  }
} // namespace NeXus
} // namespace Mantid
