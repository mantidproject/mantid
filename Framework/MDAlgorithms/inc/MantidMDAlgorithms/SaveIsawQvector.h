// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDALGORITHMS_SAVEISAWQVECTOR_H_
#define MANTID_MDALGORITHMS_SAVEISAWQVECTOR_H_

#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidMDAlgorithms//MDWSDescription.h"

namespace Mantid {
namespace MDAlgorithms {

/** SaveIsawQvector
 */
class DLLExport SaveIsawQvector : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Save an event workspace as an ISAW Q-vector file";
  }

  int version() const override;
  const std::string category() const override;

private:
  void init() override;
  void exec() override;

  MDWSDescription m_targWSDescr;

  void initTargetWSDescr(DataObjects::EventWorkspace_sptr wksp);
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_SAVEISAWQVECTOR_H_ */
