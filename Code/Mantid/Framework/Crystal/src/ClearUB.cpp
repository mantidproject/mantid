/*WIKI*

 Clears the OrientedLattice of each ExperimentInfo attached to the intput [[Workspace]]. Works with both single ExperimentInfos and MultipleExperimentInfo instances.

 *WIKI*/

#include "MantidCrystal/ClearUB.h"
#include "MantidAPI/MultipleExperimentInfos.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
  namespace Crystal
  {

    // Register the algorithm into the AlgorithmFactory
    DECLARE_ALGORITHM(ClearUB)

    //----------------------------------------------------------------------------------------------
    /** Constructor
     */
    ClearUB::ClearUB()
    {
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
     */
    ClearUB::~ClearUB()
    {
    }

    //----------------------------------------------------------------------------------------------
    /// Algorithm's name for identification. @see Algorithm::name
    const std::string ClearUB::name() const
    {
      return "ClearUB";
    }
    ;

    /// Algorithm's version for identification. @see Algorithm::version
    int ClearUB::version() const
    {
      return 1;
    }
    ;

    /// Algorithm's category for identification. @see Algorithm::category
    const std::string ClearUB::category() const
    {
      return "Crystal";
    }

    //----------------------------------------------------------------------------------------------
    /// Sets documentation strings for this algorithm
    void ClearUB::initDocs()
    {
      this->setWikiSummary("Clears the UB by removing the oriented lattice from the sample.");
      this->setOptionalMessage(this->getWikiSummary());
    }

    //----------------------------------------------------------------------------------------------
    /** Initialize the algorithm's properties.
     */
    void ClearUB::init()
    {
      declareProperty(new WorkspaceProperty<Workspace>("Workspace", "", Direction::InOut),
          "Workspace to clear the UB from.");
      declareProperty(new PropertyWithValue<bool>("DryRun", false, Direction::Input),
          "Dry run mode, will complete processing without error, and without removing any UB. Use in conjunction with DoesClear output property.");
      declareProperty(new PropertyWithValue<bool>("DoesClear", "", Direction::Output),
          "Indicates action performed, or predicted to perform if DryRun.");
    }

    /**
     * Clear the Oriented Lattice from a single experiment info.
     * @param experimentInfo : Experiment info to clear.
     * @param dryRun : Flag to indicate that this is a dry run, and that no clearing should take place.
     * @return true only if the UB was cleared.
     */
    bool ClearUB::clearSingleExperimentInfo(ExperimentInfo * const experimentInfo,
        const bool dryRun) const
    {
      bool doesClear = false;
      Sample& sampleObject = experimentInfo->mutableSample();
      if (!sampleObject.hasOrientedLattice())
      {
        this->g_log.notice("Workspace has no oriented lattice to clear.");
      }
      else
      {
        // Only actually clear the orientedlattice if this is NOT a dry run.
        if (!dryRun)
        {
          sampleObject.clearOrientedLattice();
        }
        doesClear = true;
      }
      return doesClear;
    }

    //----------------------------------------------------------------------------------------------
    /** Execute the algorithm.
     */
    void ClearUB::exec()
    {
      Workspace_sptr ws = getProperty("Workspace");
      const bool dryRun = getProperty("DryRun");
      bool doesClear = false;
      ExperimentInfo_sptr experimentInfo = boost::dynamic_pointer_cast<ExperimentInfo>(ws);
      if (experimentInfo)
      {
        doesClear = clearSingleExperimentInfo(experimentInfo.get(), dryRun);
      }
      else
      {
        MultipleExperimentInfos_sptr experimentInfos = boost::dynamic_pointer_cast<
            MultipleExperimentInfos>(ws);
        if (!experimentInfos)
        {
          if (!dryRun)
          {
            throw std::invalid_argument(
                "Input workspace is neither of type ExperimentInfo or MultipleExperimentInfo, cannot process.");
          }
        }
        else
        {
          const uint16_t nInfos = experimentInfos->getNumExperimentInfo();
          for (uint16_t i = 0; i < nInfos; ++i)
          {
            ExperimentInfo_sptr info = experimentInfos->getExperimentInfo(i);
            doesClear = clearSingleExperimentInfo(info.get(), dryRun) || doesClear;
          }
        }
      }
      this->setProperty("DoesClear", doesClear);
    }

  } // namespace Crystal
} // namespace Mantid
