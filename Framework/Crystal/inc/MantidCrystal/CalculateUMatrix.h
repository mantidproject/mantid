// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CRYSTAL_CALCULATEUMATRIX_H_
#define MANTID_CRYSTAL_CALCULATEUMATRIX_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Crystal {

/** CalculateUMatrix : Algorithm to calculate the U matrix, given lattice
  parameters and a list of peaks

  @author Andrei Savici, ORNL
  @date 2011-08-05
*/
class DLLExport CalculateUMatrix : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "CalculateUMatrix"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Calculate the U matrix from a peaks workspace, given lattice "
           "parameters.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override { return {"SetUB"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "Crystal\\UBMatrix"; }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
};

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_CALCULATEUMATRIX_H_ */
