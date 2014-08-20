#include "MantidAPI/MultiPeriodGroupWorker.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/Property.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/AlgorithmManager.h"

using namespace Mantid::Kernel;

namespace Mantid
{
  namespace API
  {

    //----------------------------------------------------------------------------------------------
    /** Constructor
     */
    MultiPeriodGroupWorker::MultiPeriodGroupWorker() :
        m_workspacePropertyName("")
    {
    }

    /**
     * Constructor
     * @param workspacePropertyName : Property name to treat as source of multiperiod workspaces.
     */
    MultiPeriodGroupWorker::MultiPeriodGroupWorker(const std::string& workspacePropertyName) :
        m_workspacePropertyName(workspacePropertyName)
    {
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
     */
    MultiPeriodGroupWorker::~MultiPeriodGroupWorker()
    {
    }

    /**
     * Try to add the input workspace to the multiperiod input group list.
     * @param ws: candidate workspace
     * @param vecWorkspaceGroups: Vector of multi period workspace groups.
     */
    void MultiPeriodGroupWorker::tryAddInputWorkspaceToInputGroups(Workspace_sptr ws,
        MultiPeriodGroupWorker::VecWSGroupType& vecWorkspaceGroups) const
    {
      WorkspaceGroup_sptr inputGroup = boost::dynamic_pointer_cast<WorkspaceGroup>(ws);
      if (inputGroup)
      {
        if (inputGroup->isMultiperiod())
        {
          vecWorkspaceGroups.push_back(inputGroup);
        }
      }
    }

    MultiPeriodGroupWorker::VecWSGroupType MultiPeriodGroupWorker::findMultiPeriodGroups(
        Algorithm const * const sourceAlg) const
    {
      if (!sourceAlg->isInitialized())
      {
        throw std::invalid_argument("Algorithm must be initialized");
      }
      VecWSGroupType vecWorkspaceGroups;

      // Handles the case in which the algorithm is providing a non-workspace property as an input.
      // This is currenly the case for algorithms that take an array of strings as an input where each entry is the name of a workspace.
      if (this->useCustomWorkspaceProperty())
      {
        typedef std::vector<std::string> WorkspaceNameType;

        // Perform a check that the input property is the correct type.
        Property* inputProperty = sourceAlg->getProperty(this->m_workspacePropertyName);

        if (!dynamic_cast<ArrayProperty<std::string>*>(inputProperty))
        {
          throw std::runtime_error(
              "Support for custom input workspaces that are not string Arrays are not currently supported.");
          /*Note that we could extend this algorithm to cover other input property types if required, but we don't need that funtionality now.*/
        }

        WorkspaceNameType workspaces = sourceAlg->getProperty(this->m_workspacePropertyName);
        WorkspaceNameType::iterator it = workspaces.begin();

        // Inspect all the input workspaces in the ArrayProperty input.
        while (it != workspaces.end())
        {
          Workspace_sptr ws = AnalysisDataService::Instance().retrieve(*it);
          if (!ws)
          {
            throw Kernel::Exception::NotFoundError("Workspace", *it);
          }
          tryAddInputWorkspaceToInputGroups(ws, vecWorkspaceGroups);
          ++it;
        }
      }
      else
      {
        typedef std::vector<boost::shared_ptr<Workspace> > WorkspaceVector;
        WorkspaceVector inWorkspaces;
        WorkspaceVector outWorkspaces;
        sourceAlg->findWorkspaceProperties(inWorkspaces, outWorkspaces);
        UNUSED_ARG(outWorkspaces);
        WorkspaceVector::iterator it = inWorkspaces.begin();
        while (it != inWorkspaces.end())
        {
          tryAddInputWorkspaceToInputGroups(*it, vecWorkspaceGroups);
          ++it;
        }
      }

      validateMultiPeriodGroupInputs(vecWorkspaceGroups);

      return vecWorkspaceGroups;
    }

    bool MultiPeriodGroupWorker::useCustomWorkspaceProperty() const
    {
      return !this->m_workspacePropertyName.empty();
    }

    /**
     * Creates a list of input workspaces as a string for a given period using all nested workspaces at that period
     * within all group workspaces.

     * This requires a little explanation, because this is the reason that this algorithm needs a customised overriden checkGroups and processGroups
     * method:

     * Say you have two multiperiod group workspaces A and B and an output workspace C. A contains matrix workspaces A_1 and A_2, and B contains matrix workspaces B_1 and B2. Because this
     * is multiperiod data. A_1 and B_1 share the same period, as do A_2 and B_2. So merging must be with respect to workspaces of equivalent periods. Therefore,
     * merging must be A_1 + B_1 = C_1 and A_2 + B_2 = C_2. This method constructs the inputs for a nested call to MultiPeriodGroupAlgorithm in this manner.

     * @param periodIndex : zero based index denoting the period.
     * @param vecWorkspaceGroups : Vector of workspace groups
     * @return comma separated string of input workspaces.
     */
    std::string MultiPeriodGroupWorker::createFormattedInputWorkspaceNames(const size_t& periodIndex,
        const VecWSGroupType& vecWorkspaceGroups) const
    {
      std::string prefix = "";
      std::string inputWorkspaces = "";
      for (size_t j = 0; j < vecWorkspaceGroups.size(); ++j)
      {
        inputWorkspaces += prefix + vecWorkspaceGroups[j]->getItem(periodIndex)->name();
        prefix = ",";
      }
      return inputWorkspaces;
    }

    /**
     * 1) Looks for input workspace properties that are of type WorkspaceGroup.
     * 2) If a multiperiod workspace has been set to that property then ..
     * 3) Extracts the individual period workspace from that WorkspaceGroup
     * 4) Manually sets that individual period workspace as the corresponding property on the targetAlgorithm.
     * Copy input workspaces assuming we are working with multi-period groups workspace inputs.
     * @param targetAlg: The spawned algorithm to set the properties on.
     * @param sourceAlg: Algorithm being executed with multiperiod group workspaces.
     * @param periodNumber: The relevant period number used to index into the group workspaces
     */
    void MultiPeriodGroupWorker::copyInputWorkspaceProperties(IAlgorithm* targetAlg,
        IAlgorithm* sourceAlg, const int& periodNumber) const
    {
      std::vector<Property*> props = sourceAlg->getProperties();
      for (size_t j = 0; j < props.size(); j++)
      {
        Property * prop = props[j];
        if (prop)
        {
          if (prop->direction() == Direction::Input)
          {
            if (const IWorkspaceProperty *wsProp = dynamic_cast<IWorkspaceProperty*>(prop))
            {
              if (WorkspaceGroup_sptr inputws = boost::dynamic_pointer_cast<WorkspaceGroup>(
                  wsProp->getWorkspace()))
              {
                if (inputws->isMultiperiod())
                {
                  targetAlg->setProperty(prop->name(), inputws->getItem(periodNumber - 1));
                }
              }
            }
          }
        }
      }
    }

    //--------------------------------------------------------------------------------------------
    /** Process WorkspaceGroup inputs.
     *
     * Overriden from Algorithm base class.
     *
     * This should be called after checkGroups(), which sets up required members.
     * It goes through each member of the group(s), creates and sets an algorithm
     * for each and executes them one by one.
     *
     * If there are several group input workspaces, then the member of each group
     * is executed pair-wise.
     *
     * @param sourceAlg : Source algorithm
     * @param vecMultiPeriodGroups : Vector of pre-identified multiperiod groups.
     * @return true - if all the workspace members are executed.
     */
    bool MultiPeriodGroupWorker::processGroups(Algorithm * const sourceAlg,
        const VecWSGroupType& vecMultiPeriodGroups) const
    {
      // If we are not processing multiperiod groups, use the base behaviour.
      if (vecMultiPeriodGroups.size() < 1)
      {
        return false; // Indicates that this is not a multiperiod group workspace.
      }
      Property* outputWorkspaceProperty = sourceAlg->getProperty("OutputWorkspace");
      const std::string outName = outputWorkspaceProperty->value();

      size_t nPeriods = vecMultiPeriodGroups[0]->size();
      WorkspaceGroup_sptr outputWS = boost::make_shared<WorkspaceGroup>();
      AnalysisDataService::Instance().addOrReplace(outName, outputWS);

      // Loop through all the periods. Create spawned algorithms of the same type as this to process pairs from the input groups.
      for (size_t i = 0; i < nPeriods; ++i)
      {
        const int periodNumber = static_cast<int>(i + 1);
        Algorithm_sptr alg_sptr = API::AlgorithmManager::Instance().createUnmanaged(sourceAlg->name(),
            sourceAlg->version());
        IAlgorithm* alg = alg_sptr.get();
        if (!alg)
        {
          throw std::runtime_error("Algorithm creation failed.");
        }
        alg->initialize();
        // Copy properties that aren't workspaces properties.
        sourceAlg->copyNonWorkspaceProperties(alg, periodNumber);

        if (this->useCustomWorkspaceProperty())
        {
          const std::string inputWorkspaces = createFormattedInputWorkspaceNames(i,
              vecMultiPeriodGroups);
          // Set the input workspace property.
          alg->setPropertyValue(this->m_workspacePropertyName, inputWorkspaces);
        }
        else
        {
          // Configure input properties that are group workspaces.
          copyInputWorkspaceProperties(alg, sourceAlg, periodNumber);
        }
        const std::string outName_i = outName + "_" + Strings::toString(i + 1);
        alg->setPropertyValue("OutputWorkspace", outName_i);
        // Run the spawned algorithm.
        if (!alg->execute())
        {
          throw std::runtime_error(
              "Execution of " + sourceAlg->name() + " for group entry " + Strings::toString(i + 1)
                  + " failed.");
        }
        // Add the output workpace from the spawned algorithm to the group.
        outputWS->add(outName_i);

      }

      sourceAlg->setProperty("OutputWorkspace", outputWS);

      return true;
    }

    /**
     * Validate the multiperiods workspace groups. Gives the opportunity to exit processing if things don't look right.
     * @param vecMultiPeriodGroups : vector of multiperiod groups.
     */
    void MultiPeriodGroupWorker::validateMultiPeriodGroupInputs(const VecWSGroupType& vecMultiPeriodGroups) const
    {
      const size_t multiPeriodGroupsSize = vecMultiPeriodGroups.size();

      if (multiPeriodGroupsSize > 0)
      {
        const size_t benchMarkGroupSize = vecMultiPeriodGroups[0]->size();
        for (size_t i = 0; i < multiPeriodGroupsSize; ++i)
        {
          WorkspaceGroup_sptr currentGroup = vecMultiPeriodGroups[i];
          if (currentGroup->size() != benchMarkGroupSize)
          {
            throw std::runtime_error(
                "Not all the input Multi-period-group input workspaces are the same size.");
          }
          for (size_t j = 0; j < currentGroup->size(); ++j)
          {
            MatrixWorkspace_const_sptr currentNestedWS = boost::dynamic_pointer_cast<
                const MatrixWorkspace>(currentGroup->getItem(j));
            Property* nPeriodsProperty = currentNestedWS->run().getLogData("nperiods");
            size_t nPeriods = atoi(nPeriodsProperty->value().c_str());
            if (nPeriods != benchMarkGroupSize)
            {
              throw std::runtime_error(
                  "Missmatch between nperiods log and the number of workspaces in the input group: "
                      + vecMultiPeriodGroups[i]->name());
            }
            Property* currentPeriodProperty = currentNestedWS->run().getLogData("current_period");
            size_t currentPeriod = atoi(currentPeriodProperty->value().c_str());
            if (currentPeriod != (j + 1))
            {
              throw std::runtime_error(
                  "Multiperiod group workspaces must be ordered by current_period. Correct: "
                      + currentNestedWS->name());
            }
          }
        }
      }
    }

  } // namespace API
} // namespace Mantid
