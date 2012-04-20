/*WIKI* 

This algorithm takes a list of workspaces and list of property, and produces a TableWorkspace of the properties with the given names.

*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/CreateLogPropertyTable.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/VisibleWhenProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h" 

#include "boost/shared_ptr.hpp"

#include <vector>
#include <assert.h>

namespace Mantid
{
namespace Algorithms
{
  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(CreateLogPropertyTable)

  using namespace Kernel;
  using namespace API;

  // Forward declarations.
  namespace
  {
    std::vector<MatrixWorkspace_sptr> retrieveMatrixWsList(const std::vector<std::string> & wsNames, const std::string & useGroupChildrenPolicy);
  }

  /**
   * Initialize the algorithm's properties.
   */
  void CreateLogPropertyTable::init()
  {
    // Input workspaces
    declareProperty(new ArrayProperty<std::string>("InputWorkspaces", boost::make_shared<MandatoryValidator<std::vector<std::string>>>()),
      "Name of the Input Workspaces from which to get log properties.");

    // Which log properties to use
    declareProperty(new ArrayProperty<std::string>("LogPropertyNames", boost::make_shared<MandatoryValidator<std::vector<std::string>>>()),
      "The names of the log properties to place in table.");

    // How to handle workspace groups
    const std::string groupOptionsArray[3] = {"All", "First", "None"};
    std::vector<std::string> groupOptions;
    groupOptions.assign(groupOptionsArray, groupOptionsArray + 3);

    declareProperty("GroupChildren", "First", boost::make_shared<StringListValidator>(groupOptions),
      "The policy by which to handle GroupWorkspaces.  \"All\" will include all children, \"First\" will include "
      "the first child, and \"None\" will not include any.");

    // Output workspace
    declareProperty(new WorkspaceProperty<ITableWorkspace> ("OutputWorkspace", "", Direction::Output),
      "Name of the output ITableWorkspace.");
  }

  /**
   * Execute the algorithm.
   */
  void CreateLogPropertyTable::exec()
  {
    std::vector<std::string> wsNames = this->getProperty("InputWorkspaces");

    // Retrieve a list of MatrixWorkspace pointers, using the given "GroupChildren" policy.
    const std::string useGroupChildrenPolicy = this->getPropertyValue("GroupChildren");
    std::vector<MatrixWorkspace_sptr> matrixWsList = retrieveMatrixWsList(wsNames, useGroupChildrenPolicy);

    // Get the names of the properties that will be stored.
    std::vector<std::string> propNames = this->getProperty("LogPropertyNames");

    // Make sure all workspaces contain the properties.
    for( auto matrixWs = matrixWsList.begin(); matrixWs != matrixWsList.end(); ++matrixWs )
    {
      const Run & run = matrixWs->get()->run();
      const std::string wsName = matrixWs->get()->getName();

      // Throw if a run does not have a property.
      for( auto propName = propNames.begin(); propName != propNames.end(); ++propName )
        if( !run.hasProperty(*propName) )
          throw std::runtime_error( "\"" + wsName + "\" does not have a run property of \"" + *propName + "\"." );
    }

    const std::string outputTableName = this->getPropertyValue("OutputWorkspace");

    // Set up output table.
    boost::shared_ptr<ITableWorkspace> outputTable = WorkspaceFactory::Instance().createTable();
    // One column for each property.
    for( auto propName = propNames.begin(); propName != propNames.end(); ++propName )
      outputTable->addColumn("str", *propName);
    // One row for each workspace.
    for( size_t i = 0; i < matrixWsList.size(); ++i )
      outputTable->appendRow();
    
    // Populate output table with the requested run properties.
    for( size_t i = 0; i < outputTable->rowCount(); ++i )
    {
      TableRow row = outputTable->getRow(i);
      MatrixWorkspace_sptr matrixWs = matrixWsList[i];
      
      for( auto propName = propNames.begin(); propName != propNames.end(); ++propName )
      {
        const std::string propValue = matrixWs->run().getProperty(*propName)->value();
        row << propValue;
      }
    }

    // Add to ADS and set as output.
    AnalysisDataService::Instance().addOrReplace(outputTableName, outputTable);
    this->setProperty("OutputWorkspace", outputTable);
  }

  /**
   * Sets documentation strings for this algorithm.
   */
  void CreateLogPropertyTable::initDocs()
  {
    this->setWikiSummary("Takes a list of workspaces and a list of log property names.  For each workspace, the Run info is inspected and "
      "all log property values are used to populate a resulting output TableWorkspace.");
  }

  namespace
  {
    /**
     * Given a list of workspace names, will retrieve pointers to the corresponding workspaces in the ADS.
     * Only MatrixWorkspaces or the children of groups of MatrixWorkspaces are retrieved. GroupWorkspaces 
     * are handled via the "useGroupChildrenPolicy":
     *
     * "All"   - Retrieve pointers to all the children of a group.
     * "First" - Only retrieve a pointer to the first child of a group.
     *
     * @param wsNames                :: the list of workspaces to retrieve pointers to.
     * @param useGroupChildrenPolicy :: the policy by which to deal with group workspaces.
     *
     * @return the retrieved MatrixWorkspace pointers
     */
    std::vector<MatrixWorkspace_sptr> retrieveMatrixWsList(const std::vector<std::string> & wsNames, const std::string & useGroupChildrenPolicy)
    {
      std::vector<MatrixWorkspace_sptr> matrixWsList;
      
      // Get all the workspaces which are to be inspected for log proeprties.
      for( auto wsName = wsNames.begin(); wsName != wsNames.end(); ++wsName )
      {
        WorkspaceGroup_sptr wsGroup = boost::shared_dynamic_cast<WorkspaceGroup>( AnalysisDataService::Instance().retrieve(*wsName) );
        MatrixWorkspace_sptr matrixWs = boost::shared_dynamic_cast<MatrixWorkspace>( AnalysisDataService::Instance().retrieve(*wsName) );

        // Handle any workspace groups.
        if( wsGroup )
        {
          std::vector<std::string> childNames = wsGroup->getNames();
          
          // If there are no child workspaces in the group (is this possible?), just ignore it.
          if( childNames.size() < 1 )
            break;

          // Retrieve pointers to all the child workspaces.
          std::vector<MatrixWorkspace_sptr> childWsList;

          for( auto childName = childNames.begin(); childName != childNames.end(); ++childName )
          {
            childWsList.push_back(AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(*childName));
          }

          if( useGroupChildrenPolicy == "All" )
            // Append all the children to the list.
            for( auto childWs = childWsList.begin(); childWs != childWsList.end(); ++childWs)
              matrixWsList.push_back(*childWs);

          else if ( useGroupChildrenPolicy == "First" )
            // Append only the first child to the list.
            matrixWsList.push_back(childWsList[0]);
        }
        // Append any MatrixWorkspaces.
        else if( matrixWs )
        {
          matrixWsList.push_back(matrixWs);
        }
      }

      return matrixWsList;
    }
  }

} // namespace Algorithms
} // namespace Mantid
