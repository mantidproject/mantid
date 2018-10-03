// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_EXRACTMEMBERS_H_
#define MANTID_ALGORITHMS_EXRACTMEMBERS_H_

#include "MantidAPI/DataProcessorAlgorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"

namespace Mantid {
namespace Algorithms {

/** ExtractQENSMembers : Extracts the fit members from a QENS fit
*/
class DLLExport ExtractQENSMembers : public API::DataProcessorAlgorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;
  std::map<std::string, std::string> validateInputs() override;

  std::vector<Mantid::API::MatrixWorkspace_sptr> getInputWorkspaces() const;

  std::vector<double>
  getQValues(const std::vector<Mantid::API::MatrixWorkspace_sptr> &workspaces);

  std::vector<std::string>
  getAxisLabels(Mantid::API::MatrixWorkspace_sptr workspace,
                size_t axisIndex) const;

  std::vector<std::string>
  renameConvolvedMembers(const std::vector<std::string> &members,
                         const std::vector<std::string> &newNames) const;

  Mantid::API::MatrixWorkspace_sptr
  extractSpectrum(Mantid::API::MatrixWorkspace_sptr inputWS, size_t spectrum);

  Mantid::API::MatrixWorkspace_sptr
  appendSpectra(Mantid::API::MatrixWorkspace_sptr inputWS,
                Mantid::API::MatrixWorkspace_sptr spectraWorkspace);

  Mantid::API::WorkspaceGroup_sptr
  groupWorkspaces(const std::vector<std::string> &workspaceNames);

  std::vector<Mantid::API::MatrixWorkspace_sptr>
  createMembersWorkspaces(Mantid::API::MatrixWorkspace_sptr initialWS,
                          const std::vector<std::string> &members);

  void appendToMembers(Mantid::API::MatrixWorkspace_sptr resultWS,
                       std::vector<Mantid::API::MatrixWorkspace_sptr> &members);

  void setNumericAxis(
      const std::vector<Mantid::API::MatrixWorkspace_sptr> &workspaces,
      const std::vector<double> &values, size_t axisIndex) const;

  std::vector<std::string> addMembersToADS(
      const std::vector<std::string> &members,
      const std::vector<Mantid::API::MatrixWorkspace_sptr> &memberWorkspaces,
      const std::string &outputWSName);
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_EXRACTMEMBERS_H_ */
