#include "MantidAPI/MultiPeriodGroupWorker.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/Property.h"
#include "MantidAPI/IAlgorithm.h"

using namespace Mantid::Kernel;

namespace Mantid
{
  namespace API
  {

    //----------------------------------------------------------------------------------------------
    /** Constructor
     */
    MultiPeriodGroupWorker::MultiPeriodGroupWorker() :
        m_workspacePropertyName(""), m_useDefaultGroupingBehaviour(true)
    {
    }

    /**
     * Constructor
     * @param workspacePropertyName : Property name to treat as source of multiperiod workspaces.
     */
    MultiPeriodGroupWorker::MultiPeriodGroupWorker(const std::string& workspacePropertyName) :
        m_workspacePropertyName(workspacePropertyName), m_useDefaultGroupingBehaviour(true)
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
    void MultiPeriodGroupWorker::tryAddInputWorkspaceToInputGroups(Workspace_sptr ws, MultiPeriodGroupWorker::VecWSGroupType& vecWorkspaceGroups) const
    {
      WorkspaceGroup_sptr inputGroup = boost::dynamic_pointer_cast<WorkspaceGroup>(ws);
      if(inputGroup)
      {
        if(inputGroup->isMultiperiod())
        {
          vecWorkspaceGroups.push_back(inputGroup);
        }
      }
    }

     MultiPeriodGroupWorker::VecWSGroupType MultiPeriodGroupWorker::findMultiPeriodGroups(Algorithm_sptr alg) const
    {
      if(!alg->isInitialized())
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
        Property* inputProperty = alg->getProperty(this->m_workspacePropertyName);

        if (!dynamic_cast<ArrayProperty<std::string>*>(inputProperty))
        {
          throw std::runtime_error(
              "Support for custom input workspaces that are not string Arrays are not currently supported.");
          /*Note that we could extend this algorithm to cover other input property types if required, but we don't need that funtionality now.*/
        }

        WorkspaceNameType workspaces = alg->getProperty(this->m_workspacePropertyName);
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
        alg->findWorkspaceProperties(inWorkspaces, outWorkspaces);
        UNUSED_ARG(outWorkspaces);
        WorkspaceVector::iterator it = inWorkspaces.begin();
        while (it != inWorkspaces.end())
        {
          tryAddInputWorkspaceToInputGroups(*it, vecWorkspaceGroups);
          ++it;
        }
      }
      return vecWorkspaceGroups;
    }

    bool MultiPeriodGroupWorker::useCustomWorkspaceProperty() const
    {
      return !this->m_workspacePropertyName.empty();
    }

  } // namespace API
} // namespace Mantid
