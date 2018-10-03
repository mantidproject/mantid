// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_FILTERBADPULSES_H_
#define MANTID_ALGORITHMS_FILTERBADPULSES_H_

#include "MantidAPI/DistributedAlgorithm.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/System.h"

namespace Mantid {

namespace Algorithms {

/** Filters out events associated with pulses that happen when proton charge is
   lower than a given percentage of the average.

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the Workspace whose detectors are to be
   aligned </LI>
    <LI> OutputWorkspace - The name of the Workspace in which the result of the
   algorithm will be stored </LI>
    <LI> LowerCutoff - The percentage of the average to use as the lower bound
   </LI>
    </UL>

    @author Peter Peterson, ORNL
    @date 21/12/10
*/
class DLLExport FilterBadPulses : public API::DistributedAlgorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Filters out events associated with pulses that happen when proton "
           "charge is lower than a given percentage of the average.";
  }

  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"FilterByTime", "FilterByLogValue"};
  }

  const std::string category() const override;

private:
  // Implement abstract Algorithm methods
  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_FILTERBADPULSES_H_ */
