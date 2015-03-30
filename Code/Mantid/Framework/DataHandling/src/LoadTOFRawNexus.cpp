#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidDataHandling/LoadEventNexus.h"
#include "MantidDataHandling/LoadTOFRawNexus.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/cow_ptr.h"
#include <nexus/NeXusFile.hpp>
#include <boost/algorithm/string/detail/classification.hpp>
#include <boost/algorithm/string/split.hpp>

namespace Mantid {
namespace DataHandling {

DECLARE_NEXUS_FILELOADER_ALGORITHM(LoadTOFRawNexus)

using namespace Kernel;
using namespace API;
using namespace DataObjects;

LoadTOFRawNexus::LoadTOFRawNexus(): m_numPixels(0), m_signalNo(0), pulseTimes(0),
    m_numBins(0), m_spec_min(0), m_dataField(""), m_axisField(""), m_xUnits(""),
    m_fileMutex(), m_assumeOldFile(false) {
}

//-------------------------------------------------------------------------------------------------
/// Initialisation method.
void LoadTOFRawNexus::init() {

  std::vector<std::string> exts;
  exts.push_back(".nxs");
  declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
                  "The name of the NeXus file to load");
  declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace", "",
                                                         Direction::Output),
                  "The name of the Workspace2D to create.");
  declareProperty("Signal", 1,
                  "Number of the signal to load from the file. Default is 1 = "
                  "time_of_flight.\n"
                  "Some NXS files have multiple data fields giving binning in "
                  "other units (e.g. d-spacing or momentum).\n"
                  "Enter the right signal number for your desired field.");
  auto mustBePositive = boost::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(1);
  declareProperty(
      new PropertyWithValue<specid_t>("SpectrumMin", 1, mustBePositive),
      "The index number of the first spectrum to read.  Only used if\n"
      "spectrum_max is set.");
  declareProperty(
      new PropertyWithValue<specid_t>("SpectrumMax", Mantid::EMPTY_INT(),
                                      mustBePositive),
      "The number of the last spectrum to read. Only used if explicitly\n"
      "set.");
}

//-------------------------------------------------------------------------------------------------
/**
 * Return the confidence with with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadTOFRawNexus::confidence(Kernel::NexusDescriptor &descriptor) const {
  int confidence(0);
  if (descriptor.pathOfTypeExists("/entry", "NXentry") ||
      descriptor.pathOfTypeExists("/entry-state0", "NXentry") ||
      descriptor.pathOfTypeExists("/raw_data_1", "NXentry")) {
    const bool hasEventData = descriptor.classTypeExists("NXevent_data");
    const bool hasData = descriptor.classTypeExists("NXdata");
    if (hasData && hasEventData)
      // Event data = this is event NXS
      confidence = 20;
    else if (hasData && !hasEventData)
      // No event data = this is the one
      confidence = 80;
    else
      // No data ?
      confidence = 10;
  }
  return confidence;
}

//-------------------------------------------------------------------------------------------------
/** Goes thoguh a histogram NXS file and counts the number of pixels.
 * It also determines the name of the data field and axis to load
 *
 * @param nexusfilename :: nxs file path
 * @param entry_name :: name of the entry
 * @param bankNames :: returns the list of bank names
 */
