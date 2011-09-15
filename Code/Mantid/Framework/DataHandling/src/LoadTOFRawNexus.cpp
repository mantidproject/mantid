// Includes

#include "MantidDataHandling/LoadTOFRawNexus.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidDataHandling/LoadEventNexus.h"
#include "MantidNexusCPP/NeXusFile.hpp"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/cow_ptr.h"
#include <boost/algorithm/string/detail/classification.hpp>
#include <boost/algorithm/string/split.hpp>

namespace Mantid
{
namespace DataHandling
{
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM( LoadTOFRawNexus )

using namespace Kernel;
using namespace API;
using namespace DataObjects;

LoadTOFRawNexus::LoadTOFRawNexus()
{
}


//-------------------------------------------------------------------------------------------------
/// Initialisation method.
void LoadTOFRawNexus::init()
{

  std::vector < std::string > exts;
  exts.push_back(".nxs");
  declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
      "The name of the NeXus file to load");
  declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace", "", Direction::Output));
  declareProperty("Signal", 1,
      "Number of the signal to load from the file. Default is 1 = time_of_flight.\n"
      "Some NXS files have multiple data fields giving binning in other units (e.g. d-spacing or momentum).\n"
      "Enter the right signal number for your desired field.");
}



//-------------------------------------------------------------------------------------------------
/** Goes thoguh a histogram NXS file and counts the number of pixels.
 * It also determines the name of the data field and axis to load
 *
 * @param nexusfilename :: nxs file path
 * @param entry_name :: name of the entry
 * @param numPixels :: returns # of pixels
 * @param numBins :: returns # of bins (length of Y vector, add one to get the number of X points)
 * @param bankNames :: returns the list of bank names
 */
void LoadTOFRawNexus::countPixels(const std::string &nexusfilename, const std::string & entry_name,
     std::vector<std::string> & bankNames)
{
  numPixels = 0;
  numBins = 0;
  m_dataField = "";
  m_axisField = "";
  bankNames.clear();

  // Create the root Nexus class
  ::NeXus::File * file = new ::NeXus::File(nexusfilename);

  // Open the default data group 'entry'
  file->openGroup(entry_name, "NXentry");

  // Look for all the banks
  std::map<std::string, std::string> entries = file->getEntries();
  std::map<std::string, std::string>::iterator it;
  for (it = entries.begin(); it != entries.end(); it++)
  {
    std::string name = it->first;
    if (name.size() > 4)
    {
      if (name.substr(0, 4) == "bank")
      {
        bankNames.push_back(name);

        // OK, this is some bank data
        file->openGroup(name, it->second);

        // -------------- Find the data field name ----------------------------
        if (m_dataField.empty())
        {
          std::map<std::string, std::string> entries = file->getEntries();
          std::map<std::string, std::string>::iterator it;
          for (it = entries.begin(); it != entries.end(); ++it)
          {
            file->openData(it->first);
            if (file->hasAttr("signal"))
            {
              int signal = 0;
              file->getAttr("signal", signal);
              if (signal == m_signal)
              {
                // That's the right signal!
                m_dataField = it->first;
                // Find the corresponding X axis
                if (!file->hasAttr("axes"))
                  throw std::runtime_error("Your chosen signal number, " + Strings::toString(m_signal) + ", corresponds to the data field '" +
                      m_dataField + "' has no 'axes' attribute specifying.");

                std::string axes;
                file->getAttr("axes", axes);
                std::vector<std::string> allAxes;
                boost::split( allAxes, axes, boost::algorithm::detail::is_any_ofF<char>(","));
                if (allAxes.size() != 3)
                  throw std::runtime_error("Your chosen signal number, " + Strings::toString(m_signal) + ", corresponds to the data field '" +
                      m_dataField + "' which has only " + Strings::toString(allAxes.size()) + " dimension. Expected 3 dimensions.");

                m_axisField = allAxes.back();
                g_log.information() << "Loading signal " << m_signal << ", " << m_dataField << " with axis " << m_axisField << std::endl;
                file->closeData();
                break;
              }
            }
            file->closeData();
          }
        }

        // Count how many pixels in the bank
        file->openData("pixel_id");
        std::vector<int> dims = file->getInfo().dims;
        file->closeData();

        size_t newPixels = 1;
        if (dims.size() > 0)
        {
          for (size_t i=0; i < dims.size(); i++)
            newPixels *= dims[i];
          numPixels += newPixels;
        }

        // Get the size of the X vector
        file->openData(m_axisField);
        dims = file->getInfo().dims;
        // Find the units, if available
        if (file->hasAttr("units"))
          file->getAttr("units", m_xUnits);
        else
          m_xUnits = "microsecond"; //use default
        file->closeData();
        if (dims.size() > 0)
          numBins = dims[0] - 1;

        file->closeGroup();
      }
    }
  }
  file->close();

  delete file;
}


/** Load a single bank into the workspace
 *
 * @param nexusfilename :: file to open
 * @param entry_name :: NXentry name
 * @param bankName :: NXdata bank name
 * @param WS :: workspace to modify
 * @param workspaceIndex :: workspaceIndex of the first spectrum. This will be incremented by the # of pixels in the bank.
 */
void LoadTOFRawNexus::loadBank(const std::string &nexusfilename, const std::string & entry_name,
    const std::string &bankName, Mantid::API::MatrixWorkspace_sptr WS)
{
  // Navigate to the point in the file
  ::NeXus::File * file = new ::NeXus::File(nexusfilename);
  file->openGroup(entry_name, "NXentry");
  file->openGroup("instrument", "NXinstrument");
  file->openGroup(bankName, "NXdetector");

  // Load the pixel IDs
  std::vector<uint32_t> pixel_id;
  file->readData("pixel_id", pixel_id);
  size_t numPixels = pixel_id.size();
  if (numPixels == 0)
  { file->close(); g_log.warning() << "Invalid pixel_id data in " << bankName << std::endl; return; }

  // Load the TOF vector
  std::vector<float> tof;
  file->readData(m_axisField, tof);
  size_t numBins = tof.size() - 1;
  if (tof.size() <= 1)
  { file->close(); g_log.warning() << "Invalid " << m_axisField << " data in " << bankName << std::endl; return; }

  // Make a shared pointer
  MantidVecPtr Xptr;
  MantidVec & X = Xptr.access();
  X.resize( tof.size(), 0);
  X.assign( tof.begin(), tof.end() );

  // Load the data. Coerce ints into double.
  std::string errorsField = "";
  std::vector<double> data;
  file->openData(m_dataField);
  file->getDataCoerce(data);
  if (file->hasAttr("errors"))
      file->getAttr("errors", errorsField);
  file->closeData();

  // Load the errors
  bool hasErrors = !errorsField.empty();
  std::vector<double> errors;
  if (hasErrors)
  {
    try
    {
      file->openData(errorsField);
      file->getDataCoerce(errors);
      file->closeData();
    }
    catch (...)
    {
      g_log.information() << "Error loading the errors field, '" << errorsField << "' for bank " << bankName << ". Will use sqrt(counts). " <<  std::endl;
      hasErrors = false;
    }
  }

  if (data.size() != numBins * numPixels)
  { file->close(); g_log.warning() << "Invalid size of '" << m_dataField << "' data in " << bankName << std::endl; return; }
  if (hasErrors && (errors.size() != numBins * numPixels))
  { file->close(); g_log.warning() << "Invalid size of '" << errorsField << "' errors in " << bankName << std::endl; return; }

  for (size_t i=0; i<numPixels; i++)
  {
    // Find the workspace index for this detector
    detid_t pixelID = pixel_id[i];
    size_t wi = (*id_to_wi)[pixelID];

    // Set the basic info of that spectrum
    ISpectrum * spec = WS->getSpectrum(wi);
    spec->setSpectrumNo( specid_t(wi+1) );
    spec->setDetectorID( pixel_id[i] );
    // Set the shared X pointer
    spec->setX(X);

    // Extract the Y
    MantidVec & Y = spec->dataY();
    Y.assign( data.begin() + i * numBins,  data.begin() + (i+1) * numBins );

    MantidVec & E = spec->dataE();

    if (hasErrors)
    {
      // Copy the errors from the loaded document
      E.assign( errors.begin() + i * numBins,  errors.begin() + (i+1) * numBins );
    }
    else
    {
      // Now take the sqrt(Y) to give E
      E = Y;
      std::transform(E.begin(), E.end(), E.begin(), (double(*)(double)) sqrt);
    }
  }

  // Done!
  file->close();
}


//-------------------------------------------------------------------------------------------------
/** @return the name of the entry that we will load */
std::string LoadTOFRawNexus::getEntryName(const std::string & filename)
{
  std::string entry_name = "entry";
  ::NeXus::File * file = new ::NeXus::File(filename);
  std::map<std::string,std::string> entries = file->getEntries();
  file->close();
  delete file;

  if (entries.size() == 0)
    throw std::runtime_error("No entries in the NXS file!");

  // name "entry" is normal, but "entry-state0" is the name of the real state for live nexus files.
  if (entries.find(entry_name) == entries.end())
    entry_name = "entry-state0";
  // If that doesn't exist, just take the first entry.
  if (entries.find(entry_name) == entries.end())
    entry_name = entries.begin()->first;
//  // Tell the user
//  if (entries.size() > 1)
//    g_log.notice() << "There are " << entries.size() << " NXentry's in the file. Loading entry '" << entry_name << "' only." << std::endl;

  return entry_name;
}

//-------------------------------------------------------------------------------------------------
/** Executes the algorithm. Reading in the file and creating and populating
 *  the output workspace
 *
 *  @throw Exception::FileError If the Nexus file cannot be found/opened
 *  @throw std::invalid_argument If the optional properties are set to invalid values
 */
void LoadTOFRawNexus::exec()
{
  // The input properties
  std::string filename = getPropertyValue("Filename");
  m_signal = getProperty("Signal");

  // Find the entry name we want.
  std::string entry_name = LoadTOFRawNexus::getEntryName(filename);

  // Count pixels and other setup
  Progress * prog = new Progress(this, 0.0, 1.0, 10);
  prog->doReport("Counting pixels");
  std::vector<std::string> bankNames;
  countPixels(filename, entry_name, bankNames);
  g_log.debug() << "Workspace found to have " << numPixels << " pixels and " << numBins << " bins" << std::endl;

  prog->setNumSteps(bankNames.size() + 5);

  prog->doReport("Creating workspace");
  // Start with a dummy WS just to hold the logs and load the instrument
  MatrixWorkspace_sptr WS = WorkspaceFactory::Instance().create(
            "Workspace2D", numPixels, numBins+1, numBins);

  // Load the logs
  prog->doReport("Loading DAS logs");
  LoadEventNexus::runLoadNexusLogs(filename, WS, pulseTimes, this);

  // Load the instrument
  prog->report("Loading instrument");
  LoadEventNexus::runLoadInstrument(filename, WS, entry_name, this);

  // Load the meta data, but don't stop on errors
  prog->report("Loading metadata");
  try
  { LoadEventNexus::loadEntryMetadata(filename, WS, entry_name);
  }
  catch (std::exception & e)
  { g_log.warning() << "Error while loading meta data. " << e.what() << std::endl; }

  // Set the spectrum number/detector ID at each spectrum. This is consistent with LoadEventNexus for non-ISIS files.
  prog->report("Building Spectra Mapping");
  WS->rebuildSpectraMapping(false);
  // And map ID to WI
  id_to_wi = WS->getDetectorIDToWorkspaceIndexMap(false);

  // Load each bank sequentially
  PARALLEL_FOR1(WS)
  for (int i=0; i<int(bankNames.size()); i++)
  {
    PARALLEL_START_INTERUPT_REGION
    std::string bankName = bankNames[i];
    prog->report("Loading bank " + bankName);
    loadBank(filename, entry_name, bankName, WS);
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  // Set some units
  if (m_xUnits == "Ang")
    WS->getAxis(0)->setUnit("dSpacing");
  else if(m_xUnits == "invAng")
    WS->getAxis(0)->setUnit("MomentumTransfer");
  else
    // Default to TOF for any other string
    WS->getAxis(0)->setUnit("TOF");
  WS->setYUnit("Counts");

  // Method that will eventually go away.
  WS->generateSpectraMap();

  // Set to the output
  setProperty("OutputWorkspace", WS);

  delete prog;
  delete id_to_wi;
}

} // namespace DataHandling
} // namespace Mantid

