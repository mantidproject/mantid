// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_PROCESSINDIRECTFITPARAMETERS_H_
#define MANTID_ALGORITHMS_PROCESSINDIRECTFITPARAMETERS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Algorithms {

/** ProcessIndirectFitParameters : Convert a parameter table output by
  PlotPeakByLogValue to a MatrixWorkspace. This will make a spectrum for each
  parameter name using the x_column vairable as the x values for the spectrum.

  @author Elliot Oram, ISIS, RAL
  @date 12/08/2015
*/
class DLLExport ProcessIndirectFitParameters : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;
  std::size_t getStartRow() const;
  std::size_t getEndRow(std::size_t maximum) const;
};
} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_PROCESSINDIRECTFITPARAMETERS_H_ */