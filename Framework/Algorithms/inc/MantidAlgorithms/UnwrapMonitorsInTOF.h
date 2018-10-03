// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_UNWRAPMONITORSINTOF_H_
#define MANTID_ALGORITHMS_UNWRAPMONITORSINTOF_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {

/** UnwrapMonitorsInTOF : Handles workspaces which contain monitors
 *  that recorded data which spills over from the previous frame.
 *  This can occur when dealing with different time regimes for detectors
 *  and monitors.
 */
class MANTID_ALGORITHMS_DLL UnwrapMonitorsInTOF : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"UnwrapMonitor", "UnwrapSNS"};
  }
  const std::string category() const override;
  const std::string summary() const override;
  std::map<std::string, std::string> validateInputs() override;

private:
  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_UNWRAPMONITORSINTOF_H_ */
