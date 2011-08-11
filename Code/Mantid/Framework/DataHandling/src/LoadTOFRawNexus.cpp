// Includes

#include "MantidDataHandling/LoadTOFRawNexus.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidDataHandling/LoadEventNexus.h"
#include "MantidNexus/NeXusFile.hpp"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/cow_ptr.h"

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

//  BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
//  mustBePositive->setLower(0);
//  declareProperty("SpectrumMin", 0, mustBePositive);
//  declareProperty("SpectrumMax", EMPTY_INT(), mustBePositive->clone());
//  declareProperty(new Kernel::ArrayProperty<int>("SpectrumList"));
}



//-------------------------------------------------------------------------------------------------
/** Goes thoguh a histogram NXS file and counts the number of pixels
 *
 * @param nexusfilename :: nxs file path
 * @param entry_name :: name of the entry
 * @param numPixels :: returns # of pixels
 * @param numBins :: returns # of bins (length of Y vector, add one to get the number of X points)
 * @param bankNames :: returns the list of bank names
 */
void countPixels(const std::string &nexusfilename, const std::string & entry_name,
    size_t & numPixels, size_t & numBins, std::vector<std::string> & bankNames)
{
  numPixels = 0;
  numBins = 0;
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
        file->openData("time_of_flight");
        dims = file->getInfo().dims;
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
  file->openGroup(bankName, "NXdata");

  // Load the pixel IDs
  std::vector<uint32_t> pixel_id;
  file->readData("pixel_id", pixel_id);
  size_t numPixels = pixel_id.size();
  if (numPixels == 0)
  { file->close(); g_log.warning() << "Invalid pixel_id data in " << bankName << std::endl; return; }

  // Load the TOF vector
  std::vector<float> tof;
  file->readData("time_of_flight", tof);
  size_t numBins = tof.size() - 1;
  if (tof.size() <= 1)
  { file->close(); g_log.warning() << "Invalid time_of_flight data in " << bankName << std::endl; return; }

  // Make a shared pointer
  MantidVecPtr Xptr;
  MantidVec & X = Xptr.access();
  X.resize( tof.size(), 0);
  X.assign( tof.begin(), tof.end() );

  // Load the data
  std::vector<uint32_t> data;
  file->readData("data", data);
  if (data.size() != numBins * numPixels)
  { file->close(); g_log.warning() << "Invalid size of 'data' data in " << bankName << std::endl; return; }

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

    // Now take the sqrt(Y) to give E
    MantidVec & E = spec->dataE();
    E = Y;
    std::transform(E.begin(), E.end(), E.begin(), (double(*)(double)) sqrt);
  }

  // Done!
  file->close();
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
  std::string filename = getPropertyValue("Filename");
  std::string entry_name = "entry";

  Progress * prog = new Progress(this, 0.0, 1.0, 10);

  prog->doReport("Counting pixels");
  size_t numPixels=0; size_t numBins=0;
  std::vector<std::string> bankNames;
  countPixels(filename, entry_name, numPixels, numBins, bankNames);
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

  // Load the meta data
  prog->report("Loading metadata");
  LoadEventNexus::loadEntryMetadata(filename, WS, entry_name);

  // Set the spectrum number/detector ID at each spectrum. This is consistent with LoadEventNexus for non-ISIS files.
  prog->report("Building Spectra Mapping");
  WS->rebuildSpectraMapping(false);
  // And map ID to WI
  id_to_wi = WS->getDetectorIDToWorkspaceIndexMap(false);

  // Load each bank sequentially
  for (size_t i=0; i<bankNames.size(); i++)
  {
    std::string bankName = bankNames[i];
    prog->report("Loading bank " + bankName);
    loadBank(filename, entry_name, bankName, WS);
  }

  // Set some units
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

