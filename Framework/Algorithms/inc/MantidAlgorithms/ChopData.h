// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {
/**

  For use in TOSCA reduction. Splits a 0-100k microsecond workspace into either
  five 20k or
  three 20k and a 40k workspaces

  @author Michael Whitty
  @date 03/02/2011
*/
class MANTID_ALGORITHMS_DLL ChopData : public API::Algorithm {
public:
  const std::string name() const override { return "ChopData"; }                  ///< @return the algorithms name
  const std::string category() const override { return "Transforms\\Splitting"; } ///< @return the algorithms category
  int version() const override { return (1); } ///< @return version number of algorithm

  const std::vector<std::string> seeAlso() const override { return {"ExtractSpectra"}; }
  /// Algorithm's summary
  const std::string summary() const override {
    return "Splits an input workspace into a grouped workspace, where each "
           "spectra "
           "if 'chopped' at a certain point (given in 'Step' input value) "
           "and the X values adjusted to give all the workspace in the group "
           "the same binning.";
  }

private:
  void init() override; ///< Initialise the algorithm. Declare properties, etc.
  void exec() override; ///< Executes the algorithm.
};
} // namespace Algorithms
} // namespace Mantid
