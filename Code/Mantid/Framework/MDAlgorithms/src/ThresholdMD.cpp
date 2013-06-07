/*WIKI*
 Threshold an MDHistoWorkspace to overwrite values below or above the defined threshold.
 *WIKI*/

#include "MantidMDAlgorithms/ThresholdMD.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::MDEvents;

namespace Mantid
{
  namespace MDAlgorithms
  {

    // Register the algorithm into the AlgorithmFactory
    DECLARE_ALGORITHM(ThresholdMD)

    //----------------------------------------------------------------------------------------------
    /** Constructor
     */
    ThresholdMD::ThresholdMD()
    {
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
     */
    ThresholdMD::~ThresholdMD()
    {
    }

    //----------------------------------------------------------------------------------------------
    /// Algorithm's name for identification. @see Algorithm::name
    const std::string ThresholdMD::name() const
    {
      return "ThresholdMD";
    }
    ;

    /// Algorithm's version for identification. @see Algorithm::version
    int ThresholdMD::version() const
    {
      return 1;
    }
    ;

    /// Algorithm's category for identification. @see Algorithm::category
    const std::string ThresholdMD::category() const
    {
      return "MDAlgorithms";
    }

    //----------------------------------------------------------------------------------------------
    /// Sets documentation strings for this algorithm
    void ThresholdMD::initDocs()
    {
      this->setWikiSummary("Threshold an MDHistoWorkspace.");
      this->setOptionalMessage(this->getWikiSummary());
    }

    //----------------------------------------------------------------------------------------------
    /** Initialize the algorithm's properties.
     */
    void ThresholdMD::init()
    {
      declareProperty(new WorkspaceProperty<IMDHistoWorkspace>("InputWorkspace", "", Direction::Input),
          "An input workspace.");

      std::vector<std::string> propOptions;
      propOptions.push_back("Less Than");
      propOptions.push_back("Greater Than");

      declareProperty("Condition", "Less Than", boost::make_shared<StringListValidator>(propOptions),
          "Selected threshold condition?");

      declareProperty("CurrentValue", 0.0, "Comparator value used by the Condition.");

      declareProperty("OverwriteWithZero", true,
          "Flag for enabling overwriting with a custom value. Defaults to overwrite signals with zeros.");

      declareProperty("CustomOverwriteValue", 0,
          "Custom overwrite value for the signal. Defaults to zero.");
      setPropertySettings("CustomOverwriteValue",
          new EnabledWhenProperty("OverwriteWithZero", IS_NOT_DEFAULT));

      declareProperty(new WorkspaceProperty<IMDHistoWorkspace>("OutputWorkspace","",Direction::Output), "Output thresholded workspace.");
    }

    //----------------------------------------------------------------------------------------------
    /** Execute the algorithm.
     */
    void ThresholdMD::exec()
    {
    }

  } // namespace MDAlgorithms
} // namespace Mantid