void LoadTOFRawNexus::countPixels(const std::string &nexusfilename,
                                  const std::string &entry_name,
                                  std::vector<std::string> &bankNames) {
  m_numPixels = 0;
  m_numBins = 0;
  m_dataField = "";
  m_axisField = "";
  bankNames.clear();

  // Create the root Nexus class
  ::NeXus::File *file = new ::NeXus::File(nexusfilename);

  // Open the default data group 'entry'
  file->openGroup(entry_name, "NXentry");
  // Also pop into the instrument
  file->openGroup("instrument", "NXinstrument");

  // Look for all the banks
  std::map<std::string, std::string> entries = file->getEntries();
  std::map<std::string, std::string>::iterator it;
  for (it = entries.begin(); it != entries.end(); ++it) {
    std::string name = it->first;
    if (name.size() > 4) {
      if (name.substr(0, 4) == "bank") {
        // OK, this is some bank data
        file->openGroup(name, it->second);

        // -------------- Find the data field name ----------------------------
        if (m_dataField.empty()) {
          std::map<std::string, std::string> entries = file->getEntries();
          std::map<std::string, std::string>::iterator it;
          for (it = entries.begin(); it != entries.end(); ++it) {
            if (it->second == "SDS") {
              file->openData(it->first);
              if (file->hasAttr("signal")) {
                int signal = 0;
                file->getAttr("signal", signal);
                if (signal == m_signalNo) {
                  // That's the right signal!
                  m_dataField = it->first;
                  // Find the corresponding X axis
                  std::string axes;
                  m_assumeOldFile = false;
                  if (!file->hasAttr("axes")) {
                    if (1 != m_signalNo) {
                      throw std::runtime_error(
                          "Your chosen signal number, " +
                          Strings::toString(m_signalNo) +
                          ", corresponds to the data field '" + m_dataField +
                          "' has no 'axes' attribute specifying.");
                    } else {
                      m_assumeOldFile = true;
                      axes = "x_pixel_offset,y_pixel_offset,time_of_flight";
                    }
                  }

                  if (!m_assumeOldFile) {
                    file->getAttr("axes", axes);
                  }

                  std::vector<std::string> allAxes;
                  boost::split(allAxes, axes,
                               boost::algorithm::detail::is_any_ofF<char>(","));
                  if (allAxes.size() != 3)
                    throw std::runtime_error(
                        "Your chosen signal number, " +
                        Strings::toString(m_signalNo) +
                        ", corresponds to the data field '" + m_dataField +
                        "' which has only " +
                        Strings::toString(allAxes.size()) +
                        " dimension. Expected 3 dimensions.");

                  m_axisField = allAxes.back();
                  g_log.information() << "Loading signal " << m_signalNo << ", "
                                      << m_dataField << " with axis "
                                      << m_axisField << std::endl;
                  file->closeData();
                  break;
                } // Data has a 'signal' attribute
              }   // Yes, it is a data field
              file->closeData();
            } // each entry in the group
          }
        }
        file->closeGroup();
      } // bankX name
    }
  } // each entry

  if (m_dataField.empty())
    throw std::runtime_error("Your chosen signal number, " +
                             Strings::toString(m_signalNo) +
                             ", was not found in any of the data fields of any "
                             "'bankX' group. Cannot load file.");

  for (it = entries.begin(); it != entries.end(); ++it) {
    std::string name = it->first;
    if (name.size() > 4) {
      if (name.substr(0, 4) == "bank") {
        // OK, this is some bank data
        file->openGroup(name, it->second);
        std::map<std::string, std::string> entries = file->getEntries();

        if (entries.find("pixel_id") != entries.end()) {
          bankNames.push_back(name);

          // Count how many pixels in the bank
          file->openData("pixel_id");
          std::vector<int64_t> dims = file->getInfo().dims;
          file->closeData();

          if (!dims.empty()) {
            size_t newPixels = 1;
            for (size_t i = 0; i < dims.size(); i++)
              newPixels *= dims[i];
            m_numPixels += newPixels;
          }
        } else {
          bankNames.push_back(name);

          // Get the number of pixels from the offsets arrays
          file->openData("x_pixel_offset");
          std::vector<int64_t> xdim = file->getInfo().dims;
          file->closeData();

          file->openData("y_pixel_offset");
          std::vector<int64_t> ydim = file->getInfo().dims;
          file->closeData();

          if (!xdim.empty() && !ydim.empty()) {
            m_numPixels += (xdim[0] * ydim[0]);
          }
        }

        if (entries.find(m_axisField) != entries.end()) {
          // Get the size of the X vector
          file->openData(m_axisField);
          std::vector<int64_t> dims = file->getInfo().dims;
          // Find the units, if available
          if (file->hasAttr("units"))
            file->getAttr("units", m_xUnits);
          else
            m_xUnits = "microsecond"; // use default
          file->closeData();
          if (!dims.empty())
            m_numBins = dims[0] - 1;
        }

        file->closeGroup();
      } // bankX name
    }
  } // each entry
  file->close();

  delete file;
}

