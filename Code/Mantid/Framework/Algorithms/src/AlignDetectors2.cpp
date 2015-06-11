//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/AlignDetectors2.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/UnitFactory.h"
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
DECLARE_ALGORITHM(AlignDetectors2)

namespace { // anonymous namespace

    /// Applies the equation d=(TOF-tzero)/difc
    struct difc_only {
      difc_only(const double difc) : factor(1./difc){
      }

      double operator()(const double tof) const {
        return factor*tof;
      }

      /// 1./difc
      double factor;
    };

    /// Applies the equation d=(TOF-tzero)/difc
    struct difc_and_tzero {
      difc_and_tzero(const double difc, const double tzero) : factor(1./difc), offset(-1.*tzero/difc){
      }

      double operator()(const double tof) const {
        return factor*tof+offset;
      }

      /// 1./difc
      double factor;
      /// -tzero/difc
      double offset;
    };

    struct difa_positive {
      difa_positive(const double difc, const double difa, const double tzero) {
          factor1 = -0.5 * difc / difa;
          factor2 = 1. / difa;
          factor3 = (factor1 * factor1) - (tzero / difa);
      }

      double operator()(const double tof) const {
        return factor1 + std::sqrt((tof*factor2) + factor3);
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
        ConversionFactors(ITableWorkspace_const_sptr table) {
            m_difcCol = table->getColumn("difc");
            m_difaCol = table->getColumn("difa");
            m_tzeroCol = table->getColumn("tzero");

            this->generateDetidToRow(table);
        }

        std::function<double(double)> getConversionFunc(const std::set<detid_t> & detIds) {
            const std::set<size_t> rows = this->getRow(detIds);
            double difc = 0.;
            double difa = 0.;
            double tzero = 0.;
            for (auto row = rows.begin(); row != rows.end(); ++row) {
                difc += m_difcCol->toDouble(*row);
                difa += m_difaCol->toDouble(*row);
                tzero += m_tzeroCol->toDouble(*row);
            }
            if (rows.size() > 1) {
                double norm = 1./static_cast<double>(rows.size());
                difc = norm * difc;
                difa = norm * difa;
                tzero = norm * tzero;
            }

            if (difa == 0.) {
                if (tzero == 0.) {
                    return difc_only(difc);
                } else {
                    return difc_and_tzero(difc, tzero);
                }
            } else if (difa > 0.){
                return difa_positive(difc, difa, tzero);
            } else {
                throw std::runtime_error("difa != 0 branch needs to be written"); // TODO
            }
        }

    private:
        void generateDetidToRow(ITableWorkspace_const_sptr table) {
            ConstColumnVector<int> detIDs = table->getVector("detid");
            const size_t numDets = detIDs.size();
            for (size_t i=0; i<numDets; ++i) {
                m_detidToRow[static_cast<detid_t>(detIDs[i])]=i;
            }
        }

        std::set<size_t> getRow(const std::set<detid_t> & detIds) {
            std::set<size_t> rows;
            for (auto detId = detIds.begin(); detId != detIds.end(); ++detId) {
                auto rowIter = m_detidToRow.find(*detId);
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

const std::string AlignDetectors2::name() const { return "AlignDetectors"; }

int AlignDetectors2::version() const { return 2; }

const std::string AlignDetectors2::category() const { return "Diffraction"; }

const std::string AlignDetectors2::summary() const {
  return "Performs a unit change from TOF to dSpacing, correcting the X "
         "values to account for small errors in the detector positions.";
}

/// (Empty) Constructor
AlignDetectors2::AlignDetectors2() { this->tofToDmap = NULL; }

/// Destructor
AlignDetectors2::~AlignDetectors2() { delete this->tofToDmap; }

//-----------------------------------------------------------------------
void AlignDetectors2::init() {
  auto wsValidator = boost::make_shared<CompositeValidator>();
  // Workspace unit must be TOF.
  wsValidator->add<WorkspaceUnitValidator>("TOF");
  wsValidator->add<RawCountValidator>();

  declareProperty(new WorkspaceProperty<API::MatrixWorkspace>(
                      "InputWorkspace", "", Direction::Input, wsValidator),
                  "A workspace with units of TOF");

  declareProperty(new WorkspaceProperty<API::MatrixWorkspace>(
                      "OutputWorkspace", "", Direction::Output),
                  "The name to use for the output workspace");

  std::vector<std::string> exts;
  // exts.push_back(".hd5"); // TODO
  exts.push_back(".cal");
  declareProperty(
      new FileProperty("CalibrationFile", "", FileProperty::OptionalLoad, exts),
      "Optional: The .cal file containing the position correction factors. "
      "Either this or OffsetsWorkspace needs to be specified.");

  declareProperty(
      new WorkspaceProperty<ITableWorkspace>(
          "CalibrationWorkspace", "", Direction::Input, PropertyMode::Optional),
      "Optional: A Workspace containing the calibration information. Either "
      "this or CalibrationFile needs to be specified.");
}

std::map<std::string, std::string> AlignDetectors2::validateInputs() {
    std::map<std::string, std::string> result;

    const std::string calFileName = getProperty("CalibrationFile");
    bool haveCalFile = (!calFileName.empty());

    ITableWorkspace_const_sptr calibrationWS = getProperty("CalibrationWorkspace");
    bool haveCalWksp = bool(calibrationWS);

    std::string message;
    if (haveCalFile && haveCalWksp) {
        message = "You must specify either CalibrationFile or "
                  "CalibrationWorkspace but not both.";
    } else if ((!haveCalFile) && (!haveCalWksp)) {
        message = "You must specify either CalibrationFile or CalibrationWorkspace.";
    }

    if (!message.empty()) {
        result["CalibrationFile"] = message;
        result["CalibrationWorkspace"] = message;
    }

    return result;
}

bool endswith(const std::string& str, const std::string &ending) {
    if (ending.size() > str.size()) {
      return false;
    }

    return std::equal(str.begin() + str.size() - ending.size(),
                      str.end(), ending.begin());
}

void AlignDetectors2::loadCalFile(MatrixWorkspace_sptr inputWS, const std::string & filename) {
  if (endswith(filename, ".cal")) {
      // Load the .cal file
      IAlgorithm_sptr alg = createChildAlgorithm("LoadCalFile");
      alg->setPropertyValue("CalFilename", filename);
      alg->setProperty("InputWorkspace", inputWS);
      alg->setProperty<bool>("MakeGroupingWorkspace", false);
      alg->setProperty<bool>("MakeOffsetsWorkspace", true);
      alg->setProperty<bool>("MakeMaskWorkspace", false);
      alg->setPropertyValue("WorkspaceName", "temp");
      alg->executeAsChildAlg();
      m_calibrationWS = alg->getProperty("OutputCalWorkspace");
//  } else if (endswith(filename, ".hd5")) { // TODO

  } else {
      std::stringstream msg;
      msg << "Do not know how to load cal file: " << filename;
      throw std::runtime_error(msg.str());
  }
}

void setXAxisUnits(API::MatrixWorkspace_sptr outputWS) {
    outputWS->getAxis(0)->unit() = UnitFactory::Instance().create("dSpacing");
}

//-----------------------------------------------------------------------
/** Executes the algorithm
 *  @throw Exception::FileError If the calibration file cannot be opened and
 * read successfully
 *  @throw Exception::InstrumentDefinitionError If unable to obtain the
 * source-sample distance
 */
void AlignDetectors2::exec() {
  // Get the input workspace
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");

  const std::string calFileName = getPropertyValue("CalibrationFile");
  if (!calFileName.empty()) {
    progress(0.0, "Reading calibration file");
    loadCalFile(inputWS, calFileName);
  } else {
    m_calibrationWS = getProperty("CalibrationWorkspace");
  }

  // Initialise the progress reporting object
  m_numberOfSpectra = static_cast<int64_t>(inputWS->getNumberHistograms());

  // Check if its an event workspace
  EventWorkspace_const_sptr eventW =
      boost::dynamic_pointer_cast<const EventWorkspace>(inputWS);
  if (eventW != NULL) {
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

       std::transform( xIn.begin(), xIn.end(), xOut.begin(), toDspacing);
//       std::bind2nd(std::multiplies<double>(), factor) );
      // the above transform creates wrong output in parallel in debug in Visual
      // Studio
//      for (size_t k = 0; k < xOut.size(); ++k) {
//        xOut[k] = xIn[k] * factor;
//      }

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

//-----------------------------------------------------------------------
/**
 * Execute the align detectors algorithm for an event workspace.
 */
void AlignDetectors2::execEvent() {
  // g_log.information("Processing event workspace");

  // the calibration information is already read in at this point

  // convert the input workspace into the event workspace we already know it is
  const MatrixWorkspace_const_sptr matrixInputWS =
      this->getProperty("InputWorkspace");
  EventWorkspace_const_sptr inputWS =
      boost::dynamic_pointer_cast<const EventWorkspace>(matrixInputWS);

  // generate the output workspace pointer
  API::MatrixWorkspace_sptr matrixOutputWS =
      this->getProperty("OutputWorkspace");
  EventWorkspace_sptr outputWS;
  if (matrixOutputWS == matrixInputWS)
    outputWS = boost::dynamic_pointer_cast<EventWorkspace>(matrixOutputWS);
  else {
    // Make a brand new EventWorkspace
    outputWS = boost::dynamic_pointer_cast<EventWorkspace>(
        API::WorkspaceFactory::Instance().create(
            "EventWorkspace", inputWS->getNumberHistograms(), 2, 1));
    // Copy geometry over.
    API::WorkspaceFactory::Instance().initializeFromParent(inputWS, outputWS,
                                                           false);
    // You need to copy over the data as well.
    outputWS->copyDataFrom((*inputWS));

    // Cast to the matrixOutputWS and save it
    matrixOutputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(outputWS);
    this->setProperty("OutputWorkspace", matrixOutputWS);
  }

  // Set the final unit that our output workspace will have
  setXAxisUnits(outputWS);

  ConversionFactors converter = ConversionFactors(m_calibrationWS);

  Progress progress(this, 0.0, 1.0, m_numberOfSpectra);

  PARALLEL_FOR_NO_WSP_CHECK()
  for (int64_t i = 0; i < m_numberOfSpectra; ++i) {
    PARALLEL_START_INTERUPT_REGION

    auto toDspacing = converter.getConversionFunc(inputWS->getSpectrum(size_t(i))->getDetectorIDs());
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
    throw std::runtime_error(msg.str());
  }
  outputWS->clearMRU();
}

} // namespace Algorithms
} // namespace Mantid
