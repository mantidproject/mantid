// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include <gsl/gsl_blas.h>
#include <gsl/gsl_multifit_nlin.h>
#include <gsl/gsl_multimin.h>
#include <gsl/gsl_statistics.h>

namespace Mantid {
namespace Algorithms {
/**
 Find the offsets for each detector

 @author Vickie Lynch SNS, ORNL
 @date 12/02/2010
 */
class MANTID_ALGORITHMS_DLL DiffractionEventCalibrateDetectors : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "DiffractionEventCalibrateDetectors"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "This algorithm optimizes the position and angles of all of the "
           "detector panels. The target instruments for this feature are SNAP "
           "and TOPAZ.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"AlignComponents", "GetDetOffsetsMultiPeaks"}; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override {
    return "Diffraction\\Calibration;"
           "CorrectionFunctions\\InstrumentCorrections";
  }
  /// Function to optimize
  double intensity(double x, double y, double z, double rotx, double roty, double rotz, const std::string &detname,
                   const std::string &inname, const std::string &outname, const std::string &peakOpt,
                   const std::string &rb_param, const std::string &groupWSName);
  void movedetector(double x, double y, double z, double rotx, double roty, double rotz, const std::string &detname,
                    const Mantid::DataObjects::EventWorkspace_sptr &inputW);

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;
  // Event workspace pointer
  // API::EventWorkspace_sptr inputW;
};

} // namespace Algorithms
} // namespace Mantid
