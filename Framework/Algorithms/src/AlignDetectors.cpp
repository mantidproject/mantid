//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/AlignDetectors.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/RawCountValidator.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/V3D.h"

#include <fstream>
#include <sstream>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;
using Mantid::DataObjects::OffsetsWorkspace;

namespace Mantid {
namespace Algorithms {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(AlignDetectors)

namespace { // anonymous namespace

/// Applies the equation d=(TOF-tzero)/difc
struct func_difc_only {
  explicit func_difc_only(const double difc) : factor(1. / difc) {}

  double operator()(const double tof) const { return factor * tof; }

  /// 1./difc
  double factor;
};

/// Applies the equation d=(TOF-tzero)/difc
struct func_difc_and_tzero {
  func_difc_and_tzero(const double difc, const double tzero)
      : factor(1. / difc), offset(-1. * tzero / difc) {}

  double operator()(const double tof) const { return factor * tof + offset; }

  /// 1./difc
  double factor;
  /// -tzero/difc
  double offset;
};

struct func_difa {
  func_difa(const double difc, const double difa, const double tzero) {
    factor1 = -0.5 * difc / difa;
    factor2 = 1. / difa;
    factor3 = (factor1 * factor1) - (tzero / difa);
  }

  double operator()(const double tof) const {
    double second = std::sqrt((tof * factor2) + factor3);
    if (second < factor1)
      return factor1 - second;
    else {
      return factor1 + second;
    }
  }

  /// -0.5*difc/difa
  double factor1;
  /// 1/difa
  double factor2;
  /// (0.5*difc/difa)^2 - (tzero/difa)
  double factor3;
};

class ConversionFactors {
public:
  explicit ConversionFactors(ITableWorkspace_const_sptr table) {
    m_difcCol = table->getColumn("difc");
    m_difaCol = table->getColumn("difa");
    m_tzeroCol = table->getColumn("tzero");

    this->generateDetidToRow(table);
  }

  std::function<double(double)>
  getConversionFunc(const std::set<detid_t> &detIds) {
    const std::set<size_t> rows = this->getRow(detIds);
    double difc = 0.;
    double difa = 0.;
    double tzero = 0.;
    for (auto row : rows) {
      difc += m_difcCol->toDouble(row);
      difa += m_difaCol->toDouble(row);
      tzero += m_tzeroCol->toDouble(row);
    }
    if (rows.size() > 1) {
      double norm = 1. / static_cast<double>(rows.size());
      difc = norm * difc;
      difa = norm * difa;
      tzero = norm * tzero;
    }

    if (difa == 0.) {
      if (tzero == 0.) {
        return func_difc_only(difc);
      } else {
        return func_difc_and_tzero(difc, tzero);
      }
    } else { // difa != 0.
      return func_difa(difc, difa, tzero);
    }
  }

private:
  void generateDetidToRow(ITableWorkspace_const_sptr table) {
    ConstColumnVector<int> detIDs = table->getVector("detid");
    const size_t numDets = detIDs.size();
    for (size_t i = 0; i < numDets; ++i) {
      m_detidToRow[static_cast<detid_t>(detIDs[i])] = i;
    }
  }

  std::set<size_t> getRow(const std::set<detid_t> &detIds) {
    std::set<size_t> rows;
    for (auto detId : detIds) {
      auto rowIter = m_detidToRow.find(detId);
      if (rowIter != m_detidToRow.end()) { // skip if not found
        rows.insert(rowIter->second);
      }
    }
    return rows;
  }

  std::map<detid_t, size_t> m_detidToRow;
  Column_const_sptr m_difcCol;
  Column_const_sptr m_difaCol;
  Column_const_sptr m_tzeroCol;
};
} // anonymous namespace

const std::string AlignDetectors::name() const { return "AlignDetectors"; }

int AlignDetectors::version() const { return 1; }

const std::string AlignDetectors::category() const {
  return "Diffraction\\Calibration";
}

const std::string AlignDetectors::summary() const {
  return "Performs a unit change from TOF to dSpacing, correcting the X "
         "values to account for small errors in the detector positions.";
}

/// (Empty) Constructor
AlignDetectors::AlignDetectors() : m_numberOfSpectra(0) {
  this->tofToDmap = nullptr;
}

/// Destructor
AlignDetectors::~AlignDetectors() { delete this->tofToDmap; }

void AlignDetectors::init() {
  auto wsValidator = boost::make_shared<CompositeValidator>();
  // Workspace unit must be TOF.
  wsValidator->add<WorkspaceUnitValidator>("TOF");
  wsValidator->add<RawCountValidator>();

  declareProperty(make_unique<WorkspaceProperty<API::MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input, wsValidator),
                  "A workspace with units of TOF");

