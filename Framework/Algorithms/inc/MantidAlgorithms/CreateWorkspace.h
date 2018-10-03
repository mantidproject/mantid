// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_CREATEWORKSPACE_H_
#define MANTID_ALGORITHMS_CREATEWORKSPACE_H_

#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {
/**
 *  CreateWorkspace Algorithm
 *
 *  This algorithm constructs a MatrixWorkspace when passed a vector for each of
 *the X, Y, E, and Dx
 *  data values. The unit for the X Axis can optionally be specified as any of
 *the units in the
 *  Kernel's UnitFactory.
 *
 *  Multiple spectra may be created by supplying the NSpec Property (integer,
 *default 1). When this
 *  is provided the vectors are split into equal-sized spectra (all X, Y, E
 *values must still be
 *  in a single vector for input). If the X values should be the same for all
 *spectra, then DataX
 *  can just provide them once, i.e. its length can be length(DataY)/NSpec (or 1
 *longer for histograms).
 *
 *  @date 20/10/2010
 *  @author Michael Whitty
 *
 *
 */
class DLLExport CreateWorkspace : public API::Algorithm {
public:
  const std::string name() const override {
    return "CreateWorkspace";
  } ///< @return the algorithms name
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "This algorithm constructs a MatrixWorkspace when passed a vector "
           "for each of the X, Y, and E data values.";
  }

  const std::string category() const override {
    return "Utility\\Workspaces";
  } ///< @return the algorithms category
  int version() const override {
    return (1);
  } ///< @return version number of algorithm

  const std::vector<std::string> seeAlso() const override {
    return {"CreateSingleValuedWorkspace", "CreateSampleWorkspace"};
  }
  std::map<std::string, std::string> validateInputs() override;

protected:
  Parallel::ExecutionMode getParallelExecutionMode(
      const std::map<std::string, Parallel::StorageMode> &storageModes)
      const override;

private:
  /// Initialise the Algorithm (declare properties)
  void init() override;
  /// Execute the Algorithm
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid
#endif // MANTID_ALGORITHMS_CREATEWORKSPACE_H_
