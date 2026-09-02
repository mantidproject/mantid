// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
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

/** @class IntegratePeaksShapeMD

  IntegratePeaksShapeMD integrates single crystal Bragg peaks using the
  ellipsoidal peak shape already stored on each peak in the input
  PeaksWorkspace (e.g. from a previous IntegrateEllipsoids run), rather than
  fitting a new shape from the events around each peak.
*/

class MANTID_MDALGORITHMS_DLL IntegratePeaksShapeMD final : public API::Algorithm {
public:
  /// Get the name of this algorithm
  const std::string name() const override;
  /**
   * Gets the algorithm version.
   * @return The algorithm version.
   */
  
  /**
   * Gets related algorithms.
   * @return The names of related algorithms.
   */
  
  /**
   * Gets the algorithm category.
   * @return The algorithm category.
   */
  
  /**
   * Describes integration of single-crystal diffraction Bragg peaks using stored ellipsoidal peak shapes.
   * @return A summary of the algorithm's purpose.
   */
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"IntegrateEllipsoids", "IntegrateEllipsoidsTwoStep", "IntegratePeaksMD"};
  }
  /// Get the category of this algorithm
  const std::string category() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Integrate Single Crystal Diffraction Bragg peaks using the ellipsoidal "
           "peak shape already stored on each peak.";
  }

private:
  void init() override;
  void exec() override;

  void qListFromHistoWS(Integrate3DEvents &integrator, API::Progress &prog, DataObjects::Workspace2D_sptr &wksp);
  void qListFromEventWS(Integrate3DEvents &integrator, API::Progress &prog, DataObjects::EventWorkspace_sptr &wksp);
};

} // namespace MDAlgorithms
} // namespace Mantid
