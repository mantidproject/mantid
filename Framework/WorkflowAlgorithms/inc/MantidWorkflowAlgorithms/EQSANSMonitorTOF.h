// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

namespace Mantid {
namespace WorkflowAlgorithms {
/**

    Determine the wavelength from the TOF in the beam monitor histogram.  The
   algorithm has to modify
    TOF values to correct for the fact that T_0 is not properly recorded by the
   DAS.
*/
// Pulse widge (micro sec per angstrom)
const double PULSEWIDTH = 20.0;
// Chopper phase offset (micro sec)
const double CHOPPER_PHASE_OFFSET[2][4] = {{9507., 9471., 9829.7, 9584.3}, {19024., 18820., 19714., 19360.}};
// Chopper angles (degree)
const double CHOPPER_ANGLE[4] = {129.605, 179.989, 230.010, 230.007};
// Chopper location (mm)
const double CHOPPER_LOCATION[4] = {5700., 7800., 9497., 9507.};

class DLLExport EQSANSMonitorTOF : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "EQSANSMonitorTOF"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Converts the TOF into a wavelength for the beam monitor. This "
           "algorithm needs to be run once on every data set.";
  }
  /// Algorithm's version
  int version() const override { return (1); }
  /// Algorithm's category for identification
  const std::string category() const override { return "Workflow\\SANS"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;

  /// Compute TOF offset
  double getTofOffset(const API::MatrixWorkspace_const_sptr &inputWS, bool frame_skipping, double source_to_monitor);
};

} // namespace WorkflowAlgorithms
} // namespace Mantid
