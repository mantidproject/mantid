// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/AddPeak.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/IPeak.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidKernel/Unit.h"

using namespace Mantid::PhysicalConstants;

namespace Mantid::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(AddPeak)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using Mantid::DataObjects::Peak_uptr;
using Mantid::DataObjects::PeaksWorkspace;
using Mantid::DataObjects::PeaksWorkspace_sptr;
using Mantid::Geometry::IPeak_uptr;

/** Initialize the algorithm's properties.
 */
void AddPeak::init() {
  declareProperty(std::make_unique<WorkspaceProperty<PeaksWorkspace>>("PeaksWorkspace", "", Direction::InOut),
                  "A peaks workspace.");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("RunWorkspace", "", Direction::Input),
                  "An input workspace containing the run information.");
  declareProperty("TOF", 0.0, "Peak position in time of flight.");
  declareProperty("DetectorID", 0, "ID of a detector at the peak centre.");
  declareProperty("Height", 0.0, "Height of the peak.");
  declareProperty("BinCount", 0.0, "Bin count.");
}

/** Execute the algorithm.
 */
void AddPeak::exec() {
  PeaksWorkspace_sptr peaksWS = getProperty("PeaksWorkspace");
  MatrixWorkspace_sptr runWS = getProperty("RunWorkspace");

  // Check the instruments match before attempting to add a peak.
  auto runInst = runWS->getInstrument()->getName();
  auto peakInst = peaksWS->getInstrument()->getName();
  if (peaksWS->getNumberPeaks() > 0 && (runInst != peakInst)) {
    throw std::runtime_error("The peak from " + runWS->getName() + " comes from a different instrument (" + runInst +
                             ") to the peaks "
                             "already in the table (" +
                             peakInst + "). It could not be added.");
  }

  const int detID = getProperty("DetectorID");
  double tof = getProperty("TOF");
  const double height = getProperty("Height");
  const double count = getProperty("BinCount");

  const auto &detectorInfo = runWS->detectorInfo();
  const size_t detectorIndex = detectorInfo.indexOf(detID);

  double theta2 = detectorInfo.twoTheta(detectorIndex);
  const Mantid::Geometry::IDetector &det = detectorInfo.detector(detectorIndex);
  double phi = det.getPhi();

  // In the inelastic convention, Q = ki - kf.
  // qSign later in algorithm will change to kf - ki for Crystallography
  // Convention
  double Qx = -sin(theta2) * cos(phi);
  double Qy = -sin(theta2) * sin(phi);
  double Qz = 1.0 - cos(theta2);
  double l1 = detectorInfo.l1();
  double l2 = detectorInfo.l2(detectorIndex);
  std::vector<int> emptyWarningVec;
  auto [difa, difc, tzero] = detectorInfo.diffractometerConstants(detectorIndex, emptyWarningVec, emptyWarningVec);

  Mantid::Kernel::Unit_sptr unit = runWS->getAxis(0)->unit();
  if (unit->unitID() != "TOF") {
    const Mantid::API::Run &run = runWS->run();
    int emode = 0;
    double efixed = 0.0;
    if (run.hasProperty("Ei")) {
      emode = 1; // direct
      efixed = run.getPropertyValueAsType<double>("Ei");
    } else if (det.hasParameter("Efixed")) {
      emode = 2; // indirect
      try {
        const Mantid::Geometry::ParameterMap &pmap = runWS->constInstrumentParameters();
        Mantid::Geometry::Parameter_sptr par = pmap.getRecursive(&det, "Efixed");
        if (par) {
          efixed = par->value<double>();
        }
      } catch (std::runtime_error &) { /* Throws if a DetectorGroup, use single
                                          provided value */
      }
    } else {
      // m_emode = 0; // Elastic
      // This should be elastic if Ei and Efixed are not set
      // TODO
    }
    std::vector<double> xdata(1, tof);
    std::vector<double> ydata;
    unit->toTOF(xdata, ydata, l1, emode,
                {{Kernel::UnitParams::l2, l2},
                 {Kernel::UnitParams::twoTheta, theta2},
                 {Kernel::UnitParams::efixed, efixed},
                 {Kernel::UnitParams::difa, difa},
                 {Kernel::UnitParams::difc, difc},
                 {Kernel::UnitParams::tzero, tzero}});
    tof = xdata[0];
  }

  std::string m_qConvention = Kernel::ConfigService::Instance().getString("Q.convention");
  double qSign = 1.0;
  if (m_qConvention == "Crystallography") {
    qSign = -1.0;
  }
  double knorm = qSign * NeutronMass * (l1 + l2) / (h_bar * tof * 1e-6) / 1e10;
  Qx *= knorm;
  Qy *= knorm;
  Qz *= knorm;

  IPeak_uptr ipeak = peaksWS->createPeak(Mantid::Kernel::V3D(Qx, Qy, Qz), l2);
  Peak_uptr peak(static_cast<DataObjects::Peak *>(ipeak.release()));
  peak->setDetectorID(detID);
  peak->setGoniometerMatrix(runWS->run().getGoniometer().getR());
  peak->setBinCount(count);
  peak->setRunNumber(runWS->getRunNumber());
  peak->setIntensity(height);
  if (height > 0.)
    peak->setSigmaIntensity(std::sqrt(height));

  peaksWS->addPeak(*peak);
  // peaksWS->modified();
}

} // namespace Mantid::Algorithms
