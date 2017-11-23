#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_ENGGDIFFFITTINGMODEL_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_ENGGDIFFFITTINGMODEL_H_

#include "DllConfig.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

#include <array>
#include <unordered_map>
#include <vector>

using namespace Mantid;

namespace MantidQT {
namespace CustomInterfaces {

class MANTIDQT_ENGGDIFFRACTION_DLL EnggDiffFittingModel {
public:
  API::MatrixWorkspace_sptr getWorkspace(const int runNumber,
                                         const size_t bank);
  std::vector<int> getAllRunNumbers() const;
  void loadWorkspaces(const std::string &filename);
  std::vector<std::pair<int, size_t>> getRunNumbersAndBanksIDs();
  void addWorkspace(const int runNumber, const size_t bank,
                    const API::MatrixWorkspace_sptr ws);
  std::string getWorkspaceFilename(const int runNumber, const size_t bank);

private:
  static const size_t MAX_BANKS = 2;
  static const std::string FOCUSED_WS_NAME;
  std::array<std::unordered_map<int, API::MatrixWorkspace_sptr>, MAX_BANKS>
      m_wsMap;
  std::array<std::unordered_map<int, std::string>, MAX_BANKS> m_wsFilenameMap;

  size_t guessBankID(API::MatrixWorkspace_const_sptr) const;
  void addWorkspace(const int runNumber, const size_t bank, 
	  const std::string &filename, API::MatrixWorkspace_sptr ws);
};

} // namespace CustomInterfaces
} // namespace MantidQT

#endif // MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_ENGGDIFFFITTINGMODEL_H_
