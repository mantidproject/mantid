#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_ENGGDIFFFITTINGMODEL_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_ENGGDIFFFITTINGMODEL_H_

#include "DllConfig.h"
#include "IEnggDiffFittingModel.h"
#include "IEnggDiffractionCalibration.h"

#include <array>
#include <unordered_map>

template <size_t S, typename T>
using RunMap = std::array<std::unordered_map<int, T>, S>;

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

  std::string getWorkspaceFilename(const int runNumber,
                                   const size_t bank) const override;

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

protected:
  void addFocusedWorkspace(const int runNumber, const size_t bank,
                           const Mantid::API::MatrixWorkspace_sptr ws,
                           const std::string &filename);

private:
  static const size_t MAX_BANKS = 2;
  static const double DEFAULT_DIFC;
  static const double DEFAULT_DIFA;
  static const double DEFAULT_TZERO;
  static const std::string FOCUSED_WS_NAME;
  static const std::string FIT_RESULTS_TABLE_NAME;

  RunMap<MAX_BANKS, Mantid::API::MatrixWorkspace_sptr> m_focusedWorkspaceMap;
  RunMap<MAX_BANKS, std::string> m_wsFilenameMap;
  RunMap<MAX_BANKS, Mantid::API::ITableWorkspace_sptr> m_fitParamsMap;
  RunMap<MAX_BANKS, Mantid::API::MatrixWorkspace_sptr> m_fittedPeaksMap;
  RunMap<MAX_BANKS, Mantid::API::MatrixWorkspace_sptr> m_alignedWorkspaceMap;

  std::vector<int> getAllRunNumbers() const;

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
                      const std::string &outputWSName);

  void setDataToClonedWS(const std::string &wsToCopyName,
                         const std::string &targetWSName);

  void appendSpectra(const std::string &ws1Name, const std::string &ws2Name);

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

  void renameWorkspace(Mantid::API::MatrixWorkspace_sptr inputWS,
                       const std::string &newName);

  void groupWorkspaces(const std::vector<std::string> &workspaceNames,
                       const std::string &outputWSName);
};

} // namespace CustomInterfaces
} // namespace MantidQT

#endif // MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_ENGGDIFFFITTINGMODEL_H_
