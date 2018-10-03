// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_DELETELOG_H_
#define MANTID_ALGORITHMS_DELETELOG_H_

#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {

class DLLExport DeleteLog : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Removes a named log from a run";
  }

  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"AddSampleLog", "RenameLog"};
  }
  const std::string category() const override;

private:
  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_DELETELOG_H_ */
