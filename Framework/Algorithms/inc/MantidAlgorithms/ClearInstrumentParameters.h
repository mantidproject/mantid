// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_CLEARINSTRUMENTPARAMETERS_H_
#define MANTID_ALGORITHMS_CLEARINSTRUMENTPARAMETERS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {

namespace Algorithms {

/** ClearInstrumentParameters : Clear out an instrument's parameters.

  @author Harry Jeffery, ISIS, RAL
  @date 30/7/2014
*/
class DLLExport ClearInstrumentParameters : public API::Algorithm {
public:
  const std::string name() const override;
  const std::string summary() const override;
  const std::string category() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"CopyInstrumentParameters"};
  }

private:
  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_CLEARINSTRUMENTPARAMETERS_H_ */
