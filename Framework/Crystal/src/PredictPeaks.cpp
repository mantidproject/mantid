#include "MantidCrystal/PredictPeaks.h"
#include "MantidGeometry/Objects/InstrumentRayTracer.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidAPI/IMDEventWorkspace.h"

#include "MantidGeometry/Crystal/HKLGenerator.h"
#include "MantidGeometry/Crystal/BasicHKLFilters.h"
#include "MantidGeometry/Crystal/HKLFilterWavelength.h"

using Mantid::Kernel::EnabledWhenProperty;

namespace Mantid {
namespace Crystal {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(PredictPeaks)

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

//----------------------------------------------------------------------------------------------
/** Constructor
 */
PredictPeaks::PredictPeaks()
    : m_runNumber(-1), m_wlMin(0.), m_wlMax(0.), m_inst(), m_pw(),
      m_numInRange(), m_crystal(), m_minD(0.), m_maxD(0.), m_mat(), m_gonio() {
  m_refConds = getAllReflectionConditions();
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
PredictPeaks::~PredictPeaks() {}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void PredictPeaks::init() {
  declareProperty(
      new WorkspaceProperty<Workspace>("InputWorkspace", "", Direction::Input),
      "An input workspace (MatrixWorkspace, MDEventWorkspace, or "
      "PeaksWorkspace) containing:\n"
      "  - The relevant Instrument (calibrated as needed).\n"
      "  - A sample with a UB matrix.\n"
      "  - The goniometer rotation matrix.");

  declareProperty(
      new PropertyWithValue<double>("WavelengthMin", 0.1, Direction::Input),
      "Minimum wavelength limit at which to start looking for single-crystal "
      "peaks.");
  declareProperty(
      new PropertyWithValue<double>("WavelengthMax", 100.0, Direction::Input),
      "Maximum wavelength limit at which to stop looking for single-crystal "
      "peaks.");

  declareProperty(
      new PropertyWithValue<double>("MinDSpacing", 1.0, Direction::Input),
      "Minimum d-spacing of peaks to consider. Default = 1.0");
  declareProperty(
      new PropertyWithValue<double>("MaxDSpacing", 100.0, Direction::Input),
      "Maximum d-spacing of peaks to consider.");

  // Build up a list of reflection conditions to use
  std::vector<std::string> propOptions;
  for (size_t i = 0; i < m_refConds.size(); ++i)
    propOptions.push_back(m_refConds[i]->getName());
  declareProperty("ReflectionCondition", "Primitive",
                  boost::make_shared<StringListValidator>(propOptions),
                  "Which reflection condition applies to this crystal, "
                  "reducing the number of expected HKL peaks?");

  declareProperty(new WorkspaceProperty<PeaksWorkspace>("HKLPeaksWorkspace", "",
                                                        Direction::Input,
                                                        PropertyMode::Optional),
                  "Optional: An input PeaksWorkspace with the HKL of the peaks "
                  "that we should predict. \n"
                  "The WavelengthMin/Max and Min/MaxDSpacing parameters are "
                  "unused if this is specified.");

  declareProperty("RoundHKL", true,
                  "When using HKLPeaksWorkspace, this will round the HKL "
                  "values in the HKLPeaksWorkspace to the nearest integers if "
                  "checked.\n"
                  "Keep unchecked to use the original values");
  setPropertySettings(
      "RoundHKL", new EnabledWhenProperty("HKLPeaksWorkspace", IS_NOT_DEFAULT));

  // Disable some props when using HKLPeaksWorkspace
  IPropertySettings *set =
      new EnabledWhenProperty("HKLPeaksWorkspace", IS_DEFAULT);
  setPropertySettings("WavelengthMin", set);
  setPropertySettings("WavelengthMax", set->clone());
  setPropertySettings("MinDSpacing", set->clone());
  setPropertySettings("MaxDSpacing", set->clone());
  setPropertySettings("ReflectionCondition", set->clone());

  declareProperty(new WorkspaceProperty<PeaksWorkspace>("OutputWorkspace", "",
                                                        Direction::Output),
                  "An output PeaksWorkspace.");
}

/** Calculate the prediction for this HKL. Thread-safe.
 *
 * @param h
 * @param k
 * @param l
 * @param doFilter if true, skip unacceptable d-spacings
 */
void PredictPeaks::doHKL(const V3D &hkl) {
  // The q-vector direction of the peak is = goniometer * ub * hkl_vector
  // This is in inelastic convention: momentum transfer of the LATTICE!
  // Also, q does have a 2pi factor = it is equal to 2pi/wavelength.
  V3D q = m_mat * hkl * (2.0 * M_PI);

  // Create the peak using the Q in the lab framewith all its info:
  Peak p(m_inst, q, boost::optional<double>());

  /* The constructor calls setQLabFrame, which already calls findDetector, which
     is expensive. It's not necessary to call it again, instead it's enough to
     check whether a detector has already been set. */
  if (p.getDetector()) {
    // Only add peaks that hit the detector
    p.setGoniometerMatrix(m_gonio);
    // Save the run number found before.
    p.setRunNumber(m_runNumber);
    p.setHKL(hkl);

    // Add it to the workspace
    m_pw->addPeak(p);
  } // Detector was found
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void PredictPeaks::exec() {
  // Get the input properties
  Workspace_sptr inBareWS = getProperty("InputWorkspace");

  ExperimentInfo_sptr inWS =
      boost::dynamic_pointer_cast<ExperimentInfo>(inBareWS);

  MatrixWorkspace_sptr matrixWS =
      boost::dynamic_pointer_cast<MatrixWorkspace>(inBareWS);
  PeaksWorkspace_sptr peaksWS =
      boost::dynamic_pointer_cast<PeaksWorkspace>(inBareWS);
  IMDEventWorkspace_sptr mdWS =
      boost::dynamic_pointer_cast<IMDEventWorkspace>(inBareWS);

  std::vector<DblMatrix> gonioVec;
  if (matrixWS) {
    // Retrieve the goniometer rotation matrix
    try {
      DblMatrix goniometerMatrix = matrixWS->run().getGoniometerMatrix();
      gonioVec.push_back(goniometerMatrix);
    } catch (std::runtime_error &e) {
      // If there is no goniometer matrix, use identity matrix instead.
      g_log.error() << "Error getting the goniometer rotation matrix from the "
                       "InputWorkspace."
                    << std::endl
                    << e.what() << std::endl;
      g_log.warning() << "Using identity goniometer rotation matrix instead."
                      << std::endl;
    }
  } else if (peaksWS) {
    // Sort peaks by run number so that peaks with equal goniometer matrices are
    // adjacent
    std::vector<std::pair<std::string, bool>> criteria;
    criteria.push_back(std::pair<std::string, bool>("RunNumber", true));

    peaksWS->sort(criteria);

    // Get all goniometer matrices
    DblMatrix lastGoniometerMatrix = Matrix<double>(3, 3, false);
    for (int i = 0; i < static_cast<int>(peaksWS->getNumberPeaks()); ++i) {
      IPeak &p = peaksWS->getPeak(i);
      DblMatrix currentGoniometerMatrix = p.getGoniometerMatrix();
      if (!(currentGoniometerMatrix == lastGoniometerMatrix)) {
        gonioVec.push_back(currentGoniometerMatrix);
        lastGoniometerMatrix = currentGoniometerMatrix;
      }
    }

  } else if (mdWS) {
    if (mdWS->getNumExperimentInfo() <= 0)
      throw std::invalid_argument(
          "Specified a MDEventWorkspace as InputWorkspace but it does not have "
          "any ExperimentInfo associated. Please choose a workspace with a "
          "full instrument and sample.");

    inWS = mdWS->getExperimentInfo(0);

    // Retrieve the goniometer rotation matrices for each experiment info
    for (uint16_t i = 0; i < mdWS->getNumExperimentInfo(); ++i) {
      try {
        DblMatrix goniometerMatrix =
            mdWS->getExperimentInfo(i)->mutableRun().getGoniometerMatrix();
        gonioVec.push_back(goniometerMatrix);
      } catch (std::runtime_error &e) {
        // If there is no goniometer matrix, use identity matrix instead.
        gonioVec.push_back(DblMatrix(3, 3, true));

        g_log.error()
            << "Error getting the goniometer rotation matrix from the "
               "InputWorkspace."
            << std::endl
            << e.what() << std::endl;
        g_log.warning() << "Using identity goniometer rotation matrix instead."
                        << std::endl;
      }
    }
  }

  // If there's no goniometer matrix at this point, push back an identity
  // matrix.
  if (gonioVec.empty()) {
    gonioVec.push_back(DblMatrix(3, 3, true));
  }

  // Find the run number
  if (inWS) {
    m_runNumber = inWS->getRunNumber();
  } else {
    throw std::runtime_error("Failed to get run number");
  }

  m_wlMin = getProperty("WavelengthMin");
  m_wlMax = getProperty("WavelengthMax");
  m_minD = getProperty("MinDSpacing");
  m_maxD = getProperty("MaxDSpacing");
  bool RoundHKL = getProperty("RoundHKL");

  PeaksWorkspace_sptr HKLPeaksWorkspace = getProperty("HKLPeaksWorkspace");

  // Check the values.
  if (!inWS || !inWS->getInstrument()->getSample())
    throw std::invalid_argument("Did not specify a valid InputWorkspace with a "
                                "full instrument and sample.");

  // Get the instrument and its detectors
  m_inst = inWS->getInstrument();

  // Sample position
  V3D samplePos = m_inst->getSample()->getPos();

  // L1 path and direction
  V3D beamDir = m_inst->getSource()->getPos() - samplePos;

  if ((fabs(beamDir.X()) > 1e-2) ||
      (fabs(beamDir.Y()) > 1e-2)) // || (beamDir.Z() < 0))
    throw std::invalid_argument("Instrument must have a beam direction that "
                                "is only in the +Z direction for this "
                                "algorithm to be valid..");

  // --- Reflection condition ----
  // Use the primitive by default
  ReflectionCondition_sptr refCond(new ReflectionConditionPrimitive());
  // Get it from the property
  std::string refCondName = getPropertyValue("ReflectionCondition");
  for (size_t i = 0; i < m_refConds.size(); ++i)
    if (m_refConds[i]->getName() == refCondName)
      refCond = m_refConds[i];

  // Create the output
  m_pw = PeaksWorkspace_sptr(new PeaksWorkspace());
  setProperty<PeaksWorkspace_sptr>("OutputWorkspace", m_pw);

  // Copy instrument, sample, etc.
  m_pw->copyExperimentInfoFrom(inWS.get());

  // Retrieve the OrientedLattice (UnitCell) from the workspace
  m_crystal = inWS->sample().getOrientedLattice();

  // Counter of possible peaks
  m_numInRange = 0;

  // Get the UB matrix from it
  Matrix<double> ub(3, 3, true);
  ub = m_crystal.getUB();

  std::vector<V3D> possibleHKLs;
  if (!HKLPeaksWorkspace) {
    HKLGenerator gen(m_crystal, m_minD);
    auto filter =
        boost::make_shared<HKLFilterCentering>(refCond) &
        boost::make_shared<HKLFilterDRange>(m_crystal, m_minD, m_maxD) &
        boost::make_shared<HKLFilterWavelength>(ub, m_wlMin, m_wlMax);

    V3D hklMin = *(gen.begin());

    g_log.notice() << "HKL range for d_min of " << m_minD << " to d_max of "
                   << m_maxD << " is from " << hklMin << " to " << hklMin * -1.0
                   << ", a total of " << gen.size() << " possible HKL's\n";

    g_log.notice() << "Other: " << gen.size() << std::endl;

    if (gen.size() > 10000000000)
      throw std::invalid_argument("More than 10 billion HKLs to search. Is "
                                  "your d_min value too small?");

    possibleHKLs.reserve(gen.size());
    std::remove_copy_if(gen.begin(), gen.end(),
                        std::back_inserter(possibleHKLs), (~filter)->fn());
  } else {
    possibleHKLs.reserve(HKLPeaksWorkspace->getNumberPeaks());
    for (int i = 0; i < static_cast<int>(HKLPeaksWorkspace->getNumberPeaks());
         ++i) {

      IPeak &p = HKLPeaksWorkspace->getPeak(i);
      // Get HKL from that peak
      V3D hkl = p.getHKL();
      // Use the rounded HKL value on option
      if (RoundHKL)
        hkl.round();
      // Predict the HKL of that peak
      possibleHKLs.push_back(hkl);

    } // for each hkl in the workspace
  }

  for (size_t iVec = 0; iVec < gonioVec.size(); ++iVec) {
    m_gonio = gonioVec[iVec];
    // Final transformation matrix (HKL to Q in lab frame)
    m_mat = m_gonio * ub;

    Progress prog(this, 0.0, 1.0, possibleHKLs.size());
    prog.setNotifyStep(0.01);

    for (auto hkl = possibleHKLs.begin(); hkl != possibleHKLs.end(); ++hkl) {
      doHKL(*hkl);
      prog.report();
    }

    g_log.notice() << "Out of " << possibleHKLs.size()
                   << " allowed peaks within parameters, "
                   << m_pw->getNumberPeaks()
                   << " were found to hit a detector.\n";
  }
}

} // namespace Mantid
} // namespace Crystal
