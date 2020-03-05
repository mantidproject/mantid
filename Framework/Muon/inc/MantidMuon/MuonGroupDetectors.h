// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_MUONGROUPDETECTORS_H_
#define MANTID_ALGORITHMS_MUONGROUPDETECTORS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidMuon/DllConfig.h"

namespace Mantid {
namespace Algorithms {

/** MuonGroupDetectors : applies detector grouping to a workspace. (Muon
  version)
*/
class MANTID_MUON_DLL MuonGroupDetectors : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Applies detector grouping to a workspace. (Muon version).";
  }

  int version() const override;
  const std::string category() const override;

private:
  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_MUONGROUPDETECTORS_H_ */
