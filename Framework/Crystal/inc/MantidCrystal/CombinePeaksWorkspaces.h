// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CRYSTAL_COMBINEPEAKSWORKSPACES_H_
#define MANTID_CRYSTAL_COMBINEPEAKSWORKSPACES_H_

#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Crystal {

/** An algorithm that combines the sets of peaks in two peaks workspaces.
    Optionally, peaks considered 'identical' are combined. Such peaks are those
   that are within
    the given tolerance in all components of Q. The peak from the first/left
   workspace is kept.
    Note that it is possible for multiple peaks in the rhs to be matched to a
   given lhs peak if
    the tolerance is too large/the peaks are close together.
*/
class DLLExport CombinePeaksWorkspaces : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Combines the sets of peaks in two peaks workspaces, optionally "
           "omitting duplicates.";
  }

  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"CreatePeaksWorkspace"};
  }
  const std::string category() const override;

private:
  void init() override;
  void exec() override;
};

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_COMBINEPEAKSWORKSPACES_H_ */
