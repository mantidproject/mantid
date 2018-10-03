// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CRYSTAL_CLEARUB_H_
#define MANTID_CRYSTAL_CLEARUB_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {

namespace API {
class ExperimentInfo;
}

namespace Crystal {

/** ClearUB : Clear the UB matrix from a workspace by removing the oriented
  lattice.
*/
class DLLExport ClearUB : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Clears the UB by removing the oriented lattice from the sample.";
  }

  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"SetUB", "HasUB"};
  }
  const std::string category() const override;

protected:
  bool doExecute(Mantid::API::Workspace *const ws, bool dryRun);

private:
  bool
  clearSingleExperimentInfo(Mantid::API::ExperimentInfo *const experimentInfo,
                            const bool dryRun) const;
  void init() override;
  void exec() override;
};

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_CLEARUB_H_ */
