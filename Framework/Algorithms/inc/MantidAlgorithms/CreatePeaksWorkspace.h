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

/** Create an empty PeaksWorkspace.
 *
 * @author Janik Zikovsky
 * @date 2011-04-26 08:49:10.540441
 */
class MANTID_ALGORITHMS_DLL CreatePeaksWorkspace : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "CreatePeaksWorkspace"; };
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Create an empty PeaksWorkspace."; }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override { return {"SortPeaksWorkspace"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "Crystal\\Peaks;Utility\\Workspaces"; }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid
