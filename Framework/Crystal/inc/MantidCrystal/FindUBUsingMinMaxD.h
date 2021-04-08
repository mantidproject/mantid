// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/DeprecatedAlgorithm.h"
#include "MantidCrystal/DllConfig.h"

namespace Mantid {
namespace Crystal {
/** FindUBUsingMinMaxD : Algorithm to calculate a UB matrix, given bounds
    on the lattice parameters and a list of peaks.

    @author Dennis Mikkelson(adapted from Andrei Savici's CalculateUMatrix)
    @date   2011-08-17
  */
class MANTID_CRYSTAL_DLL FindUBUsingMinMaxD : public API::Algorithm, public API::DeprecatedAlgorithm {
public:
  FindUBUsingMinMaxD();

  /// Algorithm's name for identification
  const std::string name() const override;

  /// Algorithm's version for identification
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"SetUB", "FindUBUsingFFT", "FindUBUsingIndexedPeaks", "FindUBUsingLatticeParameters"};
  }

  /// Algorithm's category for identification
  const std::string category() const override;

  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Calculate the UB matrix from a peaks workspace, given min(a,b,c) "
           "and max(a,b,c).";
  }

private:
  /// Initialise the properties
  void init() override;

  /// Run the algorithm
  void exec() override;
};

} // namespace Crystal
} // namespace Mantid
