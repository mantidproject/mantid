// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidMDAlgorithms/IntegrateQLabEvents.h"
#include "MantidMDAlgorithms/MDTransfInterface.h"
#include "MantidMDAlgorithms/MDWSDescription.h"
#include "MantidMDAlgorithms/UnitsConversionHelper.h"

namespace Mantid {
namespace Geometry {
class DetectorInfo;
}
namespace MDAlgorithms {
// Note: this relies on several MD specific interfaces, so it needs to be defined
// in MDAlgorithms despite it not actually operating on MDWorkspaces

class DLLExport IntegrateEllipsoids : public API::Algorithm {
public:
  const std::string name() const override { return "IntegrateEllipsoids"; }

  const std::string summary() const override {
    return "Integrate Single Crystal Diffraction Bragg peaks using 3D "
           "ellipsoids.";
  }

  int version() const override { return 1; }

  const std::vector<std::string> seeAlso() const override { return {"IntegrateEllipsoidsTwoStep"}; }
  const std::string category() const override { return "Crystal\\Integration"; }

private:
  /// Initialize the algorithm's properties
  void init() override;
  /// Execute the algorithm
  void exec() override;
  /// Private validator for inputs
  std::map<std::string, std::string> validateInputs() override;

  /**
   * @brief create a list of SlimEvent objects from an events workspace
   * @param integrator : integrator object on the list is accumulated
   * @param prog : progress object
   * @param wksp : input EventWorkspace
   */
  void qListFromEventWS(IntegrateQLabEvents &integrator, API::Progress &prog, DataObjects::EventWorkspace_sptr &wksp);

  /**
   * @brief create a list of SlimEvent objects from a histogram workspace
   * @param integrator : integrator object on which the list is accumulated
   * @param prog : progress object
   * @param wksp : input Workspace2D
   */
  void qListFromHistoWS(IntegrateQLabEvents &integrator, API::Progress &prog, DataObjects::Workspace2D_sptr &wksp);

  /// Calculate if this Q is on a detector
  void calculateE1(const Geometry::DetectorInfo &detectorInfo);

  void runMaskDetectors(const Mantid::DataObjects::PeaksWorkspace_sptr &peakWS, const std::string &property,
                        const std::string &values);

  /// save for all detector pixels
  std::vector<Kernel::V3D> E1Vec;

  MDWSDescription m_targWSDescr;

  /**
   * @brief Initialize the output information for the MD conversion framework.
   * @param wksp : The workspace to get information from.
   */
  void initTargetWSDescr(API::MatrixWorkspace_sptr &wksp);
};

} // namespace MDAlgorithms
} // namespace Mantid
