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
      "<b>Sample runs to be processed. Runs may be given as run "
      "numbers or workspace names. Multiple runs may be "
      "added together by separating them with a '+'. <br "
      "/><br /><b>Example:</b> <samp>1234+1235+1236</samp>");
  whitelist.addElement("Expected Peak(s)", "ExpectedPeaks",
                       "TableWorkspace or "
                       "WorkspaceGroup with expected "
                       "peaks used for indexing.");
  whitelist.addElement("Profile Function", "ProfileFunction",
                       "Peak function to fit peaks to. Allowed values are "
                       "'Gaussian', 'Lorentzian', 'Pseudo Voigt' and 'Voigt'");
  whitelist.addElement("Number of Peaks", "MaximumPeakNumber",
                       "Maximum number of peaks to process in the analysis.");

  /* The main reduction algorithm */
  DataProcessorProcessingAlgorithm processor(
      "PoldiDataAnalysis", std::vector<std::string>{"Poldi_"},
      std::set<std::string>{"InputWorkspace", "ExpectedPeaks",
                            "ProfileFunction", "OutputWorkspace"});

  // Pre-processing instructions as a map:
  // Keys are the column names
  // Values are the associated pre-processing algorithms
  std::map<std::string, DataProcessorPreprocessingAlgorithm> preprocessMap = {
      {"Run(s)", DataProcessorPreprocessingAlgorithm()}};

  // How to post-process groups
  DataProcessorPostprocessingAlgorithm postprocessor(
      "GroupWorkspaces", "PoldiGroup_",
      std::set<std::string>{"InputWorkspaces", "OutputWorkspaces"});

  return boost::make_shared<GenericDataProcessorPresenter>(
      whitelist, preprocessMap, processor, postprocessor);
}
}
}
