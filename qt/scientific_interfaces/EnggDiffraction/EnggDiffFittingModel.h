#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_ENGGDIFFFITTINGMODEL_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_ENGGDIFFFITTINGMODEL_H_

#include "DllConfig.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "IEnggDiffractionCalibration.h"

#include <array>
#include <unordered_map>
#include <vector>

template <size_t S, typename T>
using RunMap = std::array<std::unordered_map<int, T>, S>;

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_ENGGDIFFRACTION_DLL EnggDiffFittingModel {
public:
  Mantid::API::MatrixWorkspace_sptr getWorkspace(const int runNumber,
                                         const size_t bank);
  std::vector<int> getAllRunNumbers() const;
  void loadWorkspaces(const std::string &filename);
  std::vector<std::pair<int, size_t>> getRunNumbersAndBanksIDs();
  void addWorkspace(const int runNumber, const size_t bank,
                    const Mantid::API::MatrixWorkspace_sptr ws);
  std::string getWorkspaceFilename(const int runNumber, const size_t bank);

  void setDifcTzero(const int runNumber, const size_t bank,
	  const std::vector<GSASCalibrationParms> &calibParams);
  void enggFitPeaks(const int runNumber, const size_t bank,
	                const std::string &expectedPeaks);
  void saveDiffFittingAscii(const int runNumber, const size_t bank,
	  const std::string &filename);

private:
  static const size_t MAX_BANKS = 2;
  static const double DEFAULT_DIFC;
  static const double DEFAULT_DIFA;
  static const double DEFAULT_TZERO;
  static const std::string FOCUSED_WS_NAME;
  static const std::string FIT_RESULTS_TABLE_NAME;

  RunMap<MAX_BANKS, Mantid::API::MatrixWorkspace_sptr> m_wsMap;
  RunMap<MAX_BANKS, std::string> m_wsFilenameMap;
  RunMap<MAX_BANKS, Mantid::API::ITableWorkspace_sptr> m_fitResults;

  template<typename T, size_t S>
  void addToRunMap(const int runNumber, const size_t bank, RunMap<S, T> &map,
	  T itemToAdd);
  
  template<typename T, size_t S>
  T getFromRunMap(const int runNumber, const size_t bank, const RunMap<S, T> map);

  void addWorkspace(const int runNumber, const size_t bank,
	  const std::string &filename, Mantid::API::MatrixWorkspace_sptr);

  Mantid::API::ITableWorkspace_sptr getFitResults(const int runNumber,
	  const size_t bank);

  size_t guessBankID(Mantid::API::MatrixWorkspace_const_sptr) const;
};

} // namespace CustomInterfaces
} // namespace MantidQT

#endif // MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_ENGGDIFFFITTINGMODEL_H_
