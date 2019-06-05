// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/LoadSQW2.h"
#include "MantidMDAlgorithms/MDWSTransform.h"

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidDataObjects/BoxControllerNeXusIO.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidGeometry/MDGeometry/MDHistoDimensionBuilder.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/Memory.h"
#include "MantidKernel/ThreadScheduler.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/WarningSuppressions.h"

namespace Mantid {
namespace MDAlgorithms {

using API::ExperimentInfo;
using Geometry::Goniometer;
using Geometry::MDHistoDimensionBuilder;
using Geometry::OrientedLattice;
using Kernel::BinaryStreamReader;
using Kernel::DblMatrix;
using Kernel::Logger;
using Kernel::V3D;

namespace {
/// Defines buffer size for reading the pixel data. It is assumed to be the
/// number of pixels to read in a single call. A single pixel is 9 float
/// fields. 150000 is ~5MB buffer
constexpr int64_t NPIX_CHUNK = 150000;
/// The MD workspace will have its boxes split after reading this many
/// chunks of events;
constexpr int64_t NCHUNKS_SPLIT = 125;
/// Defines the number of fields that define a single pixel
constexpr int32_t FIELDS_PER_PIXEL = 9;
/// 1/2pi
constexpr double INV_TWO_PI = 0.5 / M_PI;
} // namespace

// Register the algorithm into the AlgorithmFactory
DECLARE_FILELOADER_ALGORITHM(LoadSQW2)

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

/**
 * Return the confidence with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadSQW2::confidence(Kernel::FileDescriptor &descriptor) const {
  // only .sqw can be considered
  const std::string &extn = descriptor.extension();
  if (extn != ".sqw")
    return 0;

  if (descriptor.isAscii()) {
    // Low so that others may try
    return 10;
  }
  // Beat v1
  return 81;
}

/// Initialize the algorithm's properties.
void LoadSQW2::init() {
  using namespace API;
  using Kernel::PropertyWithValue;
  using Kernel::StringListValidator;
  using StringInitializerList = std::initializer_list<std::string>;

  // Inputs
  declareProperty(
      std::make_unique<FileProperty>("Filename", "", FileProperty::Load,
                                     StringInitializerList({".sqw"})),
      "File of type SQW format");
  declareProperty(
      std::make_unique<PropertyWithValue<bool>>("MetadataOnly", false),
      "Load Metadata without events.");
  declareProperty(std::make_unique<FileProperty>(
                      "OutputFilename", "", FileProperty::OptionalSave,
                      StringInitializerList({".nxs"})),
                  "If specified, the output workspace will be a file-backed "
                  "MDEventWorkspace");
  std::vector<std::string> allowed = {"Q_sample", "HKL"};
  declareProperty("Q3DFrames", allowed[0],
                  boost::make_shared<StringListValidator>(allowed),
                  "The required frame for the output workspace");

  // Outputs
  declareProperty(std::make_unique<WorkspaceProperty<IMDEventWorkspace>>(
                      "OutputWorkspace", "", Kernel::Direction::Output),
                  "Output IMDEventWorkspace reflecting SQW data");
}

/// Execute the algorithm.
void LoadSQW2::exec() {
  cacheInputs();
  initFileReader();
  auto sqwType = readMainHeader();
  throwIfUnsupportedFileType(sqwType);
  createOutputWorkspace();
  readAllSPEHeadersToWorkspace();
  skipDetectorSection();
  readDataSection();
  finalize();
}

/// Cache any user input to avoid repeated lookups
void LoadSQW2::cacheInputs() { m_outputFrame = getPropertyValue("Q3DFrames"); }

/// Opens the file given to the algorithm and initializes the reader
void LoadSQW2::initFileReader() {
  using API::Progress;

  m_file = std::make_unique<std::ifstream>(getPropertyValue("Filename"),
                                           std::ios_base::binary);
  m_reader = std::make_unique<BinaryStreamReader>(*m_file);
}

/**
 * Reads the initial header section. Skips specifically the
 * following: app_name, app_version, sqw_type, ndims, filename, filepath,
 * title. Caches the number of contributing files.
 * @return An integer describing the SQW type stored: 0 = DND, 1 = SQW
 */
int32_t LoadSQW2::readMainHeader() {
  std::string appName, filename, filepath, title;
  double appVersion(0.0);
  int32_t sqwType(-1), numDims(-1), nspe(-1);
  *m_reader >> appName >> appVersion >> sqwType >> numDims >> filename >>
      filepath >> title >> nspe;
  m_nspe = static_cast<uint16_t>(nspe);
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
       << "    nfiles: " << m_nspe << "\n";
    g_log.debug(os.str());
  }
  return sqwType;
}

/**
 * Throw std::runtime_error if the sqw type of the file is unsupported
 * @param sqwType 0 = DND, 1 = SQW
 */
void LoadSQW2::throwIfUnsupportedFileType(int32_t sqwType) {
  if (sqwType != 1) {
    throw std::runtime_error(
        "Unsupported SQW type: " + std::to_string(sqwType) +
        "\nOnly files containing the full pixel "
        "information are currently supported");
  }
}

/// Create the output workspace object
void LoadSQW2::createOutputWorkspace() {
  m_outputWS = boost::make_shared<SQWWorkspace>();
}

/**
 * Read all of the SPE headers and fill in the experiment details on the
 * output workspace. It also caches the transformations between the crystal
 * frame & HKL using the same assumption as Horace that the lattice information
 * is the same for each contributing SPE file.
 */
void LoadSQW2::readAllSPEHeadersToWorkspace() {
  for (uint16_t i = 0; i < m_nspe; ++i) {
    auto expt = readSingleSPEHeader();
    m_outputWS->addExperimentInfo(expt);
  }
  auto expt0 = m_outputWS->getExperimentInfo(0);
  cacheFrameTransforms(expt0->sample().getOrientedLattice());
}

/**
 * Read single SPE header from the file. It assumes the file stream
 * points at the start of a header section. It is left pointing at the end of
 * this section
 * @return A new ExperimentInfo object storing the data
 */
boost::shared_ptr<API::ExperimentInfo> LoadSQW2::readSingleSPEHeader() {
  auto experiment = boost::make_shared<ExperimentInfo>();
  auto &sample = experiment->mutableSample();
  auto &run = experiment->mutableRun();

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
  auto lattice = std::make_unique<OrientedLattice>(
      floats[0], floats[1], floats[2], floats[3], floats[4], floats[5]);
  V3D uVec(floats[6], floats[7], floats[8]),
      vVec(floats[9], floats[10], floats[11]);
  lattice->setUFromVectors(uVec, vVec);
  // Lattice is copied into the Sample object
  sample.setOrientedLattice(lattice.get());
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
       << "\n"
       << "B matrix (calculated): " << lattice->getB() << "\n"
       << "Inverse B matrix (calculated): " << lattice->getBinv() << "\n";
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

  return experiment;
}

/**
 * Cache the transforms between the Q_sample & HKL frames from the given lattice
 * @param lattice A reference to the lattice object
 */
void LoadSQW2::cacheFrameTransforms(const Geometry::OrientedLattice &lattice) {
  m_uToRLU = lattice.getBinv() * INV_TWO_PI;
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
  bool metadataOnly = getProperty("MetadataOnly");
  if (!metadataOnly)
    readPixelDataIntoWorkspace();
}

/**
 * Skip metadata in data section.
 * On exit the file pointer will be positioned before
 * the npax entry
 */
void LoadSQW2::skipDataSectionMetadata() {
  std::string dropped;
  *m_reader >> dropped >> dropped >> dropped;
  // skip alatt, angdeg, uoffset, u_to_rlu, ulen
  m_file->seekg(120, std::ios_base::cur);

  // dimension labels
  std::vector<int32_t> ulabelShape(2);
  m_reader->read(ulabelShape, 2);
  m_file->seekg(ulabelShape[0] * ulabelShape[1], std::ios_base::cur);
}

/**
 * Read and create the SQW dimensions on the output. It assumes
 * the file pointer is positioned before npix entry.
 * On exit the file pointer will be positioned after the last
 * urange entry
 */
void LoadSQW2::readSQWDimensions() {
  auto nbins = readProjection();
  if (g_log.is(Logger::Priority::PRIO_DEBUG)) {
    std::stringstream os;
    os << "nbins: (";
    for (const auto &val : nbins) {
      os << val << ",";
    }
    os << ")";
    g_log.debug(os.str());
  }
  auto dimLimits = calculateDimLimitsFromData();
  if (g_log.is(Logger::Priority::PRIO_DEBUG)) {
    std::stringstream os;
    os << "data extents (in output frame): ";
    for (size_t i = 0; i < 4; ++i) {
      os << "(" << dimLimits[2 * i] << "," << dimLimits[2 * i + 1] << ") ";
    }
    os << "\n";
    g_log.debug(os.str());
  }

  // The lattice is assumed to be the same in all contributing files so use
  // the first B matrix to create the axis information (only needed in HKL
  // frame)
  const auto &bmat0 =
      m_outputWS->getExperimentInfo(0)->sample().getOrientedLattice().getB();
  for (size_t i = 0; i < 4; ++i) {
    // To ensure that we capture all of the data from the file we initially
    // set the dimension limits to arbitrarily large values and reset them later
    float umin(dimLimits[2 * i]), umax(dimLimits[2 * i + 1]);
    if (i < 3) {
      m_outputWS->addDimension(createQDimension(
          i, umin, umax, static_cast<size_t>(nbins[i]), bmat0));
    } else {
      m_outputWS->addDimension(
          createEnDimension(umin, umax, static_cast<size_t>(nbins[i])));
    }
  }
  setupBoxController();
}

/**
 * Read the required parts of the projection information from the data section
 * The file pointer is assumed to be positioned after the ulabel entry on
 * entry and will be positioned before the urange entry on exit.
 * @return A vector containing the number of bins for each axis
 */
std::vector<int32_t> LoadSQW2::readProjection() {
  int32_t nProjAxes(0);
  *m_reader >> nProjAxes;
  int32_t nIntAxes(4 - nProjAxes);
  if (nIntAxes > 0) {
    // n indices + 2*n limits
    m_file->seekg(nIntAxes * sizeof(int32_t) + 2 * nIntAxes * sizeof(float),
                  std::ios_base::cur);
  }
  std::vector<int32_t> nbins(4, 1);
  if (nProjAxes > 0) {
    // 1-based indices of the non-integrated axes
    std::vector<int32_t> projAxIdx;
    int32_t signalLength(1);
    m_reader->read(projAxIdx, nProjAxes);
    for (int32_t i = 0; i < nProjAxes; ++i) {
      int32_t nbounds(0);
      *m_reader >> nbounds;
      m_file->seekg(nbounds * sizeof(float), std::ios_base::cur);
      nbins[projAxIdx[i] - 1] = nbounds - 1;
      signalLength *= nbounds - 1;
    }
    // skip display axes
    m_file->seekg(nProjAxes * sizeof(int32_t), std::ios_base::cur);
    // skip data+error+npix(binned)
    m_file->seekg(2 * signalLength * sizeof(float) +
                      signalLength * sizeof(int64_t),
                  std::ios_base::cur);
  }
  return nbins;
}

/**
 * Find the dimension limits for each dimension in the target frame. For the
 * cuts the urange entry does not seem to specify the correct range to
 * encompass all of the data so we manually calculate the limits from the
 * data itself to ensure we don't drop pixels.
 * It assumes that the file pointer is positioned before the first urange
 * entry and on exit it will be placed after the last urange entry
 * @return A vector containing the range for each dimension as
 * min_0,max_0,min_1,max_1...
 */
std::vector<float> LoadSQW2::calculateDimLimitsFromData() {
  // skip urange
  m_file->seekg(8 * sizeof(float), std::ios_base::cur);

  auto filePosAfterURange = m_file->tellg();
  // Redundnant int32 field
  m_file->seekg(sizeof(int32_t), std::ios_base::cur);

  int64_t npixtot(0);
  *m_reader >> npixtot;
  API::Progress status(this, 0.0, 0.5, npixtot);
  status.setNotifyStep(0.01);

  constexpr int64_t bufferSize(FIELDS_PER_PIXEL * NPIX_CHUNK);
  std::vector<float> pixBuffer(bufferSize);
  int64_t pixelsLeftToRead(npixtot);
  std::vector<float> dimLimits(8);
  dimLimits[0] = dimLimits[2] = dimLimits[4] = dimLimits[6] = FLT_MAX;
  dimLimits[1] = dimLimits[3] = dimLimits[5] = dimLimits[7] = -FLT_MAX;
  while (pixelsLeftToRead > 0) {
    int64_t chunkSize(pixelsLeftToRead);
    if (chunkSize > NPIX_CHUNK) {
      chunkSize = NPIX_CHUNK;
    }
    m_reader->read(pixBuffer, FIELDS_PER_PIXEL * chunkSize);
    for (int64_t i = 0; i < chunkSize; ++i) {
      float *pixel = pixBuffer.data() + i * 9;
      toOutputFrame(pixel);
      for (size_t j = 0; j < 4; ++j) {
        auto uj(pixel[j]);
        if (uj < dimLimits[2 * j])
          dimLimits[2 * j] = uj;
        else if (uj > dimLimits[2 * j + 1])
          dimLimits[2 * j + 1] = uj;
      }
      status.report("Calculating data extents");
    }
    pixelsLeftToRead -= chunkSize;
  }
  m_file->seekg(filePosAfterURange);
  return dimLimits;
}

// The missing braces warning is a false positive -
// https://llvm.org/bugs/show_bug.cgi?id=21629
GNU_DIAG_OFF("missing-braces")
/**
 * Create the Q MDHistoDimension for the output frame and given information
 * from the file
 * @param index Index of the dimension
 * @param dimMin Dimension minimum in output frame
 * @param dimMax Dimension maximum in output frame
 * @param nbins Number of bins for this dimension
 * @param bmat A reference to the B matrix to create the axis labels for the
 * HKL frame
 * @return A new MDHistoDimension object
 */
Geometry::IMDDimension_sptr
LoadSQW2::createQDimension(size_t index, float dimMin, float dimMax,
                           size_t nbins, const Kernel::DblMatrix &bmat) {
  if (index > 2) {
    throw std::logic_error("LoadSQW2::createQDimension - Expected a dimension "
                           "index between 0 & 2. Found: " +
                           std::to_string(index));
  }
  static std::array<const char *, 3> indexToDim{"x", "y", "z"};
  MDHistoDimensionBuilder builder;
  builder.setId(std::string("q") + indexToDim[index]);
  MDHistoDimensionBuilder::resizeToFitMDBox(dimMin, dimMax);
  builder.setMin(dimMin);
  builder.setMax(dimMax);
  builder.setNumBins(nbins);

  std::string name, unit, frameName;
  if (m_outputFrame == "Q_sample") {
    name = m_outputFrame + "_" + indexToDim[index];
    unit = "A^-1";
    frameName = "QSample";
  } else if (m_outputFrame == "HKL") {
    static std::array<const char *, 3> indexToHKL{"[H,0,0]", "[0,K,0]",
                                                  "[0,0,L]"};
    name = indexToHKL[index];
    V3D dimDir;
    dimDir[index] = 1;
    const V3D x = bmat * dimDir;
    double length = 2. * M_PI * x.norm();
    unit = "in " + MDAlgorithms::sprintfd(length, 1.e-3) + " A^-1";
    frameName = "HKL";
  } else {
    throw std::logic_error(
        "LoadSQW2::createQDimension - Unknown output frame: " + m_outputFrame);
  }
  builder.setUnits(unit);
  builder.setName(name);
  builder.setFrameName(frameName);

  return builder.create();
}

GNU_DIAG_ON("missing-braces")

/**
 * Create an energy dimension
 * @param dimMin Dimension minimum in output frame
 * @param dimMax Dimension maximum in output frame
 * @param nbins Number of bins for this dimension
 * @return A new MDHistoDimension object
 */
Geometry::IMDDimension_sptr
LoadSQW2::createEnDimension(float dimMin, float dimMax, size_t nbins) {
  MDHistoDimensionBuilder builder;
  builder.setId("en");
  builder.setUnits("meV");
  builder.setName("en");
  builder.setFrameName("meV");
  MDHistoDimensionBuilder::resizeToFitMDBox(dimMin, dimMax);
  builder.setMin(dimMin);
  builder.setMax(dimMax);
  builder.setNumBins(nbins);
  return builder.create();
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

  std::string fileback = getProperty("OutputFilename");
  if (!fileback.empty()) {
    setupFileBackend(fileback);
  }
}

/**
 * Setup the filebackend for the output workspace. It assumes that the
 * box controller has already been initialized
 * @param filebackPath Path to the file used for backend storage
 */
void LoadSQW2::setupFileBackend(std::string filebackPath) {
  using DataObjects::BoxControllerNeXusIO;
  auto savemd = this->createChildAlgorithm("SaveMD", 0.01, 0.05, true);
  savemd->setProperty("InputWorkspace", m_outputWS);
  savemd->setPropertyValue("Filename", filebackPath);
  savemd->setProperty("UpdateFileBackEnd", false);
  savemd->setProperty("MakeFileBacked", false);
  savemd->executeAsChildAlg();

  // create file-backed box controller
  auto boxControllerMem = m_outputWS->getBoxController();
  auto boxControllerIO =
      boost::make_shared<BoxControllerNeXusIO>(boxControllerMem.get());
  boxControllerMem->setFileBacked(boxControllerIO, filebackPath);
  m_outputWS->getBox()->setFileBacked();
  boxControllerMem->getFileIO()->setWriteBufferSize(1000000);
}

/**
 * Read the pixel data into the workspace
 */
void LoadSQW2::readPixelDataIntoWorkspace() {
  using Kernel::Timer;
  Timer timer;

  // skip redundant field
  m_file->seekg(sizeof(int32_t), std::ios_base::cur);
  int64_t npixtot(0);
  *m_reader >> npixtot;
  g_log.debug() << "    npixtot: " << npixtot << "\n";
  warnIfMemoryInsufficient(npixtot);
  API::Progress status(this, 0.5, 1.0, npixtot);
  status.setNotifyStep(0.01);

  // Each pixel has 9 float fields. Do a chunked read to avoid
  // using too much memory for the buffer and also split the
  // boxes regularly to ensure that larger workspaces can be loaded
  // without blowing the memory requirements.
  constexpr int64_t bufferSize(FIELDS_PER_PIXEL * NPIX_CHUNK);
  std::vector<float> pixBuffer(bufferSize);
  int64_t pixelsLeftToRead(npixtot), chunksRead(0);
  size_t pixelsAdded(0);
  while (pixelsLeftToRead > 0) {
    int64_t chunkSize(pixelsLeftToRead);
    if (chunkSize > NPIX_CHUNK) {
      chunkSize = NPIX_CHUNK;
    }
    m_reader->read(pixBuffer, FIELDS_PER_PIXEL * chunkSize);
    for (int64_t i = 0; i < chunkSize; ++i) {
      pixelsAdded += addEventFromBuffer(pixBuffer.data() + i * 9);
      status.report("Reading pixel data to workspace");
    }
    pixelsLeftToRead -= chunkSize;
    ++chunksRead;
    if ((chunksRead % NCHUNKS_SPLIT) == 0) {
      splitAllBoxes();
    }
  }
  assert(pixelsLeftToRead == 0);
  if (pixelsAdded == 0) {
    throw std::runtime_error(
        "No pixels could be added from the source file. "
        "Please check the irun fields of all pixels are valid.");
  } else if (pixelsAdded != static_cast<size_t>(npixtot)) {
    g_log.warning("Some pixels within the source file had an invalid irun "
                  "field. They have been ignored.");
  }

  g_log.debug() << "Time to read all pixels: " << timer.elapsed() << "s\n";
}

/**
 * Split boxes in the output workspace if required
 */
void LoadSQW2::splitAllBoxes() {
  using Kernel::ThreadPool;
  using Kernel::ThreadSchedulerFIFO;
  auto *ts = new ThreadSchedulerFIFO();
  ThreadPool tp(ts);
  m_outputWS->splitAllIfNeeded(ts);
  tp.joinAll();
}

/**
 * If the output is not file backed and the machine appears to have
 * insufficient
 * memory to read the data in total then warn the user. We don't stop
 * the algorithm just in case our memory calculation is wrong.
 * @param npixtot The total number of pixels to be read
 */
void LoadSQW2::warnIfMemoryInsufficient(int64_t npixtot) {
  using DataObjects::MDEvent;
  using Kernel::MemoryStats;
  if (m_outputWS->isFileBacked())
    return;
  MemoryStats stat;
  size_t reqdMemory =
      (npixtot * sizeof(MDEvent<4>) + NPIX_CHUNK * FIELDS_PER_PIXEL) / 1024;
  if (reqdMemory > stat.availMem()) {
    g_log.warning()
        << "It looks as if there is insufficient memory to load the "
        << "entire file. It is recommended to cancel the algorithm and "
           "specify "
           "the OutputFilename option to create a file-backed workspace.\n";
  }
}

/**
 * Assume the given pointer points to the start of a full pixel and create
 * an MDEvent based on it iff it has a valid run id.
 * @param pixel A pointer assumed to point to at the start of a single pixel
 * from the data file
 * @return 1 if the event was added, 0 otherwise
 */
size_t LoadSQW2::addEventFromBuffer(const float *pixel) {
  using DataObjects::MDEvent;
  // Is the pixel field valid? Older versions of Horace produced files with
  // an invalid field and we can't use this. It should be between 1 && nfiles
  uint16_t irun = static_cast<uint16_t>(pixel[4]);
  if (irun < 1 || irun > m_nspe) {
    return 0;
  }
  coord_t centers[4] = {pixel[0], pixel[1], pixel[2], pixel[3]};
  toOutputFrame(centers);
  auto error = pixel[8];
  auto added = m_outputWS->addEvent(
      MDEvent<4>(pixel[7], error * error, static_cast<uint16_t>(irun - 1),
                 static_cast<detid_t>(pixel[5]), centers));
  // At this point the workspace should be setup so that we always add the
  // event so only do a runtime check in debug mode
  assert(added == 1);
  return added;
}

/**
 * Transform the given coordinates to the requested output frame if necessary.
 * The assumption is that the pixels on input are in the Q_sample (crystal)
 * frame as they are defined in Horace
 * @param centers Coordinates assumed to be in the crystal cartesian frame.
 * The array should be atleast 3 in size
 */
void LoadSQW2::toOutputFrame(coord_t *centers) {
  if (m_outputFrame == "Q_sample")
    return;
  V3D qout = m_uToRLU * V3D(centers[0], centers[1], centers[2]);
  centers[0] = static_cast<float>(qout[0]);
  centers[1] = static_cast<float>(qout[1]);
  centers[2] = static_cast<float>(qout[2]);
}

/**
 * Assumed to be the last step in the algorithm. Performs any steps
 * necessary after everything else has run successfully
 */
void LoadSQW2::finalize() {
  splitAllBoxes();
  m_outputWS->refreshCache();
  if (m_outputWS->isFileBacked()) {
    auto savemd = this->createChildAlgorithm("SaveMD", 0.76, 1.00);
    savemd->setProperty("InputWorkspace", m_outputWS);
    savemd->setProperty("UpdateFileBackEnd", true);
    savemd->executeAsChildAlg();
  }
  setProperty("OutputWorkspace", m_outputWS);
}

} // namespace MDAlgorithms
} // namespace Mantid