/*for (std::vector<uint32_t>::iterator it = pixel_id.begin(); it !=
pixel_id.end();)
{
  detid_t pixelID = *it;
  specid_t wi = static_cast<specid_t>((*id_to_wi)[pixelID]);
  // spectrum is just wi+1
  if (wi+1 < m_spec_min || wi+1 > m_spec_max) pixel_id.erase(it);
  else ++it;
}*/
// Function object for remove_if STL algorithm
namespace {
// Check the numbers supplied are not in the range and erase the ones that are
struct range_check {
  range_check(specid_t min, specid_t max, detid2index_map id_to_wi)
      : m_min(min), m_max(max), m_id_to_wi(id_to_wi) {}

  bool operator()(specid_t x) {
    specid_t wi = static_cast<specid_t>((m_id_to_wi)[x]);
    return (wi + 1 < m_min || wi + 1 > m_max);
  }

private:
  specid_t m_min;
  specid_t m_max;
  detid2index_map m_id_to_wi;
};
}

/** Load a single bank into the workspace
 *
 * @param nexusfilename :: file to open
 * @param entry_name :: NXentry name
 * @param bankName :: NXdata bank name
 * @param WS :: workspace to modify
 * @param id_to_wi :: det ID to workspace index mapping
 */
void LoadTOFRawNexus::loadBank(const std::string &nexusfilename,
                               const std::string &entry_name,
                               const std::string &bankName,
                               API::MatrixWorkspace_sptr WS,
                               const detid2index_map &id_to_wi) {
  g_log.debug() << "Loading bank " << bankName << std::endl;
  // To avoid segfaults on RHEL5/6 and Fedora
  m_fileMutex.lock();

  // Navigate to the point in the file
  ::NeXus::File *file = new ::NeXus::File(nexusfilename);
  file->openGroup(entry_name, "NXentry");
  file->openGroup("instrument", "NXinstrument");
  file->openGroup(bankName, "NXdetector");

  size_t m_numPixels = 0;
  std::vector<uint32_t> pixel_id;

  if (!m_assumeOldFile) {
    // Load the pixel IDs
    file->readData("pixel_id", pixel_id);
    m_numPixels = pixel_id.size();
    if (m_numPixels == 0) {
      file->close();
      m_fileMutex.unlock();
      g_log.warning() << "Invalid pixel_id data in " << bankName << std::endl;
      return;
    }
  } else {
    // Load the x and y pixel offsets
    std::vector<float> xoffsets;
    std::vector<float> yoffsets;
    file->readData("x_pixel_offset", xoffsets);
    file->readData("y_pixel_offset", yoffsets);

    m_numPixels = xoffsets.size() * yoffsets.size();
    if (0 == m_numPixels) {
      file->close();
      m_fileMutex.unlock();
      g_log.warning() << "Invalid (x,y) offsets in " << bankName << std::endl;
      return;
    }

    size_t bankNum = 0;
    if (bankName.size() > 4) {
      if (bankName.substr(0, 4) == "bank") {
        bankNum = boost::lexical_cast<size_t>(bankName.substr(4));
        bankNum--;
      } else {
        file->close();
        m_fileMutex.unlock();
        g_log.warning() << "Invalid bank number for " << bankName << std::endl;
        return;
      }
    }

    // All good, so construct the pixel ID listing
    size_t numX = xoffsets.size();
    size_t numY = yoffsets.size();

    for (size_t i = 0; i < numX; i++) {
      for (size_t j = 0; j < numY; j++) {
        pixel_id.push_back(
            static_cast<uint32_t>(j + numY * (i + numX * bankNum)));
      }
    }
  }

  size_t iPart = 0;
  if (m_spec_max != Mantid::EMPTY_INT()) {
    uint32_t ifirst = pixel_id[0];
    range_check out_range(m_spec_min, m_spec_max, id_to_wi);
    std::vector<uint32_t>::iterator newEnd =
        std::remove_if(pixel_id.begin(), pixel_id.end(), out_range);
    pixel_id.erase(newEnd, pixel_id.end());
    // check if beginning or end of array was erased
    if (ifirst != pixel_id[0])
      iPart = m_numPixels - pixel_id.size();
    m_numPixels = pixel_id.size();
    if (m_numPixels == 0) {
      file->close();
      m_fileMutex.unlock();
      g_log.warning() << "No pixels from " << bankName << std::endl;
      return;
    };
  }
  // Load the TOF vector
  std::vector<float> tof;
  file->readData(m_axisField, tof);
  size_t m_numBins = tof.size() - 1;
  if (tof.size() <= 1) {
    file->close();
    m_fileMutex.unlock();
    g_log.warning() << "Invalid " << m_axisField << " data in " << bankName
                    << std::endl;
    return;
  }

  // Make a shared pointer
  MantidVecPtr Xptr;
  MantidVec &X = Xptr.access();
  X.resize(tof.size(), 0);
  X.assign(tof.begin(), tof.end());

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
  if (hasErrors) {
    try {
      file->openData(errorsField);
      file->getDataCoerce(errors);
      file->closeData();
    } catch (...) {
      g_log.information() << "Error loading the errors field, '" << errorsField
                          << "' for bank " << bankName
                          << ". Will use sqrt(counts). " << std::endl;
      hasErrors = false;
    }
  }

  /*if (data.size() != m_numBins * m_numPixels)
  { file->close(); m_fileMutex.unlock(); g_log.warning() << "Invalid size of '"
  << m_dataField << "' data in " << bankName << std::endl; return; }
  if (hasErrors && (errors.size() != m_numBins * m_numPixels))
  { file->close(); m_fileMutex.unlock(); g_log.warning() << "Invalid size of '"
  << errorsField << "' errors in " << bankName << std::endl; return; }
*/
  // Have all the data I need
  m_fileMutex.unlock();
  file->close();

  for (size_t i = iPart; i < iPart + m_numPixels; i++) {
    // Find the workspace index for this detector
    detid_t pixelID = pixel_id[i - iPart];
    size_t wi = id_to_wi.find(pixelID)->second;

    // Set the basic info of that spectrum
    ISpectrum *spec = WS->getSpectrum(wi);
    spec->setSpectrumNo(specid_t(wi + 1));
    spec->setDetectorID(pixel_id[i - iPart]);
    // Set the shared X pointer
    spec->setX(X);

    // Extract the Y
    MantidVec &Y = spec->dataY();
    Y.assign(data.begin() + i * m_numBins, data.begin() + (i + 1) * m_numBins);

    MantidVec &E = spec->dataE();

    if (hasErrors) {
      // Copy the errors from the loaded document
      E.assign(errors.begin() + i * m_numBins,
               errors.begin() + (i + 1) * m_numBins);
    } else {
      // Now take the sqrt(Y) to give E
      E = Y;
      std::transform(E.begin(), E.end(), E.begin(), (double (*)(double))sqrt);
    }
  }

  // Done!
}

