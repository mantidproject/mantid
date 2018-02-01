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
  Mantid::API::MatrixWorkspace_sptr
  doPawleyRefinement(const Mantid::API::MatrixWorkspace_sptr inputWS,
                     const RunLabel &runLabel, const std::string &instParamFile,
                     const std::vector<std::string> &phaseFiles,
                     const std::string &pathToGSASII,
                     const std::string &GSASIIProjectFile, const double dMin,
                     const double negativeWeight) override;

  Mantid::API::MatrixWorkspace_sptr
  doRietveldRefinement(const Mantid::API::MatrixWorkspace_sptr inputWS,
                       const RunLabel &runLabel,
                       const std::string &instParamFile,
                       const std::vector<std::string> &phaseFiles,
                       const std::string &pathToGSASII,
                       const std::string &GSASIIProjectFile) override;

  boost::optional<Mantid::API::ITableWorkspace_sptr>
  getLatticeParams(const RunLabel &runLabel) const override;

  boost::optional<double> getRwp(const RunLabel &runLabel) const override;

  Mantid::API::MatrixWorkspace_sptr
  loadFocusedRun(const std::string &filename) const override;

protected:
  /// The following methods are marked as protected so that they can be exposed
  /// by a helper class in the tests

  /// Add a lattice parameter table to the map
  void addLatticeParams(const RunLabel &runLabel,
                        Mantid::API::ITableWorkspace_sptr table);

  /// Add an rwp value to the rwp map
  void addRwp(const RunLabel &runLabel, const double rwp);

private:
  static constexpr double DEFAULT_PAWLEY_DMIN = 1;
  static constexpr double DEFAULT_PAWLEY_NEGATIVE_WEIGHT = 0;
  static const size_t MAX_BANKS = 2;

  RunMap<MAX_BANKS, Mantid::API::ITableWorkspace_sptr> m_latticeParamsMap;
  RunMap<MAX_BANKS, double> m_rwpMap;

  /// Add Rwp and lattice params table to their
  /// respective RunMaps
  void addFitResultsToMaps(const RunLabel &runLabel, const double rwp,
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
  /// Returns Rwp of the fit
  virtual double doGSASRefinementAlgorithm(
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
