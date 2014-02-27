/*WIKI*
 Performs polarisation correction on the input workspace


 *WIKI*/

#include "MantidAlgorithms/PolarisationCorrection.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/ListValidator.h"
#include <algorithm>

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace
{

  const std::string pNRLabel()
  {
    return "PNR";
  }

  const std::string pALabel()
  {
    return "PA";
  }

  std::vector<std::string> modes()
  {
    std::vector<std::string> modes;
    modes.push_back(pALabel());
    modes.push_back(pNRLabel());
    return modes;
  }
}

namespace Mantid
{
  namespace Algorithms
  {

    // Register the algorithm into the AlgorithmFactory
    DECLARE_ALGORITHM(PolarisationCorrection)

    //----------------------------------------------------------------------------------------------
    /** Constructor
     */
    PolarisationCorrection::PolarisationCorrection()
    {
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
     */
    PolarisationCorrection::~PolarisationCorrection()
    {
    }

    //----------------------------------------------------------------------------------------------
    /// Algorithm's name for identification. @see Algorithm::name
    const std::string PolarisationCorrection::name() const
    {
      return "PolarisationCorrection";
    }
    ;

    /// Algorithm's version for identification. @see Algorithm::version
    int PolarisationCorrection::version() const
    {
      return 1;
    }
    ;

    /// Algorithm's category for identification. @see Algorithm::category
    const std::string PolarisationCorrection::category() const
    {
      return "ISIS//Reflectometry";
    }

    //----------------------------------------------------------------------------------------------
    /// Sets documentation strings for this algorithm
    void PolarisationCorrection::initDocs()
    {
      this->setWikiSummary("TODO: Enter a quick description of your algorithm.");
      this->setOptionalMessage("TODO: Enter a quick description of your algorithm.");
    }

    //----------------------------------------------------------------------------------------------
    /** Initialize the algorithm's properties.
     */
    void PolarisationCorrection::init()
    {
      declareProperty(new WorkspaceProperty<WorkspaceGroup>("InputWorkspace", "", Direction::Input),
          "An input workspace.");

      auto propOptions = modes();
      declareProperty("PolarisationAnalysis", "PA",
          boost::make_shared<StringListValidator>(propOptions),
          "What Polarisation mode will be used?\n"
          "PNR: Polarised Neutron Reflectivity mode\n"
          "PA: Full Polarisation Analysis PNR-PA");
      declareProperty(new WorkspaceProperty<WorkspaceGroup>("OutputWorkspace", "", Direction::Output),
          "An output workspace.");
    }

    //----------------------------------------------------------------------------------------------
    /** Execute the algorithm.
     */
    void PolarisationCorrection::exec()
    {
      WorkspaceGroup_sptr inWS = getProperty("InputWorkspace");
      const std::string analysisMode = getProperty("PolarisationAnalysis");
      const size_t nWorkspaces = inWS->size();
      if(analysisMode == pALabel())
      {
        if( nWorkspaces != 4)
        {
          throw std::invalid_argument("For PA analysis, input group must have 4 periods.");
        }
      }
      else if(analysisMode == pNRLabel())
      {
        if (nWorkspaces != 2)
        {
          throw std::invalid_argument("For PNR analysis, input group must have 2 periods.");
        }
      }
    }

  } // namespace Algorithms
} // namespace Mantid
