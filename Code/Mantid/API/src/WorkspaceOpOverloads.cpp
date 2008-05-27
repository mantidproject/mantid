
#include "MantidAPI/WorkspaceOpOverloads.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/IWorkspaceProperty.h"
#include "MantidAPI/WorkspaceFactory.h"


namespace Mantid
{
  namespace API
  {
    /** Adds two workspaces
    *  @param lhs left hand side workspace shared pointer
    *  @param rhs left hand side workspace shared pointer
    *  @returns The result in a workspace shared pointer
    */
    Workspace_sptr operator+(const Workspace_sptr lhs, const Workspace_sptr rhs)
    {
      return executeBinaryOperation("Plus",lhs,rhs);
    }

    /** Adds a workspace to a single value
    *  @param lhs left hand side workspace shared pointer
    *  @param rhs the single value (error is taken as sqrt(value))
    *  @returns The result in a workspace shared pointer
    */
    Workspace_sptr operator+(const Workspace_sptr lhs, const double rhsValue)
    {
      return executeBinaryOperation("Plus",lhs,createWorkspaceSingleValue(rhsValue));
    }

    /** Subtracts two workspaces
    *  @param lhs left hand side workspace shared pointer
    *  @param rhs left hand side workspace shared pointer
    *  @returns The result in a workspace shared pointer
    */
    Workspace_sptr operator-(const Workspace_sptr lhs, const Workspace_sptr rhs)
    {
      return executeBinaryOperation("Minus",lhs,rhs);
    }

    /** Subtracts  a single value from a workspace
    *  @param lhs left hand side workspace shared pointer
    *  @param rhs the single value (error is taken as sqrt(value))
    *  @returns The result in a workspace shared pointer
    */
    Workspace_sptr operator-(const Workspace_sptr lhs, const double rhsValue)
    {
      return executeBinaryOperation("Minus",lhs,createWorkspaceSingleValue(rhsValue));
    }

    /** Multiply two workspaces
    *  @param lhs left hand side workspace shared pointer
    *  @param rhs left hand side workspace shared pointer
    *  @returns The result in a workspace shared pointer
    */
    Workspace_sptr operator*(const Workspace_sptr lhs, const Workspace_sptr rhs)
    {
      return executeBinaryOperation("Multiply",lhs,rhs);
    }

    /** Multiply a workspace and a single value
    *  @param lhs left hand side workspace shared pointer
    *  @param rhs the single value (error is taken as sqrt(value))
    *  @returns The result in a workspace shared pointer
    */
    Workspace_sptr operator*(const Workspace_sptr lhs, const double rhsValue)
    {
      return executeBinaryOperation("Multiply",lhs,createWorkspaceSingleValue(rhsValue));
    }

    /** Divide two workspaces
    *  @param lhs left hand side workspace shared pointer
    *  @param rhs left hand side workspace shared pointer
    *  @returns The result in a workspace shared pointer
    */
    Workspace_sptr operator/(const Workspace_sptr lhs, const Workspace_sptr rhs)
    {
      return executeBinaryOperation("Divide",lhs,rhs);
    }

    /** Divide a workspace by a single value
    *  @param lhs left hand side workspace shared pointer
    *  @param rhs the single value (error is taken as sqrt(value)
    *  @returns The result in a workspace shared pointer
    */
    Workspace_sptr operator/(const Workspace_sptr lhs, const double rhsValue)
    {
      return executeBinaryOperation("Divide",lhs,createWorkspaceSingleValue(rhsValue));
    }

    /** Performs a binary operation on two workspaces
    *  @param algorithmName The name of the binary operation to perform
    *  @param lhs left hand side workspace shared pointer
    *  @param rhs left hand side workspace shared pointer
    *  @returns The result in a workspace shared pointer
    */
    Workspace_sptr executeBinaryOperation(const std::string algorithmName, const Workspace_sptr lhs, const Workspace_sptr rhs)
    {
      Algorithm_sptr alg = AlgorithmManager::Instance().createUnmanaged(algorithmName);
      alg->setChild(true);
      alg->initialize();    
      
      //we have to set text names for the workspace parameters even though they will not be used.
      //This satisfies the validation.
      alg->setPropertyValue("InputWorkspace_1","¦¬NotApplicable1");
      alg->setPropertyValue("InputWorkspace_2","¦¬NotApplicable2");
      alg->setPropertyValue("OutputWorkspace","¦¬NotApplicable3");

      alg->setProperty<Workspace_sptr>("InputWorkspace_1",lhs);
      alg->setProperty<Workspace_sptr>("InputWorkspace_2",rhs);
      alg->execute();

      if (alg->isExecuted())
      {
      //Get the output workspace property
      const std::vector< Kernel::Property*> &props = alg->getProperties();
      for (unsigned int i = 0; i < props.size(); ++i)
      {
        if (props[i]->name() == "OutputWorkspace")
        {
          IWorkspaceProperty *wsProp = dynamic_cast<IWorkspaceProperty*>(props[i]);
          if (wsProp)
          {
            return wsProp->getWorkspace();
          }
        }
      }
      }
      else
      {
        throw std::runtime_error("Error while executing operation algorithm: algorithmName");
      }

      throw Kernel::Exception::NotFoundError("Required output workspace property not found on sub algorithm" ,"OutputWorkspace");

      //Horendous code inclusion to satisfy compilers that all code paths return a value
      // in reality the above code should either throw or return successfully.
      Workspace_sptr retVal = WorkspaceFactory::Instance().create("Workspace2D");
      return retVal;
    }

    /** Creates a temporary single value workspace the error is set to sqrt(value)
    *  @param rhsValue the value to use
    *  @returns The value in a workspace shared pointer
    */
    Workspace_sptr createWorkspaceSingleValue(const double rhsValue)
    {
      Workspace_sptr retVal = WorkspaceFactory::Instance().create("WorkspaceSingleValue");
      retVal->dataY(0)[0]=rhsValue;
      
      return retVal;
    }
  }
}
