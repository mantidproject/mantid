//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidMDAlgorithms/LoadSQW2.h"

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/Progress.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidGeometry/MDGeometry/MDHistoDimensionBuilder.h"
#include "MantidKernel/make_unique.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/ThreadScheduler.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/V3D.h"

namespace Mantid {
namespace MDAlgorithms {

using Geometry::Goniometer;
using Geometry::OrientedLattice;
using Kernel::BinaryStreamReader;
using Kernel::Logger;
using Kernel::Matrix;
using Kernel::V3D;

//------------------------------------------------------------------------------
// Constants
//------------------------------------------------------------------------------
/// Defines buffer size for reading the pixel data. It is assumed to be the
/// number of pixels to read in a single call. A single pixel is 9 float
/// fields. 150000 is ~5MB buffer
constexpr int64_t NPIX_CHUNK = 150000;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadSQW2)

//------------------------------------------------------------------------------
// Public methods
//------------------------------------------------------------------------------
/// Default constructor
LoadSQW2::LoadSQW2()
    : API::Algorithm(), m_file(), m_reader(), m_outputWS(), m_progress() {}

/// Default destructor
LoadSQW2::~LoadSQW2() {}

/// Algorithms name for identification. @see Algorithm::name
const std::string LoadSQW2::name() const { return "LoadSQW"; }

/// Algorithm's version for identification. @see Algorithm::version
int LoadSQW2::version() const { return 2; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string LoadSQW2::category() const {
  return "DataHandling\\SQW;MDAlgorithms\\DataHandling";
}

/// Algorithm's summary for use in the GUI and help. @see
/// Algorithm::summary
const std::string LoadSQW2::summary() const {
  return "Load an N-dimensional workspace from a .sqw file produced by "
         "Horace.";
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
  readAllSPEHeadersToWorkspace(mainHeader.nfiles);
  skipDetectorSection();
  readDataSection();
  finalize();
}

/**
 * Opens the file given to the algorithm and initializes the reader
 */
void LoadSQW2::initFileReader() {
  using API::Progress;
  using Kernel::make_unique;
  m_file = make_unique<std::ifstream>(getPropertyValue("Filename"),
                                      std::ios_base::binary);
  m_reader = make_unique<BinaryStreamReader>(*m_file);
  // steps are reset once we know what we are reading
  m_progress = make_unique<Progress>(this, 0.0, 1.0, 100);
}

/**
 * Reads the initial header section. Skips specifically the
 * following: app_name, app_version, sqw_type, ndims, filename, filepath,
 * title.
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

/// Create the output workspace object
void LoadSQW2::createOutputWorkspace() {
  m_outputWS = boost::make_shared<SQWWorkspace>();
}

/**
 * Read all of the SPE headers and fill in the experiment details on the
 * output workspace
 * @param nfiles The number of expected spe header sections
 */
void LoadSQW2::readAllSPEHeadersToWorkspace(const int32_t nfiles) {
  using API::ExperimentInfo;
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

  // Skip the per-spe file projection information. We only use the
  // information from the data section
  m_file->seekg(96, std::ios_base::cur);
  std::vector<int32_t> ulabel_shape(2);
  m_reader->read(ulabel_shape, 2);
  // shape[0]*shape[1]*sizeof(char)
  m_file->seekg(ulabel_shape[0] * ulabel_shape[1], std::ios_base::cur);
}

/**
 * Skip the data in the detector section. The size is based on the number
 * of contribution detector parameters
 */
void LoadSQW2::skipDetectorSection() {
  std::string filename, filepath;
  *m_reader >> filename >> filepath;
  int32_t ndet(0);
  *m_reader >> ndet;
  if (g_log.is(Logger::Priority::PRIO_DEBUG)) {
    std::stringstream os;
    os << "Skipping " << ndet << " detector parameters from '" << filename
       << "'\n";
    g_log.debug(os.str());
  }
  // 6 float fields all ndet long - group, x2, phi, azim, width, height
  m_file->seekg(6 * 4 * ndet, std::ios_base::cur);
}

void LoadSQW2::readDataSection() {
  skipDataSectionMetadata();
  readSQWDimensions();
  readPixelData();
}

/**
 * Skip metadata in data section: filename, filepath, title, lattice,
 * projection information.
 * On exit the file pointer will be positioned after the last
 * ulen entry
 */
void LoadSQW2::skipDataSectionMetadata() {
  std::string filename, filepath, title;
  *m_reader >> filename >> filepath >> title;
  // skip alatt, angdeg, uoffset, u_to_rlu, ulen
  m_file->seekg(120, std::ios_base::cur);
}

/**
 * Read and create the SQW dimensions on the output
 * On exit the file pointer will be positioned after the last
 * ulimit entry
 */
void LoadSQW2::readSQWDimensions() {
  using Geometry::MDHistoDimension;
  using Geometry::MDHistoDimensionBuilder;

  // dimension labels
  std::vector<int32_t> ulabelShape(2);
  m_reader->read(ulabelShape, 2);
  std::vector<std::string> dimNames;
  m_reader->read(dimNames, ulabelShape,
                 BinaryStreamReader::MatrixOrdering::ColumnMajor);
  // projection information
  int32_t npax(0);
  *m_reader >> npax;
  int32_t niax(4 - npax);
  if (niax > 0) {
    // skip iaxes, iint (2 values per axis)
    m_file->seekg(niax * sizeof(int32_t) + 2 * niax * sizeof(float),
                  std::ios_base::cur);
  }
  std::vector<int32_t> nbins(npax, 1);
  int32_t signalLength(1);
  if (npax > 0) {
    // indices (starting at 1) of projection axes
    std::vector<int32_t> pax;
    m_reader->read(pax, npax);
    for (int32_t i = 0; i < npax; ++i) {
      int32_t np(0);
      *m_reader >> np;
      nbins[pax[i] - 1] = np - 1;
      signalLength *= np - 1;
      m_file->seekg(np * sizeof(float), std::ios_base::cur);
    }
    // skip display axes
    m_file->seekg(npax * sizeof(int32_t), std::ios_base::cur);
  }
  // skip signal(float), error data(float), npix(int64)
  m_file->seekg(2 * signalLength * sizeof(float) +
                    signalLength * sizeof(int64_t),
                std::ios_base::cur);
  // dimensions limits
  Matrix<float> urange;
  m_reader->read(urange, {int32_t(2), int32_t(4)},
                 BinaryStreamReader::MatrixOrdering::ColumnMajor);
  assert(urange.numRows() == 2 && urange.numCols() == 4);

  if (g_log.is(Logger::Priority::PRIO_DEBUG)) {
    std::stringstream os;
    os << "Data:\n"
       << "    labels: ";
    for (const auto &val : dimNames) {
      os << val << ",";
    }
    os << "\n";
    os << "    ulimits (from file): ";
    for (size_t i = 0; i < 4; ++i) {
      os << "(" << urange[0][i] << "," << urange[1][i] << ") ";
    }
    os << "\n";
    os << "    nbins: (";
    for (const auto &val : nbins) {
      os << val << ",";
    }
    os << ")";
    g_log.debug(os.str());
  }

  // Create dimensions
  const char *ids[] = {"Q1", "Q2", "Q3", "DeltaE"};
  const char *units[] = {"A\\^-1", "A\\^-1", "A\\^-1", "mev"};
  const char *frames[] = {"HKL", "HKL", "HKL", "meV"};
  std::vector<float> dimMin{urange[0][0], urange[0][1], urange[0][2],
                            urange[0][3]};
  toHKL(dimMin[0], dimMin[1], dimMin[2], 0);
  std::vector<float> dimMax{urange[1][0], urange[1][1], urange[1][2],
                            urange[1][3]};
  toHKL(dimMax[0], dimMax[1], dimMax[2], 0);
  for (size_t i = 0; i < 4; ++i) {
    MDHistoDimensionBuilder builder;
    builder.setId(ids[i]);
    builder.setName(dimNames[i]);
    builder.setMin(dimMin[i]);
    builder.setMax(dimMax[i]);
    builder.setNumBins(static_cast<size_t>(nbins[i]));
    builder.setFrameName(frames[i]);
    builder.setUnits(units[i]);
    m_outputWS->addDimension(builder.create());
  }
  setupBoxController();
}

/**
 * Setup the box controller based on the bin structure
 */
void LoadSQW2::setupBoxController() {
  using Kernel::Timer;
  Timer timer;

  auto boxController = m_outputWS->getBoxController();
  for (size_t i = 0; i < 4; i++) {
    boxController->setSplitInto(i, m_outputWS->getDimension(i)->getNBins());
  }
  boxController->setMaxDepth(1);
  m_outputWS->initialize();
  // Start with a MDGridBox.
  m_outputWS->splitBox();

  g_log.debug() << "Time to setup box structure: " << timer.elapsed() << "s\n";
}

/**
 * Read the pixel data into the workspace
 */
void LoadSQW2::readPixelData() {
  using Kernel::Timer;
  Timer timer;

  // skip redundant field
  m_file->seekg(sizeof(int32_t), std::ios_base::cur);
  int64_t npixtot(0);
  *m_reader >> npixtot;
  g_log.debug() << "    npixtot: " << npixtot << "\n";
  m_progress->setNumSteps(npixtot);

  // Each pixel has 9 float fields. Do a chunked read to avoid
  // using too much memory for the buffer
  constexpr int32_t numFields(9);
  constexpr int64_t bufferSize(numFields * NPIX_CHUNK);
  std::vector<float> pixBuffer(bufferSize);
  int64_t pixelsToRead(npixtot);
  while (pixelsToRead > 0) {
    int64_t readNPix(pixelsToRead);
    if (readNPix > NPIX_CHUNK) {
      readNPix = NPIX_CHUNK;
    }
    m_reader->read(pixBuffer, numFields * readNPix);
    for (int64_t i = 0; i < readNPix; ++i) {
      addEventFromBuffer(pixBuffer.data() + i * 9);
      m_progress->report();
    }
    pixelsToRead -= readNPix;
  }
  g_log.debug() << "Time to read all pixels: " << timer.elapsed() << "s\n";
}

/**
 * Assume the given pointer points to the start of a full pixel and create
 * an MDEvent based on it
 * @param pixel A pointer assumed to point to at the start of a single pixel
 * from the data file
 */
void LoadSQW2::addEventFromBuffer(const float *pixel) {
  using DataObjects::MDEvent;
  auto u1(pixel[0]), u2(pixel[1]), u3(pixel[2]), en(pixel[3]);
  auto irun = static_cast<uint16_t>(pixel[4] - 1);
  toHKL(u1, u2, u3, irun);
  const auto idet = static_cast<detid_t>(pixel[5]);
  // skip energy bin
  auto signal = pixel[7];
  auto error = pixel[8];
  coord_t centres[4] = {u1, u2, u3, en};
  m_outputWS->addEvent(MDEvent<4>(signal, error * error, irun, idet, centres));
}


/**
 * Transform the given coordinates from U to the HKL frame
 * using the transformation defined for the given run
 * @param u1 Coordinate parallel to the beam axis
 * @param u2 Coordinate perpendicular to u1 and in the horizontal plane
 * @param u3 The cross-product of u1 & u2
 * @param runIndex Index into the experiment list
 */
void LoadSQW2::toHKL(float &u1, float &u2, float &u3, const uint16_t runIndex) {
  constexpr double invTwoPi = 0.5 / M_PI;
  const auto &sample = m_outputWS->getExperimentInfo(runIndex)->sample();
  const auto &uToRLU = sample.getOrientedLattice().getBinv();

  V3D uVec(static_cast<double>(u1), static_cast<double>(u2),
           static_cast<double>(u3));
  V3D qhkl = (uToRLU * uVec) * invTwoPi;
  u1 = static_cast<float>(qhkl[0]);
  u2 = static_cast<float>(qhkl[1]);
  u3 = static_cast<float>(qhkl[2]);
}

/**
 * Assumed to be the last step in the algorithm. Performs any steps
 * necessary after everything else has run successfully
 */
void LoadSQW2::finalize() {
  using Kernel::ThreadPool;
  using Kernel::ThreadSchedulerFIFO;
  auto *ts = new ThreadSchedulerFIFO();
  ThreadPool tp(ts);
  m_outputWS->splitAllIfNeeded(ts);
  tp.joinAll();
  m_outputWS->refreshCache();
  setProperty("OutputWorkspace", m_outputWS);
}

} // namespace MDAlgorithms
} // namespace Mantid
