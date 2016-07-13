#include "MantidQtCustomInterfaces/Poldi/PoldiGenericDataProcessorPresenterFactory.h"

namespace MantidQt {
namespace CustomInterfaces {

using namespace MantidQt::MantidWidgets;

/**
* Creates a Poldi Data Processor Presenter
*/
boost::shared_ptr<GenericDataProcessorPresenter>
PoldiGenericDataProcessorPresenterFactory::create() {

  /* The white list defining number of columns, their names and how they relate
  * to the algorithm's input properties */
  DataProcessorWhiteList whitelist;
  whitelist.addElement(
      "Run(s)", "InputWorkspace",
      "Workspace with POLDI 2D-data and valid instrument definition.");

  whitelist.addElement("Expected peak(s)", "ExpectedPeaks",
                       "TableWorkspace or "
                       "WorkspaceGroup with expected "
                       "peaks used for indexing.");
  whitelist.addElement("Maximum number of peaks", "MaximumPeakNumber",
                       "Maximum number of peaks to process in the analysis.");
  whitelist.addElement("Profile function", "ProfileFunction",
                       "Peak function to fit peaks to. Allowed values are "
                       "'Gaussian', 'Lorentzian', 'Pseudo Voigt' and 'Voigt'");
  whitelist.addElement("Pawley fit", "PawleyFit",
                       "If enabled, the 2D-fit refines lattice parameters "
                       "according to the crystal structures of the workspaces "
                       "with the expected peaks.");
  whitelist.addElement("Plot result", "PlotResult",
                       "If activated, plot the sum of residuals and calculated "
                       "spectrum together with the theoretical spectrum and "
                       "the residuals.");

  /* The main reduction algorithm */
  DataProcessorProcessingAlgorithm processor(
      "PoldiDataAnalysis", std::vector<std::string>{"Poldi_"},
      std::set<std::string>{"InputWorkspace", "ExpectedPeaks",
                            "ProfileFunction", "OutputWorkspace"});

  // How to post-process groups
  DataProcessorPostprocessingAlgorithm postprocessor(
      "GroupWorkspaces", "PoldiGroup_",
      std::set<std::string>{"InputWorkspaces", "OutputWorkspaces"});

  return boost::make_shared<GenericDataProcessorPresenter>(whitelist, processor,
                                                           postprocessor);
}
}
}
