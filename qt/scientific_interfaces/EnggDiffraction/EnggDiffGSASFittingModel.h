#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_GSASFITTINGMODEL_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_GSASFITTINGMODEL_H_

#include "DllConfig.h"
#include "IEnggDiffGSASFittingModel.h"
#include "RunMap.h"

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_ENGGDIFFRACTION_DLL EnggDiffGSASFittingModel
    : public IEnggDiffGSASFittingModel {

public:
  bool doPawleyRefinement(const RunLabel &runLabel,
                          const std::string &instParamFile,
                          const std::vector<std::string> &phaseFiles,
                          const std::string &pathToGSASII,
                          const std::string &GSASIIProjectFile,
                          const double dMin,
                          const double negativeWeight) override;

  bool doRietveldRefinement(const RunLabel &runLabel,
                            const std::string &instParamFile,
                            const std::vector<std::string> &phaseFiles,
                            const std::string &pathToGSASII,
                            const std::string &GSASIIProjectFile) override;

  boost::optional<Mantid::API::MatrixWorkspace_sptr>
  getFittedPeaks(const RunLabel &runLabel) const override;

  boost::optional<Mantid::API::MatrixWorkspace_sptr>
  getFocusedWorkspace(const RunLabel &runLabel) const override;

  boost::optional<Mantid::API::ITableWorkspace_sptr>
  getLatticeParams(const RunLabel &runLabel) const override;

  std::vector<RunLabel> getRunLabels() const override;

  boost::optional<double> getRwp(const RunLabel &runLabel) const override;

  bool hasFittedPeaksForRun(const RunLabel &runLabel) const override;

  bool loadFocusedRun(const std::string &filename) override;

protected:
  /// The following methods are marked as protected so that they can be exposed
  /// by a helper class in the tests

  // Add a workspace to the fitted peaks map
  void addFittedPeaks(const RunLabel &runLabel,
                      Mantid::API::MatrixWorkspace_sptr ws);

  /// Add a workspace to the focused workspace map
  void addFocusedRun(const RunLabel &runLabel,
                     Mantid::API::MatrixWorkspace_sptr ws);

  /// Add a lattice parameter table to the map
  void addLatticeParams(const RunLabel &runLabel,
                        Mantid::API::ITableWorkspace_sptr table);

  /// Add an rwp value to the rwp map
  void addRwp(const RunLabel &runLabel, const double rwp);

  /// Get whether a focused run has been loaded with a given runNumber and bank
  /// ID.
  bool hasFocusedRun(const RunLabel &runLabel) const;

private:
  static constexpr double DEFAULT_PAWLEY_DMIN = 1;
  static constexpr double DEFAULT_PAWLEY_NEGATIVE_WEIGHT = 0;
  static const size_t MAX_BANKS = 2;

  RunMap<MAX_BANKS, Mantid::API::MatrixWorkspace_sptr> m_fittedPeaksMap;
  RunMap<MAX_BANKS, Mantid::API::MatrixWorkspace_sptr> m_focusedWorkspaceMap;
  RunMap<MAX_BANKS, Mantid::API::ITableWorkspace_sptr> m_latticeParamsMap;
  RunMap<MAX_BANKS, double> m_rwpMap;

  /// Add Rwp, fitted peaks workspace and lattice params table to their
  /// respective RunMaps
  void addFitResultsToMaps(const RunLabel &runLabel, const double rwp,
                           const std::string &fittedPeaksWSName,
                           const std::string &latticeParamsTableName);

  template <typename T>
  boost::optional<T> getFromRunMapOptional(const RunMap<MAX_BANKS, T> &map,
                                           const RunLabel &runLabel) const {
    if (map.contains(runLabel)) {
      return map.get(runLabel);
    }
    return boost::none;
  }

  /// Run GSASIIRefineFitPeaks
  /// Note this must be virtual so that it can be mocked out by the helper class
  /// in EnggDiffGSASFittingModelTest
  /// Returns Rwp of the fit (empty optional if fit was unsuccessful)
  virtual boost::optional<double> doGSASRefinementAlgorithm(
      Mantid::API::MatrixWorkspace_sptr inputWorkspace,
      const std::string &outputWorkspaceName,
      const std::string &latticeParamsName, const std::string &refinementMethod,
      const std::string &instParamFile,
      const std::vector<std::string> &phaseFiles,
      const std::string &pathToGSASII, const std::string &GSASIIProjectFile,
      const double dMin, const double negativeWeight);
};

} // CustomInterfaces
} // MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_GSASFITTINGMODEL_H_
