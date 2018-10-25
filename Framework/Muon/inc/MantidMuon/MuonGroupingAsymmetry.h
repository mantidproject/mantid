#ifndef MANTID_MUON_MUONGROUPINGASYMMETRY_H_
#define MANTID_MUON_MUONGROUPINGASYMMETRY_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"

using namespace Mantid::API;

namespace Mantid {
namespace Muon {

class DLLExport MuonGroupingAsymmetry : public API::Algorithm {
public:
  MuonGroupingAsymmetry() : API::Algorithm() {}
  ~MuonGroupingAsymmetry() {}

  const std::string name() const override { return "MuonGroupingAsymmetry"; }
  int version() const override { return (1); }
  const std::string category() const override { return "Muon\\DataHandling"; }
  const std::string summary() const override {
    return "Apply an estimate of the asymmetry to a particular detector in "
           "Muon data.";
  }
  const std::vector<std::string> seeAlso() const override {
    return {"MuonProcess"};
  }

private:
  void init() override;
  void exec() override;

  std::map<std::string, std::string> validateInputs() override;

  void addGroupingAsymmetrySampleLogs(MatrixWorkspace_sptr workspace);
};

} // namespace Muon
} // namespace Mantid

#endif /* MANTID_MUON_MUONGROUPINGASYMMETRY_H_ */
