/*WIKI*



*WIKI*/

#include "MantidWorkflowAlgorithms/DgsAbsoluteUnitsReduction.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/PropertyWithValue.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace Mantid
{
  namespace WorkflowAlgorithms
  {
    // Register the algorithm into the AlgorithmFactory
    DECLARE_ALGORITHM(DgsAbsoluteUnitsReduction)

    //----------------------------------------------------------------------------------------------
    /** Constructor
     */
    DgsAbsoluteUnitsReduction::DgsAbsoluteUnitsReduction()
    {
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
     */
    DgsAbsoluteUnitsReduction::~DgsAbsoluteUnitsReduction()
    {
    }

    //----------------------------------------------------------------------------------------------
    /// Algorithm's name for identification. @see Algorithm::name
    const std::string DgsAbsoluteUnitsReduction::name() const { return "DgsAbsoluteUnitsReduction"; };

    /// Algorithm's version for identification. @see Algorithm::version
    int DgsAbsoluteUnitsReduction::version() const { return 1; };

    /// Algorithm's category for identification. @see Algorithm::category
    const std::string DgsAbsoluteUnitsReduction::category() const { return "General"; }

    //----------------------------------------------------------------------------------------------
    /// Sets documentation strings for this algorithm
    void DgsAbsoluteUnitsReduction::initDocs()
    {
      this->setWikiSummary("Process the absolute units sample.");
      this->setOptionalMessage("Process the absolute units sample.");
    }

    //----------------------------------------------------------------------------------------------
    /** Initialize the algorithm's properties.
     */
    void DgsAbsoluteUnitsReduction::init()
    {
      this->declareProperty(new WorkspaceProperty<>("InputWorkspace", "",
          Direction::Input), "The absolute units sample workspace.");
      this->declareProperty(new WorkspaceProperty<>("OutputWorkspace", "",
          Direction::Output), "An output workspace.");
      this->declareProperty("ReductionProperties", "__dgs_reduction_properties",
          Direction::Input);
    }

    //----------------------------------------------------------------------------------------------
    /** Execute the algorithm.
     */
    void DgsAbsoluteUnitsReduction::exec()
    {
      // TODO Auto-generated execute stub
    }

} // namespace WorkflowAlgorithms
} // namespace Mantid
