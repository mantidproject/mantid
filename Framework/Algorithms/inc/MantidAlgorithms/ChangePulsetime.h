// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {

/** ChangePulsetime : TODO: DESCRIPTION
 *
 * @author
 * @date 2011-03-31 09:31:55.674594
 */
class MANTID_ALGORITHMS_DLL ChangePulsetime : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "ChangePulsetime"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Adds a constant time value, in seconds, to the pulse time of "
           "events in an EventWorkspace. ";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override { return {"CreateLogTimeCorrection", "CalculateCountRate"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "Events;Transforms\\Axes"; }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid
