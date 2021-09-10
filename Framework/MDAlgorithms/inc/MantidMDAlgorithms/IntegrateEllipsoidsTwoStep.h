// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Progress.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/Matrix.h"
#include "MantidMDAlgorithms/Integrate3DEvents.h"

namespace Mantid {
namespace MDAlgorithms {

/** @class IntegrateEllipsoidsTwoStep

  IntegrateEllipsoidsTwoStep provides a two pass peak integration algorithm.
*/

class DLLExport IntegrateEllipsoidsTwoStep : public API::Algorithm {
public:
  /// Get the name of this algorithm
  const std::string name() const override;
  /// Get the version of this algorithm
  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"IntegrateEllipsoids"}; }
  /// Get the category of this algorithm
  const std::string category() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Integrate Single Crystal Diffraction Bragg peaks using 3D "
           "ellipsoids.";
  }

private:
  void init() override;
  void exec() override;
  IntegrationParameters makeIntegrationParameters(const Kernel::V3D &peak_q) const;

  void qListFromHistoWS(Integrate3DEvents &integrator, API::Progress &prog, DataObjects::Workspace2D_sptr &wksp,
                        const Kernel::DblMatrix &UBinv, bool hkl_integ);
  void qListFromEventWS(Integrate3DEvents &integrator, API::Progress &prog, DataObjects::EventWorkspace_sptr &wksp,
                        const Kernel::DblMatrix &UBinv, bool hkl_integ);
  /// Calculate if this Q is on a detector
  void calculateE1(const Geometry::DetectorInfo &detectorInfo);
  void runMaskDetectors(const Mantid::DataObjects::PeaksWorkspace_sptr &peakWS, const std::string &property,
                        const std::string &values);

  /// integrate a collection of strong peaks
  DataObjects::PeaksWorkspace_sptr integratePeaks(DataObjects::PeaksWorkspace_sptr peaks, API::MatrixWorkspace_sptr ws);
  /// save all detector pixels
  std::vector<Kernel::V3D> E1Vec;
};

} // namespace MDAlgorithms
} // namespace Mantid
