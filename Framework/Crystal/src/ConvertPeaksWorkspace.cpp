// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidCrystal/ConvertPeaksWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidDataObjects/LeanElasticPeak.h"
#include "MantidDataObjects/LeanElasticPeaksWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidKernel/Logger.h"

namespace Mantid {
namespace Crystal {

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

/// Config logger
namespace {
Logger logger("ConvertPeaksWorkspace");
}

DECLARE_ALGORITHM(ConvertPeaksWorkspace)

/**
 * @brief initialization of alg
 *
 */
void ConvertPeaksWorkspace::init() {
  // Input peakworkspace
  declareProperty(std::make_unique<WorkspaceProperty<IPeaksWorkspace>>("PeakWorkspace", "", Direction::Input),
                  "Workspace of Indexed Peaks");

  // donor workspace if going from lean to regular
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("InstrumentWorkpace", "", Direction::Input,
                                                                       PropertyMode::Optional),
                  "Donor workspace with instrument for up-casting LeanPeaksWorkspace to PeaksWorkspace");

  // output
  declareProperty(std::make_unique<WorkspaceProperty<IPeaksWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "Converted Workspaces");
}

/**
 * @brief Inputs validation
 *
 * @return std::map<std::string, std::string>
 */
std::map<std::string, std::string> ConvertPeaksWorkspace::validateInputs() {
  std::map<std::string, std::string> issues;

  IPeaksWorkspace_sptr ipws = getProperty("PeakWorkspace");
  PeaksWorkspace_sptr pws = std::dynamic_pointer_cast<PeaksWorkspace>(ipws);
  LeanElasticPeaksWorkspace_sptr lpws = std::dynamic_pointer_cast<LeanElasticPeaksWorkspace>(ipws);
  MatrixWorkspace_sptr ws = getProperty("InstrumentWorkpace");

  // case I: missing instrument when converting to PeaksWorkspace
  if (lpws && !pws) {
    if (ws->getInstrument()) {
      issues["InstrumentWorkpace"] = "InstrumentWorkspace must have a valid instrument attached to assist the "
                                     "conversion from LeanElasticPeaksWorkspace to PeaksWorkspace.";
    }
  }

  return issues;
}

/**
 * @brief Algorithm entrance func
 *
 */
void ConvertPeaksWorkspace::exec() {
  // parsing input
  IPeaksWorkspace_sptr ipws = getProperty("PeakWorkspace");
  PeaksWorkspace_sptr pws = std::dynamic_pointer_cast<PeaksWorkspace>(ipws);
  LeanElasticPeaksWorkspace_sptr lpws = std::dynamic_pointer_cast<LeanElasticPeaksWorkspace>(ipws);

  // decide which route to take
  if (pws && !lpws) {
    g_log.notice() << "PeaksWorkspace -> LeanElasticPeaksWorkspace\n";
    IPeaksWorkspace_sptr outpws = makeLeanElasticPeaksWorkspace(ipws);
    setProperty("OutputWorkspace", outpws);
  } else {
    g_log.notice() << "LeanElasticPeaksWorkspace -> PeaksWorkspace\n";
    MatrixWorkspace_sptr ws = getProperty("InstrumentWorkpace");
    Instrument_const_sptr inst = ws->getInstrument();
    IPeaksWorkspace_sptr outpws = makePeaksWorkspace(ipws, inst);
    setProperty("OutputWorkspace", outpws);
  }

  // cleanup
}

/**
 * @brief make a LeanElasticPeaksWorkspace using peaks from a regular PeaksWorkspace
 *
 * @param pws
 * @return IPeaksWorkspace_sptr
 */
IPeaksWorkspace_sptr ConvertPeaksWorkspace::makeLeanElasticPeaksWorkspace(IPeaksWorkspace_sptr ipws) {
  // prep
  PeaksWorkspace_sptr pws = std::dynamic_pointer_cast<PeaksWorkspace>(ipws);
  LeanElasticPeaksWorkspace_sptr lpws = std::make_shared<LeanElasticPeaksWorkspace>();

  ExperimentInfo_sptr inputExperimentInfo = std::dynamic_pointer_cast<ExperimentInfo>(ipws);
  lpws->copyExperimentInfoFrom(inputExperimentInfo.get());

  // down casting Peaks to LeanElasticPeaks
  for (int i = 0; i < pws->getNumberPeaks(); ++i) {
    LeanElasticPeak lpk(pws->getPeak(i).getQSampleFrame(), pws->getPeak(i).getWavelength());
    // NOTE:
    // Peak level info requires explicit copying
    lpk.setRunNumber(pws->getPeak(i).getRunNumber());
    lpk.setHKL(pws->getPeak(i).getHKL());
    lpk.setGoniometerMatrix(pws->getPeak(i).getGoniometerMatrix());
    //
    lpws->addPeak(lpk);
  }

  //
  IPeaksWorkspace_sptr outpws = std::dynamic_pointer_cast<IPeaksWorkspace>(lpws);
  return outpws;
}

/**
 * @brief Build a regular PeaksWorkspace using peaks from a LeanElasticPeaksWorkspace and the provided instrument
 *
 * @param lpws
 * @param instrument
 * @return IPeaksWorkspace_sptr
 */
IPeaksWorkspace_sptr ConvertPeaksWorkspace::makePeaksWorkspace(IPeaksWorkspace_sptr ipws,
                                                               Instrument_const_sptr &instrument) {
  // prep
  LeanElasticPeaksWorkspace_sptr lpws = std::dynamic_pointer_cast<LeanElasticPeaksWorkspace>(ipws);
  PeaksWorkspace_sptr pws = std::make_shared<PeaksWorkspace>();

  // up casting LeanElasticPeaks to Peaks
  // NOTE:
  //   - use instrument provided
  //   - check for negative wavelength, log a warning if needed

  //
  IPeaksWorkspace_sptr outpws = std::dynamic_pointer_cast<IPeaksWorkspace>(pws);
  return outpws;
}

} // namespace Crystal
} // namespace Mantid
