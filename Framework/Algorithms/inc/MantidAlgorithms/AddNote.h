// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_ADDNOTE_H_
#define MANTID_ALGORITHMS_ADDNOTE_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/DeprecatedAlgorithm.h"
#include "MantidAPI/Run.h"

namespace Mantid {
namespace Algorithms {

/**
  An Algorithm that adds a timestamped note to a workspace.

  @author Elliot Oram, ISIS, RAL
  @date 17/07/2015
*/
class DLLExport AddNote : public API::Algorithm,
                          public API::DeprecatedAlgorithm {
public:
  AddNote();

  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"Comment"};
  }
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;

  /// Remove an existing log of the given name
  void removeExisting(API::MatrixWorkspace_sptr &, const std::string &);
  /// Create or update the named log entry
  void createOrUpdate(API::Run &, const std::string &);
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_ADDNOTE_H_ */