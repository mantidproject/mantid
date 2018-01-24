#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_ENGGDIFFFITTINGMODEL_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_ENGGDIFFFITTINGMODEL_H_

#include "DllConfig.h"
#include "IEnggDiffFittingModel.h"
#include "IEnggDiffractionCalibration.h"
#include "RunMap.h"

#include <array>
#include <unordered_map>

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_ENGGDIFFRACTION_DLL EnggDiffFittingModel
    : public IEnggDiffFittingModel {

public:
  Mantid::API::MatrixWorkspace_sptr
  getFocusedWorkspace(const int runNumber, const size_t bank) const override;

  Mantid::API::MatrixWorkspace_sptr
  getAlignedWorkspace(const int runNumber, const size_t bank) const override;

  Mantid::API::MatrixWorkspace_sptr
  getFittedPeaksWS(const int runNumber, const size_t bank) const override;

  Mantid::API::ITableWorkspace_sptr
  getFitResults(const int runNumber, const size_t bank) const override;

  const std::string &getWorkspaceFilename(const int runNumber,
                                          const size_t bank) const override;

  void removeRun(const int runNumber, const size_t bank) override;

  void loadWorkspaces(const std::string &filenames) override;

  std::vector<std::pair<int, size_t>> getRunNumbersAndBankIDs() const override;

  void
  setDifcTzero(const int runNumber, const size_t bank,
               const std::vector<GSASCalibrationParms> &calibParams) override;

  void enggFitPeaks(const int runNumber, const size_t bank,
                    const std::string &expectedPeaks) override;

  void saveDiffFittingAscii(const int runNumber, const size_t bank,
                            const std::string &filename) const override;

  void createFittedPeaksWS(const int runNumber, const size_t bank) override;

  size_t getNumFocusedWorkspaces() const override;

  void addAllFitResultsToADS() const override;

  void addAllFittedPeaksToADS() const override;

  bool hasFittedPeaksForRun(const int runNumber,
                            const size_t bank) const override;

protected:
  void addFocusedWorkspace(const int runNumber, const size_t bank,
                           const Mantid::API::MatrixWorkspace_sptr ws,
                           const std::string &filename);

  void addFitResults(const int runNumber, const size_t bank,
                     const Mantid::API::ITableWorkspace_sptr ws);

  void mergeTables(const Mantid::API::ITableWorkspace_sptr tableToCopy,
                   Mantid::API::ITableWorkspace_sptr targetTable) const;

private:
  static const size_t MAX_BANKS = 2;
  static const double DEFAULT_DIFC;
  static const double DEFAULT_DIFA;
  static const double DEFAULT_TZERO;
  static const std::string FOCUSED_WS_NAME;
  static const std::string FIT_RESULTS_TABLE_NAME;
  static const std::string FITTED_PEAKS_WS_NAME;

  RunMap<MAX_BANKS, Mantid::API::MatrixWorkspace_sptr> m_focusedWorkspaceMap;
  RunMap<MAX_BANKS, std::string> m_wsFilenameMap;
  RunMap<MAX_BANKS, Mantid::API::ITableWorkspace_sptr> m_fitParamsMap;
  RunMap<MAX_BANKS, Mantid::API::MatrixWorkspace_sptr> m_fittedPeaksMap;
  RunMap<MAX_BANKS, Mantid::API::MatrixWorkspace_sptr> m_alignedWorkspaceMap;

  std::string createFunctionString(
      const Mantid::API::ITableWorkspace_sptr fitFunctionParams,
      const size_t row);

  std::pair<double, double> getStartAndEndXFromFitParams(
      const Mantid::API::ITableWorkspace_sptr fitFunctionParams,
      const size_t row);

  void evaluateFunction(const std::string &function,
                        const Mantid::API::MatrixWorkspace_sptr inputWS,
                        const std::string &outputWSName, const double startX,
                        const double endX);

  void cropWorkspace(const std::string &inputWSName,
                     const std::string &outputWSName, const int startWSIndex,
                     const int endWSIndex);

  void rebinToFocusedWorkspace(const std::string &wsToRebinName,
                               const int runNumberToMatch,
                               const size_t bankToMatch,
                               const std::string &outputWSName);

  void cloneWorkspace(const Mantid::API::MatrixWorkspace_sptr inputWorkspace,
                      const std::string &outputWSName) const;

  void setDataToClonedWS(const std::string &wsToCopyName,
                         const std::string &targetWSName);

  void appendSpectra(const std::string &ws1Name,
                     const std::string &ws2Name) const;

  std::tuple<double, double, double>
  getDifcDifaTzero(Mantid::API::MatrixWorkspace_const_sptr ws);

  Mantid::API::ITableWorkspace_sptr
  createCalibrationParamsTable(Mantid::API::MatrixWorkspace_const_sptr inputWS);

  void convertFromDistribution(Mantid::API::MatrixWorkspace_sptr inputWS);

  void alignDetectors(const std::string &inputWSName,
                      const std::string &outputWSName);

  void alignDetectors(Mantid::API::MatrixWorkspace_sptr inputWS,
                      const std::string &outputWSName);

  void loadWorkspace(const std::string &filename, const std::string &wsName);

  void renameWorkspace(Mantid::API::Workspace_sptr inputWS,
                       const std::string &newName) const;

  void groupWorkspaces(const std::vector<std::string> &workspaceNames,
                       const std::string &outputWSName);

  size_t guessBankID(Mantid::API::MatrixWorkspace_const_sptr) const;
};

} // namespace CustomInterfaces
} // namespace MantidQT

#endif // MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_ENGGDIFFFITTINGMODEL_H_
