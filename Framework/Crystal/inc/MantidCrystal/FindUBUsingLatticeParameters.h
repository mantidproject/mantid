// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CRYSTAL_FIND_UB_USING_LATTICE_PARAMETERS_H_
#define MANTID_CRYSTAL_FIND_UB_USING_LATTICE_PARAMETERS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Crystal {
/** FindUBUsingLatticeParameters : Algorithm to calculate a UB matrix,
    given lattice parameters and a list of peaks

    @author Dennis Mikkelson(adapted from Andrei Savici's CalculateUMatrix)
    @date   2011-08-17
  */
class DLLExport FindUBUsingLatticeParameters : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override {
    return "FindUBUsingLatticeParameters";
  };

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"SetUB", "FindUBUsingFFT", "FindUBUsingIndexedPeaks",
            "FindUBUsingMinMaxD"};
  }

  /// Algorithm's category for identification
  const std::string category() const override { return "Crystal\\UBMatrix"; }

  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Calculate the UB matrix from a peaks workspace, given lattice "
           "parameters.";
  }

private:
  /// Initialise the properties
  void init() override;

  /// Run the algorithm
  void exec() override;
};

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_FIND_UB_USING_LATTICE_PARAMETERS */