//-------------------------------------------------------------------------------------------------
/** @return the name of the entry that we will load */
std::string LoadTOFRawNexus::getEntryName(const std::string &filename) {
  std::string entry_name = "entry";
  ::NeXus::File *file = new ::NeXus::File(filename);
  std::map<std::string, std::string> entries = file->getEntries();
  file->close();
  delete file;

  if (entries.empty())
    throw std::runtime_error("No entries in the NXS file!");

  // name "entry" is normal, but "entry-state0" is the name of the real state
  // for live nexus files.
  if (entries.find(entry_name) == entries.end())
    entry_name = "entry-state0";
  // If that doesn't exist, just take the first entry.
  if (entries.find(entry_name) == entries.end())
    entry_name = entries.begin()->first;
  //  // Tell the user
  //  if (entries.size() > 1)
  //    g_log.notice() << "There are " << entries.size() << " NXentry's in the
  //    file. Loading entry '" << entry_name << "' only." << std::endl;

  return entry_name;
}

//-------------------------------------------------------------------------------------------------
/** Executes the algorithm. Reading in the file and creating and populating
 *  the output workspace
 *
 *  @throw Exception::FileError If the Nexus file cannot be found/opened
 *  @throw std::invalid_argument If the optional properties are set to invalid
 *values
 */
