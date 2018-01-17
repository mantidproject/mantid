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
  bool doPawleyRefinement(const int runNumber, const size_t bank,
                          const std::string &instParamFile,
                          const std::vector<std::string> &phaseFiles,
                          const std::string &pathToGSASII,
                          const std::string &GSASIIProjectFile,
                          const double dMin,
                          const double negativeWeight) override;

  bool doRietveldRefinement(const int runNumber, const size_t bank,
                            const std::string &instParamFile,
                            const std::vector<std::string> &phaseFiles,
                            const std::string &pathToGSASII,
                            const std::string &GSASIIProjectFile) override;

  boost::optional<Mantid::API::MatrixWorkspace_sptr>
  getFittedPeaks(const int runNumber, const size_t bank) const override;

  boost::optional<Mantid::API::MatrixWorkspace_sptr>
  getFocusedWorkspace(const int runNumber, const size_t bank) const override;

  boost::optional<Mantid::API::ITableWorkspace_sptr>
  getLatticeParams(const int runNumber, const size_t bank) const override;

  std::vector<std::pair<int, size_t>> getRunLabels() const override;

  boost::optional<double> getRwp(const int runNumber,
                                 const size_t bank) const override;

  bool hasFittedPeaksForRun(const int runNumber,
                            const size_t bank) const override;

  bool loadFocusedRun(const std::string &filename) override;

protected:
  /// The following methods are marked as protected so that they can be exposed
  /// by a helper class in the tests

  // Add a workspace to the fitted peaks map
  void addFittedPeaks(const int runNumber, const size_t bank,
                      Mantid::API::MatrixWorkspace_sptr ws);

  /// Add a workspace to the focused workspace map
  void addFocusedRun(const int runNumber, const size_t bank,
                     Mantid::API::MatrixWorkspace_sptr ws);

  /// Add a lattice parameter table to the map
  void addLatticeParams(const int runNumber, const size_t bank,
                        Mantid::API::ITableWorkspace_sptr table);

  /// Add an rwp value to the rwp map
  void addRwp(const int runNumber, const size_t bank, const double rwp);

  /// Get whether a focused run has been loaded with a given runNumber and bank
  /// ID.
  bool hasFocusedRun(const int runNumber, const size_t bank) const;

private:
  static const size_t MAX_BANKS = 2;

  RunMap<MAX_BANKS, Mantid::API::MatrixWorkspace_sptr> m_fittedPeaksMap;
  RunMap<MAX_BANKS, Mantid::API::MatrixWorkspace_sptr> m_focusedWorkspaceMap;
  RunMap<MAX_BANKS, Mantid::API::ITableWorkspace_sptr> m_latticeParamsMap;
  RunMap<MAX_BANKS, double> m_rwpMap;

  template <typename T>
  boost::optional<T> getFromRunMapOptional(const RunMap<MAX_BANKS, T> &map,
                                           const int runNumber,
                                           const size_t bank) const {
    if (map.contains(runNumber, bank)) {
      return map.get(runNumber, bank);
    }
    return boost::none;
  }
};

} // CustomInterfaces
} // MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_GSASFITTINGMODEL_H_
