// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CRYSTAL_HASUB_H_
#define MANTID_CRYSTAL_HASUB_H_

#include "MantidAPI/Algorithm.h"
#include "MantidCrystal/ClearUB.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Crystal {

/** HasUB : Determine if a workspace has a UB matrix on any of it's samples.
 Returns True if one is found. Returns false if none can be found, or if the
 * workspace type is incompatible.
*/
class DLLExport HasUB : public ClearUB {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Determines whether the workspace has one or more UB Matrix";
  }

  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"SetUB", "ClearUB"};
  }
  const std::string category() const override;

private:
  void init() override;
  void exec() override;
};

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_HASUB_H_ */
