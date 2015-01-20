#include "MantidCrystal/PredictPeaks.h"
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/UnitCell.h"
#include "MantidKernel/Matrix.h"
#include "MantidGeometry/Objects/InstrumentRayTracer.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/System.h"
#include <cmath>
#include <MantidGeometry/Crystal/OrientedLattice.h>
#include "MantidKernel/ListValidator.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidGeometry/Crystal/ReflectionCondition.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidAPI/IMDEventWorkspace.h"

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
PredictPeaks::PredictPeaks() { m_refConds = getAllReflectionConditions(); }

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
void PredictPeaks::doHKL(const double h, const double k, const double l,
                         bool doFilter) {
  V3D hkl(h, k, l);

  // Skip those with unacceptable d-spacings
  double d = crystal.d(hkl);
  if (!doFilter || (d > minD && d < maxD)) {
    // The q-vector direction of the peak is = goniometer * ub * hkl_vector
    // This is in inelastic convention: momentum transfer of the LATTICE!
    // Also, q does have a 2pi factor = it is equal to 2pi/wavelength.
    V3D q = mat * hkl * (2.0 * M_PI);

    /* The incident neutron wavevector is in the +Z direction, ki = 2*pi/wl (in
     * z direction).
     * In the inelastic convention, q = ki - kf.
     * The final neutron wavector kf = -qx in x; -qy in y; and (-qz+2*pi/wl) in
     * z.
     * AND: norm(kf) = norm(ki) = 2*pi/wavelength
     * THEREFORE: 2*pi/wl = norm(q)^2 / (2*qz)
     */
    double norm_q = q.norm();
    double one_over_wl = ((norm_q * norm_q) / (2.0 * q.Z())) / (2.0 * M_PI);
    double wl = 1.0 / one_over_wl;

    g_log.debug() << "Peak at " << hkl << " has d-spacing " << d
                  << " and wavelength " << wl << std::endl;

    // Only keep going for accepted wavelengths.
    if (wl > 0 && (!doFilter || (wl >= wlMin && wl <= wlMax))) {
      PARALLEL_CRITICAL(PredictPeaks_numInRange) { numInRange++; }

      // Create the peak using the Q in the lab framewith all its info:
      Peak p(inst, q);
      if (p.findDetector()) {
        // Only add peaks that hit the detector
        p.setGoniometerMatrix(gonio);
        // Save the run number found before.
        p.setRunNumber(runNumber);
        p.setHKL(hkl);

        // Add it to the workspace
        PARALLEL_CRITICAL(PredictPeaks_appendPeak) { pw->addPeak(p); }
      } // Detector was found
    }   // (wavelength is okay)
  }     // (d is acceptable)
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void PredictPeaks::exec() {
  // Get the input properties
  Workspace_sptr inBareWS = getProperty("InputWorkspace");
  ExperimentInfo_sptr inWS;
  MatrixWorkspace_sptr matrixWS =
      boost::dynamic_pointer_cast<MatrixWorkspace>(inBareWS);
  PeaksWorkspace_sptr peaksWS =
      boost::dynamic_pointer_cast<PeaksWorkspace>(inBareWS);
  IMDEventWorkspace_sptr mdWS =
      boost::dynamic_pointer_cast<IMDEventWorkspace>(inBareWS);
  std::vector<Matrix<double> > gonioVec;
  gonio = Matrix<double>(3, 3, true);
  Mantid::Kernel::DblMatrix gonioLast = Matrix<double>(3, 3, true);
  if (matrixWS) {
    inWS = matrixWS;
    // Retrieve the goniometer rotation matrix
    try {
      gonio = inWS->mutableRun().getGoniometerMatrix();
      gonioVec.push_back(gonio);
    }
    catch (std::runtime_error &e) {
      g_log.error() << "Error getting the goniometer rotation matrix from the "
                       "InputWorkspace." << std::endl << e.what() << std::endl;
      g_log.warning() << "Using identity goniometer rotation matrix instead."
                      << std::endl;
    }
  } else if (peaksWS) {
  // We must sort the peaks
    std::vector<std::pair<std::string, bool>> criteria;
    criteria.push_back(std::pair<std::string, bool>("RunNumber", true));
    //criteria.push_back(std::pair<std::string, bool>("BankName", true));
    peaksWS->sort(criteria);
    inWS = peaksWS;
    for (int i = 0; i < static_cast<int>(peaksWS->getNumberPeaks()); ++i) {

      IPeak &p = peaksWS->getPeak(i);
      gonio = p.getGoniometerMatrix();
      if (!(gonio == gonioLast)) {
        gonioLast = gonio;
        gonioVec.push_back(gonioLast);
      }
    } // for each hkl in the workspace
  } else if (mdWS) {
    if (mdWS->getNumExperimentInfo() <= 0)
      throw std::invalid_argument(
          "Specified a MDEventWorkspace as InputWorkspace but it does not have "
          "any ExperimentInfo associated. Please choose a workspace with a "
          "full instrument and sample.");
    inWS = mdWS->getExperimentInfo(0);
    // Retrieve the goniometer rotation matrix
    for (uint16_t i = 0; i < mdWS->getNumExperimentInfo(); i++) {
      gonio = mdWS->getExperimentInfo(i)->mutableRun().getGoniometerMatrix();
      gonioVec.push_back(gonio);
    }
  }
  // Find the run number
  runNumber = inWS->getRunNumber();

  wlMin = getProperty("WavelengthMin");
  wlMax = getProperty("WavelengthMax");
  minD = getProperty("MinDSpacing");
  maxD = getProperty("MaxDSpacing");
  bool RoundHKL = getProperty("RoundHKL");

  PeaksWorkspace_sptr HKLPeaksWorkspace = getProperty("HKLPeaksWorkspace");

  // Check the values.
  if (!inWS || !inWS->getInstrument()->getSample())
    throw std::invalid_argument("Did not specify a valid InputWorkspace with a "
                                "full instrument and sample.");
  if (wlMin >= wlMax)
    throw std::invalid_argument("WavelengthMin must be < WavelengthMax.");
  if (wlMin < 1e-5)
    throw std::invalid_argument("WavelengthMin must be stricly positive.");
  if (minD < 1e-4)
    throw std::invalid_argument("MinDSpacing must be stricly positive.");
  if (minD >= maxD)
    throw std::invalid_argument("MinDSpacing must be < MaxDSpacing.");

  // Get the instrument and its detectors
  inst = inWS->getInstrument();

  // --- Reflection condition ----
  // Use the primitive by default
  ReflectionCondition_sptr refCond(new ReflectionConditionPrimitive());
  // Get it from the property
  std::string refCondName = getPropertyValue("ReflectionCondition");
  for (size_t i = 0; i < m_refConds.size(); ++i)
    if (m_refConds[i]->getName() == refCondName)
      refCond = m_refConds[i];

  // Create the output
  pw = PeaksWorkspace_sptr(new PeaksWorkspace());
  setProperty<PeaksWorkspace_sptr>("OutputWorkspace", pw);
  // Copy instrument, sample, etc.
  pw->copyExperimentInfoFrom(inWS.get());

  // Retrieve the OrientedLattice (UnitCell) from the workspace
  crystal = inWS->sample().getOrientedLattice();

  // Counter of possible peaks
  numInRange = 0;

  // Get the UB matrix from it
  Matrix<double> ub(3, 3, true);
  ub = crystal.getUB();
  for (size_t iVec = 0; iVec < gonioVec.size(); ++iVec) {
    gonio = gonioVec[iVec];
    // Final transformation matrix (HKL to Q in lab frame)
    mat = gonio * ub;

    // Sample position
    V3D samplePos = inst->getSample()->getPos();

    // L1 path and direction
    V3D beamDir = inst->getSource()->getPos() - samplePos;
    // double L1 = beamDir.normalize(); // Normalize to unity

    if ((fabs(beamDir.X()) > 1e-2) ||
        (fabs(beamDir.Y()) > 1e-2)) // || (beamDir.Z() < 0))
      throw std::invalid_argument("Instrument must have a beam direction that "
                                  "is only in the +Z direction for this "
                                  "algorithm to be valid..");

    if (HKLPeaksWorkspace) {
      // --------------Use the HKL from a list in a PeaksWorkspace
      // --------------------------
      // Disable some of the other filters
      minD = 0.0;
      maxD = 1e10;
      wlMin = 0.0;
      wlMax = 1e10;

      PRAGMA_OMP(parallel for schedule(dynamic, 1) )
      for (int i = 0; i < static_cast<int>(HKLPeaksWorkspace->getNumberPeaks()); ++i) {
        PARALLEL_START_INTERUPT_REGION

        IPeak &p = HKLPeaksWorkspace->getPeak(i);
        // Get HKL from that peak
        V3D hkl = p.getHKL();
        // Use the rounded HKL value on option
        if (RoundHKL)
          hkl.round();
        // Predict the HKL of that peak
        doHKL(hkl[0], hkl[1], hkl[2], false);

        PARALLEL_END_INTERUPT_REGION
      } // for each hkl in the workspace
      PARALLEL_CHECK_INTERUPT_REGION
    } else {
      // ---------------- Determine which HKL to look for
      // -------------------------------------
      // Inverse d-spacing that is the limit to look for.
      double Qmax = 2. * M_PI / minD;
      V3D hklMin(0, 0, 0);
      V3D hklMax(0, 0, 0);
      for (double qx = -1; qx < 2; qx += 2)
        for (double qy = -1; qy < 2; qy += 2)
          for (double qz = -1; qz < 2; qz += 2) {
            // Build a q-vector for this corner of a cube
            V3D Q(qx, qy, qz);
            Q *= Qmax;
            V3D hkl = crystal.hklFromQ(Q);
            // Find the limits of each hkl
            for (size_t i = 0; i < 3; i++) {
              if (hkl[i] < hklMin[i])
                hklMin[i] = hkl[i];
              if (hkl[i] > hklMax[i])
                hklMax[i] = hkl[i];
            }
          }
      // Round to nearest int
      hklMin.round();
      hklMax.round();

      // How many HKLs is that total?
      V3D hklDiff = hklMax - hklMin + V3D(1, 1, 1);
      size_t numHKLs = size_t(hklDiff[0] * hklDiff[1] * hklDiff[2]);

      g_log.information() << "HKL range for d_min of " << minD << "to d_max of "
                          << maxD << " is from " << hklMin << " to " << hklMax
                          << ", a total of " << numHKLs << " possible HKL's\n";

      if (numHKLs > 10000000000)
        throw std::invalid_argument("More than 10 billion HKLs to search. Is "
                                    "your d_min value too small?");

      Progress prog(this, 0.0, 1.0, numHKLs);
      prog.setNotifyStep(0.01);

      PRAGMA_OMP(parallel for schedule(dynamic, 1) )
      for (int h = (int)hklMin[0]; h <= (int)hklMax[0]; h++) {
        PARALLEL_START_INTERUPT_REGION
        for (int k = (int)hklMin[1]; k <= (int)hklMax[1]; k++) {
          for (int l = (int)hklMin[2]; l <= (int)hklMax[2]; l++) {
            if (refCond->isAllowed(h, k, l) &&
                (abs(h) + abs(k) + abs(l) != 0)) {
              doHKL(double(h), double(k), double(l), true);
            } // refl is allowed and not 0,0,0
            prog.report();
          } // for each l
        }   // for each k
        PARALLEL_END_INTERUPT_REGION
      } // for each h
      PARALLEL_CHECK_INTERUPT_REGION

    } // Find the HKL automatically

    g_log.notice() << "Out of " << numInRange
                   << " allowed peaks within parameters, "
                   << pw->getNumberPeaks()
                   << " were found to hit a detector.\n";
  }
}

} // namespace Mantid
} // namespace Crystal