void LoadTOFRawNexus::exec() {
  // The input properties
  std::string filename = getPropertyValue("Filename");
  m_signalNo = getProperty("Signal");
  m_spec_min = getProperty("SpectrumMin");
  m_spec_max = getProperty("SpectrumMax");

  // Find the entry name we want.
  std::string entry_name = LoadTOFRawNexus::getEntryName(filename);

  // Count pixels and other setup
  Progress *prog = new Progress(this, 0.0, 1.0, 10);
  prog->doReport("Counting pixels");
  std::vector<std::string> bankNames;
  countPixels(filename, entry_name, bankNames);
  g_log.debug() << "Workspace found to have " << m_numPixels << " pixels and "
                << m_numBins << " bins" << std::endl;

  prog->setNumSteps(bankNames.size() + 5);

  prog->doReport("Creating workspace");
  // Start with a dummy WS just to hold the logs and load the instrument
  MatrixWorkspace_sptr WS = WorkspaceFactory::Instance().create(
      "Workspace2D", m_numPixels, m_numBins + 1, m_numBins);

  // Load the logs
  prog->doReport("Loading DAS logs");
  g_log.debug() << "Loading DAS logs" << std::endl;

  LoadEventNexus::runLoadNexusLogs(filename, WS, *this, false);

  // Load the instrument
  prog->report("Loading instrument");
  g_log.debug() << "Loading instrument" << std::endl;
  LoadEventNexus::runLoadInstrument(filename, WS, entry_name, this);

  // Load the meta data, but don't stop on errors
  prog->report("Loading metadata");
  g_log.debug() << "Loading metadata" << std::endl;
  try {
    LoadEventNexus::loadEntryMetadata(filename, WS, entry_name);
  } catch (std::exception &e) {
    g_log.warning() << "Error while loading meta data: " << e.what()
                    << std::endl;
  }

  // Set the spectrum number/detector ID at each spectrum. This is consistent
  // with LoadEventNexus for non-ISIS files.
  prog->report("Building Spectra Mapping");
  g_log.debug() << "Building Spectra Mapping" << std::endl;
  WS->rebuildSpectraMapping(false);
  // And map ID to WI
  g_log.debug() << "Mapping ID to WI" << std::endl;
  const auto id_to_wi = WS->getDetectorIDToWorkspaceIndexMap();

  // Load each bank sequentially
  // PARALLEL_FOR1(WS)
  for (int i = 0; i < int(bankNames.size()); i++) {
    //    PARALLEL_START_INTERUPT_REGION
    std::string bankName = bankNames[i];
    prog->report("Loading bank " + bankName);
    g_log.debug() << "Loading bank " << bankName << std::endl;
    loadBank(filename, entry_name, bankName, WS, id_to_wi);
    //    PARALLEL_END_INTERUPT_REGION
  }
  //  PARALLEL_CHECK_INTERUPT_REGION

  // Set some units
  if (m_xUnits == "Ang")
    WS->getAxis(0)->setUnit("dSpacing");
  else if (m_xUnits == "invAng")
    WS->getAxis(0)->setUnit("MomentumTransfer");
  else
    // Default to TOF for any other string
    WS->getAxis(0)->setUnit("TOF");
  WS->setYUnit("Counts");

  // Set to the output
  setProperty("OutputWorkspace", WS);

  delete prog;
}

} // namespace DataHandling
} // namespace Mantid
