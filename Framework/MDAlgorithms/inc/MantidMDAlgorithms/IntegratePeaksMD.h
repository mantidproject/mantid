// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDALGORITHMS_INTEGRATEPEAKSMD_H_
#define MANTID_MDALGORITHMS_INTEGRATEPEAKSMD_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/IMDEventWorkspace_fwd.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace MDAlgorithms {

/** Integrate single-crystal peaks in reciprocal-space.
 *
 * @author Janik Zikovsky
 * @date 2011-04-13 18:11:53.496539
 */
class DLLExport IntegratePeaksMD : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "IntegratePeaksMD"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Integrate single-crystal peaks in reciprocal space, for "
           "MDEventWorkspaces.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  /// Algorithm's category for identification
  const std::string category() const override { return "Crystal\\Integration"; }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;

  template <typename MDE, size_t nd>
  void integrate(typename DataObjects::MDEventWorkspace<MDE, nd>::sptr ws);

  /// Input MDEventWorkspace
  Mantid::API::IMDEventWorkspace_sptr inWS;

  /// Calculate if this Q is on a detector
  bool detectorQ(Mantid::Kernel::V3D QLabFrame, double r);

  /// Instrument reference
  Geometry::Instrument_const_sptr inst;
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_INTEGRATEPEAKSMD_H_ */
