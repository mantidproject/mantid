// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_PERFORMINDEXOPERATIONS_H_
#define MANTID_ALGORITHMS_PERFORMINDEXOPERATIONS_H_

#include "MantidAPI/DataProcessorAlgorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Algorithms {

/** PerformIndexOperations : Crop and sum a workspace according to the parsed
  workspace index operations provided.
*/
class DLLExport PerformIndexOperations : public API::DataProcessorAlgorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Process the workspace according to the Index operations provided.";
  }

  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"ExtractSpectra"};
  }
  const std::string category() const override;

private:
  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_PERFORMINDEXOPERATIONS_H_ */
