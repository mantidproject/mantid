#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_ENGGDIFFGSASFITTINGPRESENTER_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_ENGGDIFFGSASFITTINGPRESENTER_H_

#include "DllConfig.h"
#include "IEnggDiffGSASFittingModel.h"
#include "IEnggDiffGSASFittingPresenter.h"
#include "IEnggDiffGSASFittingView.h"

#include "MantidAPI/MatrixWorkspace_fwd.h"

#include <memory>

namespace MantidQt {
namespace CustomInterfaces {

// needs to be dll-exported for the tests
class MANTIDQT_ENGGDIFFRACTION_DLL EnggDiffGSASFittingPresenter
    : public IEnggDiffGSASFittingPresenter {

public:
  EnggDiffGSASFittingPresenter(std::unique_ptr<IEnggDiffGSASFittingModel> model,
                               std::unique_ptr<IEnggDiffGSASFittingView> view);

  ~EnggDiffGSASFittingPresenter() override;

  void notify(IEnggDiffGSASFittingPresenter::Notification notif) override;

private:
  void processDoRefinement();
  void processLoadRun();
  void processSelectRun();
  void processShutDown();
  void processStart();

  /**
   Perform a Pawley refinement on a run
   @param runNumber The run number of the run
   @param bank The bank ID of the run
   @param instParamFile The instrument parameter file name (.prm) to use for
   refinement
   @param phaseFiles Vector of file paths to phases to use in refinement
   @param pathToGSASII Location of the directory containing GSASIIscriptable.py
   (and GSAS-II executables)
   @param GSASIIProjectFile Location to save the .gpx project to
   @return Whether the refinement was successful
   */
  bool doPawleyRefinement(const int runNumber, const size_t bank,
                          const std::string &instParamFile,
                          const std::vector<std::string> &phaseFiles,
                          const std::string &pathToGSASII,
                          const std::string &GSASIIProjectFile);

  /**
   Perform a Rietveld refinement on a run
   @param runNumber The run number of the run
   @param bank The bank ID of the run
   @param instParamFile The instrument parameter file name (.prm) to use for
   refinement
   @param phaseFiles Vector of file paths to phases to use in refinement
   @param pathToGSASII Location of the directory containing GSASIIscriptable.py
   (and GSAS-II executables)
   @param GSASIIProjectFile Location to save the .gpx project to
   @return Whether the refinement was successful
   */
  bool doRietveldRefinement(const int runNumber, const size_t bank,
                            const std::string &instParamFile,
                            const std::vector<std::string> &phaseFiles,
                            const std::string &pathToGSASII,
                            const std::string &GSASIIProjectFile);

  /**
   Overplot fitted peaks for a run, and display lattice parameters and Rwp in
   the view
   @param runNumber The run number of the run
   @param bank The bank ID of the run
  */
  void displayFitResults(const int runNumber, const size_t bank);

  /**
   Update the view with the data from a run, and refinement results if they are
   available and the user has selected to show them
   @param runNumber Run number of the run
   @param bank Bank ID of the run
  */
  void updatePlot(const int runNumber, const size_t bank);

  std::unique_ptr<IEnggDiffGSASFittingModel> m_model;

  std::unique_ptr<IEnggDiffGSASFittingView> m_view;

  bool m_viewHasClosed;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_ENGGDIFFGSASFITTINGPRESENTER_H_
