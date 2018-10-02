#ifndef MANTID_MUON_MUONPAIRINGASYMMETRY_H_
#define MANTID_MUON_MUONPAIRINGASYMMETRY_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"

using namespace Mantid::API;

namespace Mantid {
namespace Muon {

class DLLExport MuonPairingAsymmetry : public API::Algorithm {
public:
  MuonPairingAsymmetry() : API::Algorithm() {}
  ~MuonPairingAsymmetry() {}

  const std::string name() const override { return "MuonPairingAsymmetry"; }
  int version() const override { return (1); }
  const std::string category() const override { return "Muon\\DataHandling"; }
  const std::string summary() const override { return "."; }
  const std::vector<std::string> seeAlso() const override {
    return {"MuonProcess"};
  }

private:
  void init() override;
  void exec() override;

  MatrixWorkspace_sptr
  createPairWorkspaceFromGroupWorkspaces(MatrixWorkspace_sptr inputWS1,
                                         MatrixWorkspace_sptr inputWS2,
                                         const double &alpha);

  /// Perform an asymmetry calculation
  MatrixWorkspace_sptr asymmetryCalc(MatrixWorkspace_sptr inputWS,
                                     const double &alpha);

  /// Execute the algorithm if "SpecifyGroupsManually" is checked
  MatrixWorkspace_sptr execSpecifyGroupsManually(WorkspaceGroup_sptr inputWS);

  void setPairAsymmetrySampleLogs(MatrixWorkspace_sptr workspace);
};

} // namespace Muon
} // namespace Mantid

#endif /* MANTID_MUON_MUONPAIRINGASYMMETRY_H_ */
