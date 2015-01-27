#include "MantidLiveData/ISISHistoDataListener.h"
#include "MantidAPI/LiveListenerFactory.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/SpectrumDetectorMapping.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ArrayBoundedValidator.h"
// the following were previously defined in Poco/Platform_POSIX.h 
#ifdef GCC_DIAG_ON 
#undef GCC_DIAG_ON
#endif
#ifdef GCC_DIAG_OFF
#undef GCC_DIAG_OFF
#endif
#include "MantidKernel/WarningSuppressions.h"
#include "MantidGeometry/Instrument.h"

#ifdef GCC_VERSION
// Avoid compiler warnings on gcc from unused static constants in
// isisds_command.h
GCC_DIAG_OFF(unused-variable)
#endif
#include "LoadDAE/idc.h"

#include <boost/lexical_cast.hpp>
#include <boost/make_shared.hpp>

#include <algorithm>
#include <numeric>

using namespace Mantid::API;
using namespace Mantid::Geometry;
using Mantid::Kernel::ConfigService;

namespace Mantid {
namespace LiveData {
DECLARE_LISTENER(ISISHistoDataListener)

namespace {
/// static logger
Kernel::Logger g_log("ISISHistoDataListener");
}

/// Constructor
ISISHistoDataListener::ISISHistoDataListener()
    : ILiveListener(), isInitilized(false), m_daeHandle(NULL),
      m_timeRegime(-1) {
  declareProperty(new Kernel::ArrayProperty<specid_t>("SpectraList"),
                  "An optional list of spectra to load. If blank, all "
                  "available spectra will be loaded.");

  auto validator = boost::make_shared<Kernel::ArrayBoundedValidator<int>>();
  validator->setLower(1);
  declareProperty(new Kernel::ArrayProperty<int>("PeriodList", validator),
                  "An optional list of periods to load. If blank, all "
                  "available periods will be loaded.");
}

/// Destructor
ISISHistoDataListener::~ISISHistoDataListener() {
  if (m_daeHandle) {
    IDCclose(&m_daeHandle);
  }
}

/** Function called by IDC routines to report an error. Passes the error through
* to the logger
* @param status ::  The status code of the error (disregarded)
* @param code ::    The error code (disregarded)
* @param message :: The error message - passed to the logger at error level
*/
void ISISHistoDataListener::IDCReporter(int status, int code,
                                        const char *message) {
  (void)status;
  (void)code; // Avoid compiler warning
  g_log.error(message);
}

/** Connect to the specified address and checks that is valid
  *  @param address   The IP address and port to contact (port is ignored).
  *  @return True if the connection was successfully established
  */
bool ISISHistoDataListener::connect(const Poco::Net::SocketAddress &address) {

  m_daeName = address.toString();
  // remove the port part
  auto i = m_daeName.find(':');
  if (i != std::string::npos) {
    m_daeName.erase(i);
  }

  // set IDC reporter function for errors
  IDCsetreportfunc(&ISISHistoDataListener::IDCReporter);

  if (IDCopen(m_daeName.c_str(), 0, 0, &m_daeHandle, address.port()) != 0) {
    m_daeHandle = NULL;
    return false;
  }

  m_numberOfPeriods = getInt("NPER");
  g_log.information() << "Number of periods " << m_numberOfPeriods << std::endl;

  // Set the spectra list to load
  std::vector<specid_t> spectra = getProperty("SpectraList");
  if (!spectra.empty()) {
    setSpectra(spectra);
  }

  // Set the period list to load
  std::vector<int> periodList = getProperty("PeriodList");
  if (!periodList.empty()) {
    setPeriods(periodList);
  }

  loadSpectraMap();

  loadTimeRegimes();

  return true;
}

bool ISISHistoDataListener::isConnected() {
  if (m_daeHandle == NULL)
    return false;
  // try to read a parameter, success means connected
  int sv_dims_array[1] = {1}, sv_ndims = 1, buffer;
  int stat = IDCgetpari(m_daeHandle, "NPER", &buffer, sv_dims_array, &sv_ndims);
  return stat == 0;
}

ILiveListener::RunStatus ISISHistoDataListener::runStatus() {
  // In a run by default
  return Running;
}

int ISISHistoDataListener::runNumber() const {
  // Not available it would seem - just return 0
  return 0;
}

void ISISHistoDataListener::start(
    Kernel::DateAndTime /*startTime*/) // Ignore the start time
{
  return;
}

/**
 * Read the data from the DAE.
 * @return :: A workspace with the data.
 */
boost::shared_ptr<Workspace> ISISHistoDataListener::extractData() {

  if (m_timeRegime < 0) {
    m_timeRegime = getTimeRegimeToLoad();
    g_log.debug() << "Loading spectra for time regime " << m_timeRegime + 1
                  << std::endl;
  }

  if (!m_daeHandle) {
    g_log.error("DAE is not connected");
    throw Kernel::Exception::FileError("DAE is not connected ", m_daeName);
  }

  m_dataReset = false;
  isInitilized = true;

  // check that the dimensions haven't change since last time
  int numberOfPeriods = getInt("NPER");
  if (numberOfPeriods != m_numberOfPeriods) {
    g_log.error("Data dimensions changed");
    throw Kernel::Exception::FileError("Data dimensions changed", m_daeName);
  }

  loadTimeRegimes();

  // buffer to read loat values in
  std::vector<float> floatBuffer;

  // read in the proton charge
  getFloatArray("RRPB", floatBuffer, 32);
  const double protonCharge = floatBuffer[8];

  // find out the number of histograms in the output workspace
  const size_t numberOfHistograms =
      m_specList.empty() ? m_numberOfSpectra[m_timeRegime] : m_specList.size();

  // Create the 2D workspace for the output
  auto localWorkspace = WorkspaceFactory::Instance().create(
      "Workspace2D", numberOfHistograms, m_numberOfBins[m_timeRegime] + 1,
      m_numberOfBins[m_timeRegime]);

  // Set the unit on the workspace to TOF
  localWorkspace->getAxis(0)->unit() =
      Kernel::UnitFactory::Instance().create("TOF");
  localWorkspace->setYUnit("Counts");
  localWorkspace->updateSpectraUsing(
      SpectrumDetectorMapping(m_specIDs, m_detIDs));

  // cut the spectra numbers into chunks
  std::vector<int> index, count;
  calculateIndicesForReading(index, count);

  int firstPeriod = m_periodList.empty() ? 0 : m_periodList.front() - 1;

  // create a workspace group in case the data are multiperiod
  auto workspaceGroup = API::WorkspaceGroup_sptr(new API::WorkspaceGroup);
  // loop over periods and spectra and fill in the output workspace
  for (int period = 0; period < m_numberOfPeriods; ++period) {
    if (isPeriodIgnored(period))
      continue;

    if (period > firstPeriod) {
      // create a new matrix workspace similar to the previous, copy over the
      // instrument info
      localWorkspace = WorkspaceFactory::Instance().create(localWorkspace);
      workspaceGroup->addWorkspace(localWorkspace);
    }
    size_t workspaceIndex = 0;
    for (size_t i = 0; i < index.size(); ++i) {
      getData(period, index[i], count[i], localWorkspace, workspaceIndex);
      workspaceIndex += count[i];
    }

    if (period == firstPeriod) {
      // Only run the Child Algorithms once
      runLoadInstrument(localWorkspace, getString("NAME"));
      if (m_numberOfPeriods > 1) {
        // adding first ws to the group after loading instrument
        // otherwise ws can be lost.
        workspaceGroup->addWorkspace(localWorkspace);
      }
      // Set the total proton charge for this run
      localWorkspace->mutableRun().setProtonCharge(protonCharge);
    }
  }

  if (m_numberOfPeriods > 1 &&
      (m_periodList.empty() || m_periodList.size() > 1)) {
    return workspaceGroup;
  }

  return localWorkspace;
}

/**
 * Read an integer parameter from the DAE
 * @param par :: Parameter name
 */
int ISISHistoDataListener::getInt(const std::string &par) const {
  int sv_dims_array[1] = {1}, sv_ndims = 1, buffer;
  int stat =
      IDCgetpari(m_daeHandle, par.c_str(), &buffer, sv_dims_array, &sv_ndims);
  if (stat != 0) {
    g_log.error("Unable to read " + par + " from DAE " + m_daeName);
    throw Kernel::Exception::FileError("Unable to read " + par + " from DAE ",
                                       m_daeName);
  }
  return buffer;
}

/**
 * Read a string parameter from the DAE
 * @param par :: Parameter name
 */
std::string ISISHistoDataListener::getString(const std::string &par) const {
  const int maxSize = 1024;
  char buffer[maxSize];
  int dim = maxSize, ndims = 1;
  if (IDCgetparc(m_daeHandle, par.c_str(), (char *)buffer, &dim, &ndims) != 0) {
    g_log.error("Unable to read " + par + " from DAE " + m_daeName);
    throw Kernel::Exception::FileError("Unable to read " + par + " from DAE ",
                                       m_daeName);
  };
  return std::string(buffer, dim);
}

/** Sets a list of spectra to be extracted. Default is reading all available
 * spectra.
  * @param specList :: A vector with spectra indices.
  */
void ISISHistoDataListener::setSpectra(const std::vector<specid_t> &specList) {
  // after listener has created its first workspace the spectra numbers cannot
  // be changed
  if (!isInitilized) {
    m_specList = specList;
  }
}

/** Sets a list of periods to be extracted. Default is reading all available
 * periods.
  * @param periodList :: A vector with period numbers.
  */
void
ISISHistoDataListener::setPeriods(const std::vector<specid_t> &periodList) {
  // after listener has created its first workspace the period numbers cannot be
  // changed
  if (!isInitilized) {
    m_periodList = periodList;
    if (*std::max_element(m_periodList.begin(), m_periodList.end()) >
        m_numberOfPeriods) {
      throw std::invalid_argument(
          "Invalid period(s) specified. Maximum " +
          boost::lexical_cast<std::string>(m_numberOfPeriods));
    }
  }
}

/**
 * Read an array of floats from the DAE
 * @param par :: Array name
 * @param arr :: A vector to store the values
 * @param dim :: Size of the array
 */
void ISISHistoDataListener::getFloatArray(const std::string &par,
                                          std::vector<float> &arr,
                                          const size_t dim) {
  int dims = static_cast<int>(dim), ndims = 1;
  arr.resize(dim);
  if (IDCgetparr(m_daeHandle, par.c_str(), arr.data(), &dims, &ndims) != 0) {
    g_log.error("Unable to read " + par + " from DAE " + m_daeName);
    throw Kernel::Exception::FileError("Unable to read " + par + " from DAE ",
                                       m_daeName);
  }
}

/**
 * Read an array of ints from the DAE
 * @param par :: Array name
 * @param arr :: A vector to store the values
 * @param dim :: Size of the array
 */
void ISISHistoDataListener::getIntArray(const std::string &par,
                                        std::vector<int> &arr,
                                        const size_t dim) {
  int dims = static_cast<int>(dim), ndims = 1;
  arr.resize(dim);
  if (IDCgetpari(m_daeHandle, par.c_str(), arr.data(), &dims, &ndims) != 0) {
    g_log.error("Unable to read " + par + " from DAE " + m_daeName);
    throw Kernel::Exception::FileError("Unable to read " + par + " from DAE ",
                                       m_daeName);
  }
}

/**
 * Split up all spectra into chunks
 * @param index :: Vector of first indices of a chunk.
 * @param count :: Numbers of spectra in each chunk.
 */
void
ISISHistoDataListener::calculateIndicesForReading(std::vector<int> &index,
                                                  std::vector<int> &count) {
  const int numberOfBins = m_numberOfBins[m_timeRegime];
  const int numberOfSpectra = m_numberOfSpectra[m_timeRegime];
  // max number of spectra that could be read in in one go
  int maxNumberOfSpectra = 1024 * 1024 / (numberOfBins * (int)sizeof(int));
  if (maxNumberOfSpectra == 0) {
    maxNumberOfSpectra = 1;
  }

  if (m_specList.empty()) {
    // make sure the chunk sizes < maxNumberOfSpectra
    int spec = 1;
    int n = numberOfSpectra;
    while (n > 0) {
      if (n < maxNumberOfSpectra) {
        index.push_back(spec);
        count.push_back(n);
        break;
      } else {
        index.push_back(spec);
        count.push_back(maxNumberOfSpectra);
        n -= maxNumberOfSpectra;
        spec += maxNumberOfSpectra;
      }
    }
  } else {
    // combine consecutive spectra but don't exceed the maxNumberOfSpectra
    size_t i0 = 0;
    specid_t spec = m_specList[i0];
    for (size_t i = 1; i < m_specList.size(); ++i) {
      specid_t next = m_specList[i];
      if (next - m_specList[i - 1] > 1 ||
          static_cast<int>(i - i0) >= maxNumberOfSpectra) {
        int n = static_cast<int>(i - i0);
        index.push_back(spec);
        count.push_back(n);
        i0 = i;
        spec = next;
      }
    }
    int n = static_cast<int>(m_specList.size() - i0);
    index.push_back(spec);
    count.push_back(n);
  }
}

/**
 * Read spectra from the DAE
 * @param period :: Current period index
 * @param index :: First spectrum index
 * @param count :: Number of spectra to read
 * @param workspace :: Workspace to store the data
 * @param workspaceIndex :: index in workspace to store data
 */
void ISISHistoDataListener::getData(int period, int index, int count,
                                    API::MatrixWorkspace_sptr workspace,
                                    size_t workspaceIndex) {
  const int numberOfBins = m_numberOfBins[m_timeRegime];
  const size_t bufferSize = count * (numberOfBins + 1) * sizeof(int);
  std::vector<int> dataBuffer(bufferSize);
  // Read in spectra from DAE
  int ndims = 2, dims[2];
  dims[0] = count;
  dims[1] = numberOfBins + 1;

  int spectrumIndex = index + period * (m_totalNumberOfSpectra + 1);
  if (IDCgetdat(m_daeHandle, spectrumIndex, count, dataBuffer.data(), dims,
                &ndims) != 0) {
    g_log.error("Unable to read DATA from DAE " + m_daeName);
    throw Kernel::Exception::FileError("Unable to read DATA from DAE ",
                                       m_daeName);
  }

  for (size_t i = 0; i < static_cast<size_t>(count); ++i) {
    size_t wi = workspaceIndex + i;
    workspace->setX(wi, m_bins[m_timeRegime]);
    MantidVec &y = workspace->dataY(wi);
    MantidVec &e = workspace->dataE(wi);
    workspace->getSpectrum(wi)->setSpectrumNo(index + static_cast<specid_t>(i));
    size_t shift = i * (numberOfBins + 1) + 1;
    y.assign(dataBuffer.begin() + shift, dataBuffer.begin() + shift + y.size());
    std::transform(y.begin(), y.end(), e.begin(), dblSqrt);
  }
}

/** Populate spectra-detector map
 *
 */
void ISISHistoDataListener::loadSpectraMap() {
  // Read in the number of detectors
  int ndet = getInt("NDET");
  getIntArray("UDET", m_detIDs, ndet);
  getIntArray("SPEC", m_specIDs, ndet);
}

/** Run the Child Algorithm LoadInstrument (or LoadInstrumentFromRaw).
 *  @param localWorkspace :: The workspace
 *  @param iName :: The instrument name
 */
void
ISISHistoDataListener::runLoadInstrument(MatrixWorkspace_sptr localWorkspace,
                                         const std::string &iName) {
  auto loadInst =
      API::AlgorithmFactory::Instance().create("LoadInstrument", -1);
  if (!loadInst)
    return;
  loadInst->initialize();
  try {
    loadInst->setPropertyValue("InstrumentName", iName);
    loadInst->setProperty<MatrixWorkspace_sptr>("Workspace", localWorkspace);
    loadInst->setProperty("RewriteSpectraMap", false);
    loadInst->executeAsChildAlg();
  } catch (std::invalid_argument &) {
    g_log.information("Invalid argument to LoadInstrument Child Algorithm");
  } catch (std::runtime_error &) {
    g_log.information(
        "Unable to successfully run LoadInstrument Child Algorithm");
  }
  if (API::AnalysisDataService::Instance().doesExist("Anonymous")) {
    // LoadInstrument adds the workspace to ADS as Anonymous
    // we don't want it there
    API::AnalysisDataService::Instance().remove("Anonymous");
  }
}

/**
 * Determine the number of time regimes.
 * Load time regime for each spectrum, bin boundaries and number of spectra for
 * each regime.
 */
void ISISHistoDataListener::loadTimeRegimes() {
  // Expecting loadSpectraMap() to be called first.
  if (m_specIDs.empty() || m_detIDs.empty()) {
    throw std::logic_error("Spectra-detector mapping must be loaded first.");
  }

  // prefix for DAE command to get the number of time channels (bins)
  const std::string ntcPrefix = "NTC";
  // prefix for DAE command to get the time channel boundaries
  const std::string rtcbPrefix = "RTCB";
  // prefix for DAE command to get the number of spectra
  const std::string nspPrefix = "NSP";

  // At the moment we cannot get the number of time regimes from the dae.
  // It will be possible in the future when Freddie adds a parameter for it.
  // For now we assume that two regimes are possible. The first must always be
  // present.
  // If there is nonzero number of time channels for the second one then we have
  // two regimes.
  for (size_t tr = 0; tr < 2; ++tr) {
    const std::string regime = boost::lexical_cast<std::string>(tr + 1);
    // get number of bins in this regime
    int nbins = getInt(ntcPrefix + regime);
    if (nbins == 0) {
      if (tr == 0) {
        throw std::runtime_error(
            "Didn't find any time bins for time regime 1.");
      }
      break;
    }
    // get number of spectra in this time regime
    int nspec = getInt(nspPrefix + regime);

    // if it's first call of this method populate the member variables
    if (m_bins.size() == tr) {
      m_numberOfBins.push_back(nbins);
      m_numberOfSpectra.push_back(nspec);
      // buffer to read float values in
      std::vector<float> floatBuffer;

      if (tr == 0) {
        // read in the bin boundaries
        getFloatArray(rtcbPrefix + regime, floatBuffer, nbins + 1);
      } else {
        // In principle bin boundaries for all regimes should be loaded
        // the same way as for tr == 0 but because of a bug in the dae software
        // it only works for regime 1. What follows is a workaround.

        // number of monitors
        int nmon = getInt("NMON");
        // indices of monitors in m_detIDs and m_specIDs ( +1 )
        std::vector<int> monitorIndices;
        getIntArray("MDET", monitorIndices, nmon);

        // we make an assumtion that regime 2 is used for monitors only
        if (monitorIndices.empty()) {
          throw std::runtime_error("Time regime 2 is expected to be used for "
                                   "monitors but none are found.");
        }

        m_monitorSpectra.resize(nmon);
        for (size_t i = 0; i < m_monitorSpectra.size(); ++i) {
          m_monitorSpectra[i] = m_specIDs[monitorIndices[i] - 1];
        }

        for (auto mon = m_monitorSpectra.begin(); mon != m_monitorSpectra.end();
             ++mon) {
          g_log.information() << "Monitor spectrum " << *mon << std::endl;
        }

        const std::string detRTCB =
            rtcbPrefix + "_" +
            boost::lexical_cast<std::string>(m_monitorSpectra.front());
        // read in the bin boundaries
        getFloatArray(detRTCB, floatBuffer, nbins + 1);
      }

      // copy them into a MantidVec
      m_bins.push_back(boost::make_shared<MantidVec>(floatBuffer.begin(),
                                                     floatBuffer.end()));
    } else {
      // check that dimensions haven't changed
      if (nspec != m_numberOfSpectra[tr] || nbins != m_numberOfBins[tr]) {
        g_log.error("Data dimensions changed");
        throw Kernel::Exception::FileError("Data dimensions changed",
                                           m_daeName);
      }
    }
  }
  g_log.information() << "Number of time regimes " << m_bins.size()
                      << std::endl;
  assert(m_numberOfBins.size() == m_numberOfSpectra.size());
  for (size_t i = 0; i < m_numberOfBins.size(); ++i) {
    g_log.information() << "Number of bins in time regime " << i + 1 << " is "
                        << m_numberOfBins[i] << std::endl;
    g_log.information() << "Number of spectra in time regime " << i + 1
                        << " is " << m_numberOfSpectra[i] << std::endl;
  }

  // find the total number of spectra in all regimes
  m_totalNumberOfSpectra =
      std::accumulate(m_numberOfSpectra.begin(), m_numberOfSpectra.end(), 0);
}

/**
 * Get the time regime for which the data should be loaded.
 * If spectrum list isn't specified (all data) return regime 1.
 * If spectrum list is given return the common regime for all
 * spectra in the list. If regimes are mixed throw invalid_argument.
 * @return :: 0 (regime 1) or 1 (regime 2).
 */
int ISISHistoDataListener::getTimeRegimeToLoad() const {
  if (!m_specList.empty()) {
    if (m_monitorSpectra.empty())
      return 0;
    int regime = -1;
    for (auto specIt = m_specList.begin(); specIt != m_specList.end();
         ++specIt) {
      bool isMonitor =
          std::find(m_monitorSpectra.begin(), m_monitorSpectra.end(),
                    *specIt) != m_monitorSpectra.end();
      if (!isMonitor && *specIt > m_totalNumberOfSpectra)
        throw std::invalid_argument("Invalid spectra index is found: " +
                                    boost::lexical_cast<std::string>(*specIt));
      int specRegime = isMonitor ? 1 : 0;
      if (regime < 0) {
        regime = specRegime;
      } else if (specRegime != regime) {
        throw std::invalid_argument(
            "Cannot mix spectra in different time regimes.");
      }
    }
    return regime;
  }
  return 0;
}

/// Personal wrapper for sqrt to allow msvs to compile
double ISISHistoDataListener::dblSqrt(double in) { return sqrt(in); }

/**
 * Check if a data period should be ignored.
 * @param period :: Period to check.
 * @return :: True to ignore the period.
 */
bool ISISHistoDataListener::isPeriodIgnored(int period) const {
  if (m_periodList.empty())
    return false;
  return std::find(m_periodList.begin(), m_periodList.end(), period + 1) ==
         m_periodList.end();
}

} // namespace LiveData
} // namespace Mantid
