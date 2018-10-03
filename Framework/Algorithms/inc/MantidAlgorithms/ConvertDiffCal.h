// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_CONVERTDIFFCAL_H_
#define MANTID_ALGORITHMS_CONVERTDIFFCAL_H_

#include "MantidAPI/ParallelAlgorithm.h"
#include "MantidKernel/System.h"
namespace Mantid {
namespace Algorithms {

/** ConvertDiffCal : TODO: DESCRIPTION
 */
class DLLExport ConvertDiffCal : public API::ParallelAlgorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"CalculateDIFC"};
  }
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_CONVERTDIFFCAL_H_ */
