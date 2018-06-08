#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_ENGGDIFFGSASFITTINGPRESENTER_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_ENGGDIFFGSASFITTINGPRESENTER_H_

#include "DllConfig.h"
#include "GSASIIRefineFitPeaksOutputProperties.h"
#include "IEnggDiffGSASFittingModel.h"
#include "IEnggDiffGSASFittingPresenter.h"
#include "IEnggDiffGSASFittingView.h"
#include "IEnggDiffMultiRunFittingWidgetPresenter.h"
#include "IEnggDiffractionParam.h"

#include "MantidAPI/MatrixWorkspace_fwd.h"

#include <boost/shared_ptr.hpp>
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {

// needs to be dll-exported for the tests
class MANTIDQT_ENGGDIFFRACTION_DLL EnggDiffGSASFittingPresenter
    : public IEnggDiffGSASFittingPresenter {

public:
  EnggDiffGSASFittingPresenter(
      std::unique_ptr<IEnggDiffGSASFittingModel> model,
      IEnggDiffGSASFittingView *view,
      boost::shared_ptr<IEnggDiffMultiRunFittingWidgetPresenter> multiRunWidget,
      boost::shared_ptr<IEnggDiffractionParam> mainSettings);

  EnggDiffGSASFittingPresenter(EnggDiffGSASFittingPresenter &&other) = default;

  EnggDiffGSASFittingPresenter &
  operator=(EnggDiffGSASFittingPresenter &&other) = default;

  ~EnggDiffGSASFittingPresenter() override;

  void notify(IEnggDiffGSASFittingPresenter::Notification notif) override;

  void notifyRefinementsComplete(
      Mantid::API::IAlgorithm_sptr alg,
      const std::vector<GSASIIRefineFitPeaksOutputProperties> &
          refinementResultSets) override;

  void notifyRefinementSuccessful(
      const Mantid::API::IAlgorithm_sptr alg,
      const GSASIIRefineFitPeaksOutputProperties &refinementResults) override;

  void notifyRefinementFailed(const std::string &failureMessage) override;

  void notifyRefinementCancelled() override;

private:
  void processDoRefinement();
  void processLoadRun();
  void processRefineAll();
  void processSelectRun();
  void processShutDown();
  void processStart();

  /// Collect GSASIIRefineFitPeaks parameters for all runs loaded in
  std::vector<GSASIIRefineFitPeaksParameters> collectAllInputParameters() const;

  /// Collect GSASIIRefineFitPeaks input parameters for a given run from the
  /// presenter's various children
  GSASIIRefineFitPeaksParameters
  collectInputParameters(const RunLabel &runLabel,
                         const Mantid::API::MatrixWorkspace_sptr ws) const;

  /**
   Perform refinements on a number of runs
   @param params Input parameters for each run to pass to GSASIIRefineFitPeaks
   */
  void doRefinements(const std::vector<GSASIIRefineFitPeaksParameters> &params);

  /**
   Overplot fitted peaks for a run, and display lattice parameters and Rwp in
   the view
   @param runLabel Run number and bank ID of the run to display
  */
  void displayFitResults(const RunLabel &runLabel);

  std::unique_ptr<IEnggDiffGSASFittingModel> m_model;

  boost::shared_ptr<IEnggDiffMultiRunFittingWidgetPresenter> m_multiRunWidget;

  boost::shared_ptr<IEnggDiffractionParam> m_mainSettings;

  IEnggDiffGSASFittingView *m_view;

  bool m_viewHasClosed;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_ENGGDIFFGSASFITTINGPRESENTER_H_
