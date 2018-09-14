#ifndef MANTID_MUON_MUONGROUPINGCOUNTS_H_
#define MANTID_MUON_MUONGROUPINGCOUNTS_H_

#include "MantidAPI/Algorithm.h"

using namespace Mantid::API;

namespace Mantid {
namespace Muon {

class DLLExport MuonGroupingCounts : public API::Algorithm {
public:
  MuonGroupingCounts() : API::Algorithm() {}
  ~MuonGroupingCounts() {}

  const std::string name() const override { return "MuonGroupingCounts"; }
  int version() const override { return (1); }
  const std::string category() const override { return "Muon\\DataHandling"; }
  const std::string summary() const override {
    return "Apply a grouping (summation of counts) across a set of detectors "
           "in Muon data.";
  }
  const std::vector<std::string> seeAlso() const override {
    return {"MuonProcess"};
  }

private:
  void init() override;
  void exec() override;

  void setGroupingSampleLogs(MatrixWorkspace_sptr workspace);
};

} // namespace Muon
} // namespace Mantid

#endif /* MANTID_MUON_MUONGROUPINGCOUNTS_H_ */
