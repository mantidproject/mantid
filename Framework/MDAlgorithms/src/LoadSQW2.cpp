//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidMDAlgorithms/LoadSQW2.h"

#include "MantidAPI/FileProperty.h"
#include "MantidKernel/make_unique.h"

namespace Mantid {
namespace MDAlgorithms {

using API::ExperimentInfo;
using Kernel::BinaryStreamReader;
using Kernel::Logger;
using Kernel::make_unique;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadSQW2)

//------------------------------------------------------------------------------
// Public methods
//------------------------------------------------------------------------------
/// Default constructor
LoadSQW2::LoadSQW2() : API::Algorithm(), m_file(), m_reader(), m_outputWS() {}

/// Default destructor
LoadSQW2::~LoadSQW2() {}

/// Algorithms name for identification. @see Algorithm::name
const std::string LoadSQW2::name() const { return "LoadSQW"; }

/// Algorithm's version for identification. @see Algorithm::version
int LoadSQW2::version() const { return 2; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string LoadSQW2::category() const { return "DataHandling"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string LoadSQW2::summary() const {
  return "Load an N-dimensional workspace from a .sqw file produced by Horace.";
}

//------------------------------------------------------------------------------
// Private methods
//------------------------------------------------------------------------------

/// Initialize the algorithm's properties.
void LoadSQW2::init() {
  using namespace API;

  declareProperty(
      new FileProperty("Filename", "", FileProperty::Load, {".sqw"}),
      "File of type SQW format");
  declareProperty(new WorkspaceProperty<IMDEventWorkspace>(
                      "OutputWorkspace", "", Kernel::Direction::Output),
                  "Output IMDEventWorkspace reflecting SQW data");
}

/// Execute the algorithm.
void LoadSQW2::exec() {
  initFileReader();
  auto mainHeader = readMainHeader();
  createOutputWorkspace();
  readAllSPEHeaders(mainHeader.nfiles);

  setProperty("OutputWorkspace", m_outputWS);
}

/**
 * Opens the file given to the algorithm and initializes the reader
 */
void LoadSQW2::initFileReader() {
  m_file = make_unique<std::ifstream>(getPropertyValue("Filename"),
                                      std::ios_base::binary);
  m_reader = make_unique<BinaryStreamReader>(*m_file);
}

/**
 * Reads the initial header section. Skips specifically the
 * following: app_name, app_version, sqw_type, ndims, filename, filepath, title.
 * Stores the number of files.
 * @return A SQWHeader object describing the section
 */
LoadSQW2::SQWHeader LoadSQW2::readMainHeader() {
  std::string appName, filename, filepath, title;
  double appVersion(0.0);
  int32_t sqwType(-1);
  int32_t numDims(-1);
  SQWHeader header;
  *m_reader >> appName >> appVersion >> sqwType >> numDims >> filename >>
      filepath >> title >> header.nfiles;

  if (g_log.is(Logger::Priority::PRIO_DEBUG)) {
    std::ostringstream os;
    os << "Main header:\n"
       << "    app_name: " << appName << "\n"
       << "    app_version: " << appVersion << "\n"
       << "    sqw_type: " << sqwType << "\n"
       << "    ndims: " << numDims << "\n"
       << "    filename: " << filename << "\n"
       << "    filepath: " << filepath << "\n"
       << "    title: " << title << "\n"
       << "    nfiles: " << header.nfiles << "\n";
    g_log.debug(os.str());
  }
  return header;
}

/**
 * Read all of the SPE headers and fill in the experiment details on the
 * output workspace
 * @param nfiles The number of expected spe header sections
 */
void LoadSQW2::readAllSPEHeaders(const int32_t nfiles) {
  for (int32_t i = 0; i < nfiles; ++i) {
    auto expt = boost::make_shared<ExperimentInfo>();
    readSingleSPEHeader(*expt);
    m_outputWS->addExperimentInfo(expt);
  }
}

/**
 * Read single SPE header from the file. It assumes the file stream
 * points at the start of a header section
 * @param experiment A reference to an ExperimentInfo object to store the data
 */
void LoadSQW2::readSingleSPEHeader(API::ExperimentInfo &experiment) {
  
}

/// Create the output workspace object
void LoadSQW2::createOutputWorkspace() {
  m_outputWS = boost::make_shared<SQWWorkspace>();
}

} // namespace MDAlgorithms
} // namespace Mantid
