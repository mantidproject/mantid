//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidMDAlgorithms/LoadSQW2.h"

#include "MantidAPI/FileProperty.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidKernel/make_unique.h"
#include "MantidKernel/V3D.h"

namespace Mantid {
namespace MDAlgorithms {

using API::ExperimentInfo;
using Geometry::Goniometer;
using Geometry::OrientedLattice;
using Kernel::BinaryStreamReader;
using Kernel::Logger;
using Kernel::make_unique;
using Kernel::V3D;

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
  auto &sample = experiment.mutableSample();
  auto &run = experiment.mutableRun();

  std::string chars;
  // skip filename, filepath
  *m_reader >> chars >> chars;
  float efix(1.0f);
  int32_t emode(0);
  // add ei as log but skip emode
  *m_reader >> efix >> emode;
  run.addProperty("Ei", static_cast<double>(efix), true);

  // lattice - alatt, angdeg, cu, cv = 12 values
  std::vector<float> floats;
  m_reader->read(floats, 12);
  auto lattice = new OrientedLattice(floats[0], floats[1], floats[2], floats[3],
                                     floats[4], floats[5]);
  V3D uVec(floats[6], floats[7], floats[8]),
      vVec(floats[9], floats[10], floats[11]);
  lattice->setUFromVectors(uVec, vVec);
  sample.setOrientedLattice(lattice);
  if (g_log.is(Logger::Priority::PRIO_DEBUG)) {
    std::stringstream os;
    os << "Lattice:"
       << "    alatt: " << lattice->a1() << " " << lattice->a2() << " "
       << lattice->a3() << "\n"
       << "    angdeg: " << lattice->alpha() << " " << lattice->beta() << " "
       << lattice->gamma() << "\n"
       << "    cu: " << floats[6] << " " << floats[7] << " " << floats[8]
       << "\n"
       << "    cv: " << floats[9] << " " << floats[10] << " " << floats[11]
       << "\n";
    g_log.debug(os.str());
  }

  // goniometer angles
  float psi(0.0f), omega(0.0f), dpsi(0.0f), gl(0.0f), gs(0.0f);
  *m_reader >> psi >> omega >> dpsi >> gl >> gs;
  V3D uvCross = uVec.cross_prod(vVec);
  Goniometer goniometer;
  goniometer.pushAxis("psi", uvCross[0], uvCross[1], uvCross[2], psi);
  goniometer.pushAxis("omega", uvCross[0], uvCross[1], uvCross[2], omega);
  goniometer.pushAxis("gl", 1.0, 0.0, 0.0, gl);
  goniometer.pushAxis("gs", 0.0, 0.0, 1.0, gs);
  goniometer.pushAxis("dpsi", 0.0, 1.0, 0.0, dpsi);
  run.setGoniometer(goniometer, false);
  if (g_log.is(Logger::Priority::PRIO_DEBUG)) {
    std::stringstream os;
    os << "Goniometer angles:\n"
       << "    psi: " << psi << "\n"
       << "    omega: " << omega << "\n"
       << "    gl: " << gl << "\n"
       << "    gs: " << gs << "\n"
       << "    dpsi: " << dpsi << "\n"
       << "    goniometer matrix: " << goniometer.getR() << "\n";
    g_log.debug(os.str());
  }
  // energy bins
  int32_t nbounds(0);
  *m_reader >> nbounds;
  std::vector<float> enBins(nbounds);
  m_reader->read(enBins, nbounds);
  run.storeHistogramBinBoundaries(
      std::vector<double>(enBins.begin(), enBins.end()));

  // skip uoffset (4*4), u_to_rlu (16*4), ulen (4*4), ulabel_shape (2*4),
  // ulabel (size based on shape)
  m_file->seekg(96, std::ios_base::cur);
  std::vector<int32_t> ulabel_shape(2);
  m_reader->read(ulabel_shape, 2);
  m_file->seekg(ulabel_shape[0] * ulabel_shape[1], std::ios_base::cur);
}

/// Create the output workspace object
void LoadSQW2::createOutputWorkspace() {
  m_outputWS = boost::make_shared<SQWWorkspace>();
}

} // namespace MDAlgorithms
} // namespace Mantid