  declareProperty(make_unique<WorkspaceProperty<API::MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "The name to use for the output workspace");

  const std::vector<std::string> exts{".h5", ".hd5", ".hdf", ".cal"};
  declareProperty(
      Kernel::make_unique<FileProperty>("CalibrationFile", "",
                                        FileProperty::OptionalLoad, exts),
      "Optional: The .cal file containing the position correction factors. "
      "Either this or OffsetsWorkspace needs to be specified.");

  declareProperty(
      make_unique<WorkspaceProperty<ITableWorkspace>>(
          "CalibrationWorkspace", "", Direction::Input, PropertyMode::Optional),
      "Optional: A Workspace containing the calibration information. Either "
      "this or CalibrationFile needs to be specified.");

  declareProperty(
      make_unique<WorkspaceProperty<OffsetsWorkspace>>(
          "OffsetsWorkspace", "", Direction::Input, PropertyMode::Optional),
      "Optional: A OffsetsWorkspace containing the calibration offsets. Either "
      "this or CalibrationFile needs to be specified.");

  // make group associations.
  std::string calibrationGroup("Calibration");
  setPropertyGroup("CalibrationFile", calibrationGroup);
  setPropertyGroup("CalibrationWorkspace", calibrationGroup);
  setPropertyGroup("OffsetsWorkspace", calibrationGroup);
}

std::map<std::string, std::string> AlignDetectors::validateInputs() {
  std::map<std::string, std::string> result;

  int numWays = 0;

  const std::string calFileName = getProperty("CalibrationFile");
  if (!calFileName.empty())
    numWays += 1;

  ITableWorkspace_const_sptr calibrationWS =
      getProperty("CalibrationWorkspace");
  if (bool(calibrationWS))
    numWays += 1;

  OffsetsWorkspace_const_sptr offsetsWS = getProperty("OffsetsWorkspace");
  if (bool(offsetsWS))
    numWays += 1;

  std::string message;
  if (numWays == 0) {
    message = "You must specify only one of CalibrationFile, "
              "CalibrationWorkspace, OffsetsWorkspace.";
  }
  if (numWays > 1) {
    message = "You must specify one of CalibrationFile, "
              "CalibrationWorkspace, OffsetsWorkspace.";
  }

  if (!message.empty()) {
    result["CalibrationFile"] = message;
    result["CalibrationWorkspace"] = message;
  }

  return result;
}

void AlignDetectors::loadCalFile(MatrixWorkspace_sptr inputWS,
                                 const std::string &filename) {
  IAlgorithm_sptr alg = createChildAlgorithm("LoadDiffCal");
  alg->setProperty("InputWorkspace", inputWS);
  alg->setPropertyValue("Filename", filename);
  alg->setProperty<bool>("MakeCalWorkspace", true);
  alg->setProperty<bool>("MakeGroupingWorkspace", false);
  alg->setProperty<bool>("MakeMaskWorkspace", false);
  alg->setPropertyValue("WorkspaceName", "temp");
  alg->executeAsChildAlg();

  m_calibrationWS = alg->getProperty("OutputCalWorkspace");
}

void AlignDetectors::getCalibrationWS(MatrixWorkspace_sptr inputWS) {
  m_calibrationWS = getProperty("CalibrationWorkspace");
  if (m_calibrationWS)
    return; // nothing more to do

  OffsetsWorkspace_sptr offsetsWS = getProperty("OffsetsWorkspace");
  if (offsetsWS) {
    auto alg = createChildAlgorithm("ConvertDiffCal");
    alg->setProperty("OffsetsWorkspace", offsetsWS);
    alg->executeAsChildAlg();
    m_calibrationWS = alg->getProperty("OutputWorkspace");
    m_calibrationWS->setTitle(offsetsWS->getTitle());
    return;
  }

  const std::string calFileName = getPropertyValue("CalibrationFile");
  if (!calFileName.empty()) {
    progress(0.0, "Reading calibration file");
    loadCalFile(inputWS, calFileName);
    return;
  }

  throw std::runtime_error("Failed to determine calibration information");
}

void setXAxisUnits(API::MatrixWorkspace_sptr outputWS) {
  outputWS->getAxis(0)->unit() = UnitFactory::Instance().create("dSpacing");
}

/** Executes the algorithm
 *  @throw Exception::FileError If the calibration file cannot be opened and
 * read successfully
 *  @throw Exception::InstrumentDefinitionError If unable to obtain the
 * source-sample distance
 */
void AlignDetectors::exec() {
  // Get the input workspace
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");

  this->getCalibrationWS(inputWS);

  // Initialise the progress reporting object
  m_numberOfSpectra = static_cast<int64_t>(inputWS->getNumberHistograms());

  // Check if its an event workspace
  EventWorkspace_const_sptr eventW =
      boost::dynamic_pointer_cast<const EventWorkspace>(inputWS);
  if (eventW != nullptr) {
    this->execEvent();
    return;
  }

  API::MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  // If input and output workspaces are not the same, create a new workspace for
  // the output
  if (outputWS != inputWS) {
    outputWS = WorkspaceFactory::Instance().create(inputWS);
    setProperty("OutputWorkspace", outputWS);
  }

  // Set the final unit that our output workspace will have
  setXAxisUnits(outputWS);

  ConversionFactors converter = ConversionFactors(m_calibrationWS);

  Progress progress(this, 0.0, 1.0, m_numberOfSpectra);

  // Loop over the histograms (detector spectra)
  PARALLEL_FOR2(inputWS, outputWS)
  for (int64_t i = 0; i < m_numberOfSpectra; ++i) {
    PARALLEL_START_INTERUPT_REGION
    try {
      // Get the input spectrum number at this workspace index
      auto inSpec = inputWS->getSpectrum(size_t(i));
      auto toDspacing = converter.getConversionFunc(inSpec->getDetectorIDs());

      // Get references to the x data
      MantidVec &xOut = outputWS->dataX(i);

      // Make sure reference to input X vector is obtained after output one
      // because in the case
      // where the input & output workspaces are the same, it might move if the
      // vectors were shared.
      const MantidVec &xIn = inSpec->readX();

      std::transform(xIn.begin(), xIn.end(), xOut.begin(), toDspacing);

      // Copy the Y&E data
      outputWS->dataY(i) = inSpec->readY();
      outputWS->dataE(i) = inSpec->readE();

    } catch (Exception::NotFoundError &) {
      // Zero the data in this case
      outputWS->dataX(i).assign(outputWS->readX(i).size(), 0.0);
      outputWS->dataY(i).assign(outputWS->readY(i).size(), 0.0);
      outputWS->dataE(i).assign(outputWS->readE(i).size(), 0.0);
    }
    progress.report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
}

/**
 * Execute the align detectors algorithm for an event workspace.
 */
void AlignDetectors::execEvent() {
  // the calibration information is already read in at this point

  const MatrixWorkspace_const_sptr matrixInputWS =
      getProperty("InputWorkspace");

  // generate the output workspace pointer
  API::MatrixWorkspace_sptr matrixOutputWS = getProperty("OutputWorkspace");
  if (matrixOutputWS != matrixInputWS) {
    matrixOutputWS = matrixInputWS->clone();
    this->setProperty("OutputWorkspace", matrixOutputWS);
  }
  auto outputWS = boost::dynamic_pointer_cast<EventWorkspace>(matrixOutputWS);

  // Set the final unit that our output workspace will have
  setXAxisUnits(outputWS);

  ConversionFactors converter = ConversionFactors(m_calibrationWS);

  Progress progress(this, 0.0, 1.0, m_numberOfSpectra);

  PARALLEL_FOR_NO_WSP_CHECK()
  for (int64_t i = 0; i < m_numberOfSpectra; ++i) {
    PARALLEL_START_INTERUPT_REGION

    auto toDspacing = converter.getConversionFunc(
        outputWS->getSpectrum(size_t(i))->getDetectorIDs());
    outputWS->getEventList(i).convertTof(toDspacing);

    progress.report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  if (outputWS->getTofMin() < 0.) {
    std::stringstream msg;
    msg << "Something wrong with the calibration. Negative minimum d-spacing "
           "created. d_min = " << outputWS->getTofMin() << " d_max "
        << outputWS->getTofMax();
    g_log.warning(msg.str());
  }
  outputWS->clearMRU();
}

} // namespace Algorithms
} // namespace Mantid
