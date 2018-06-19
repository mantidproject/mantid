#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_GSASFITTINGMODEL_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_GSASFITTINGMODEL_H_

#include "DllConfig.h"
#include "EnggDiffGSASFittingWorker.h"
#include "GSASIIRefineFitPeaksOutputProperties.h"
#include "IEnggDiffGSASFittingModel.h"
#include "IEnggDiffGSASFittingObserver.h"
#include "RunMap.h"

#include "MantidAPI/IAlgorithm_fwd.h"

#include <QObject>
#include <QThread>

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_ENGGDIFFRACTION_DLL EnggDiffGSASFittingModel
    : public QObject, // Must be a QObject to run GSASIIRefineFitPeaksWorker
                      // asynchronously
      public IEnggDiffGSASFittingModel {
  Q_OBJECT

  friend void EnggDiffGSASFittingWorker::doRefinements();

public:
  EnggDiffGSASFittingModel();

  ~EnggDiffGSASFittingModel();

  void setObserver(
      boost::shared_ptr<IEnggDiffGSASFittingObserver> observer) override;

  void doRefinements(
      const std::vector<GSASIIRefineFitPeaksParameters> &params) override;

  boost::optional<Mantid::API::ITableWorkspace_sptr>
  getLatticeParams(const RunLabel &runLabel) const override;

  boost::optional<double> getGamma(const RunLabel &runLabel) const override;

  boost::optional<double> getRwp(const RunLabel &runLabel) const override;

  boost::optional<double> getSigma(const RunLabel &runLabel) const override;

  bool hasFitResultsForRun(const RunLabel &runLabel) const override;

  Mantid::API::MatrixWorkspace_sptr
  loadFocusedRun(const std::string &filename) const override;

  void saveRefinementResultsToHDF5(
      const Mantid::API::IAlgorithm_sptr successfulAlgorithm,
      const std::vector<GSASIIRefineFitPeaksOutputProperties> &
          refinementResultSets,
      const std::string &filename) const override;

protected:
  /// The following methods are marked as protected so that they can be exposed
  /// by a helper class in the tests

  /// Add a lattice parameter table to the map
  void addLatticeParams(const RunLabel &runLabel,
                        Mantid::API::ITableWorkspace_sptr table);

  /// Add a gamma value to the gamma map
  void addGamma(const RunLabel &runLabel, const double gamma);

  /// Add an rwp value to the rwp map
  void addRwp(const RunLabel &runLabel, const double rwp);

  /// Add a sigma value to the sigma map
  void addSigma(const RunLabel &runLabel, const double sigma);

protected slots:
  void processRefinementsComplete(
      Mantid::API::IAlgorithm_sptr alg,
      const std::vector<GSASIIRefineFitPeaksOutputProperties> &
          refinementResultSets);

  void processRefinementFailed(const std::string &failureMessage);

  void processRefinementSuccessful(
      Mantid::API::IAlgorithm_sptr successfulAlgorithm,
      const GSASIIRefineFitPeaksOutputProperties &refinementResults);

  void processRefinementCancelled();

private:
  static constexpr double DEFAULT_PAWLEY_DMIN = 1;
  static constexpr double DEFAULT_PAWLEY_NEGATIVE_WEIGHT = 0;
  static const size_t MAX_BANKS = 2;

  RunMap<MAX_BANKS, double> m_gammaMap;
  RunMap<MAX_BANKS, Mantid::API::ITableWorkspace_sptr> m_latticeParamsMap;
  RunMap<MAX_BANKS, double> m_rwpMap;
  RunMap<MAX_BANKS, double> m_sigmaMap;

  boost::shared_ptr<IEnggDiffGSASFittingObserver> m_observer;

  std::unique_ptr<QThread> m_workerThread;

  /// Add Rwp, sigma, gamma and lattice params table to their
  /// respective RunMaps
  void
  addFitResultsToMaps(const RunLabel &runLabel, const double rwp,
                      const double sigma, const double gamma,
                      const Mantid::API::ITableWorkspace_sptr latticeParams);

  void deleteWorkerThread();

  /// Run GSASIIRefineFitPeaks
  std::pair<Mantid::API::IAlgorithm_sptr, GSASIIRefineFitPeaksOutputProperties>
  doGSASRefinementAlgorithm(const GSASIIRefineFitPeaksParameters &params);

  template <typename T>
  boost::optional<T> getFromRunMapOptional(const RunMap<MAX_BANKS, T> &map,
                                           const RunLabel &runLabel) const {
    if (map.contains(runLabel)) {
      return map.get(runLabel);
    }
    return boost::none;
  }
};

} // CustomInterfaces
} // MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_GSASFITTINGMODEL_H_
