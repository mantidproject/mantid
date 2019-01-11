// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_CONVERTTOEVENTWORKSPACE_H_
#define MANTID_ALGORITHMS_CONVERTTOEVENTWORKSPACE_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Algorithms {

/** Perform a conversion for a Workspace2D to an equivalent
 * EventWorkspace

  @author Janik Zikovsky
  @date 2011-08-23
*/
class DLLExport ConvertToEventWorkspace : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "ConvertToEventWorkspace"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Converts a Workspace2D from histograms to events in an "
           "EventWorkspace by converting each bin to an equivalent weighted "
           "event.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"ConvertToMatrixWorkspace"};
  }
  /// Algorithm's category for identification
  const std::string category() const override { return "Events"; }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_CONVERTTOEVENTWORKSPACE_H_ */
