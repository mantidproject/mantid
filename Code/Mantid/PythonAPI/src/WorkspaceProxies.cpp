//---------------------------------------
// Includes
//---------------------------------------
#include <MantidPythonAPI/WorkspaceProxies.h>
#include <MantidPythonAPI/MantidVecHelper.h>

#include <MantidAPI/WorkspaceOpOverloads.h>

namespace Mantid
{
  namespace PythonAPI
  {

    //****************************************
    //
    // MatrixWorkspaceWrapper
    //
    //****************************************
    /**
    * Returns the X values from the spectra at the given index wrapped in a read-only numpy array
    * @param self A reference to the workspace
    * @param index The index of the workspace
    * @returns A numpy array for the given index
    */
    PyObject * MatrixWorkspaceWrapper::readX(API::MatrixWorkspace& self, int index)
    {
      return MantidVecHelper::createPythonWrapper(self.readX(index), true);
    }

    /**
    * Returns the Y values from the spectra at the given index wrapped in a read-only numpy array
    * @param self A reference to the workspace
    * @param index The index of the workspace
    * @returns A numpy array for the given index
    */
    PyObject * MatrixWorkspaceWrapper::readY(API::MatrixWorkspace& self, int index)
    {
      return MantidVecHelper::createPythonWrapper(self.readY(index), true);
    }

    /**
    * Returns the E values from the spectra at the given index wrapped in a read-only numpy array
    * @param self A reference to the workspace
    * @param index The index of the workspace
    * @returns A numpy array for the given index
    */
    PyObject * MatrixWorkspaceWrapper::readE(API::MatrixWorkspace& self, int index)
    {
      return MantidVecHelper::createPythonWrapper(self.readE(index), true);
    }

    //*********************************************************************************
    //
    // WorkspaceAlgebraHelper
    //
    //**********************************************************************************

    /** Binary operation for two workspaces
     * @param lhs the left hand side workspace of the operation
     * @param rhs the right hand side workspace of the operation
     * @param op The operation
     * @param name The output name
     * @param inplace is this is an inplace operation (i.e. does the output overwrite the lhs
     * @returns The resulting workspace
     */
    WorkspaceAlgebraHelper::wraptype_ptr 
    WorkspaceAlgebraHelper::performBinaryOp(const wraptype_ptr lhs, const wraptype_ptr rhs, 
					    const std::string& op, const std::string & name, bool inplace)
    {
      wraptype_ptr result;
      std::string error("");
      std::string wsName("");
      if( inplace )
      {
	if( op == "Plus" ) lhs += rhs;
	else if( op == "Minus" ) lhs -= rhs;
	else if( op == "Multiply" ) lhs *= rhs;
	else if( op == "Divide" ) lhs /= rhs;
	else error = "WorkspaceAlgebraHelper::performBinaryOp - Unknown inplace binary operation requested: " + op;
        result = lhs;
	wsName = lhs->getName();
      }
      else
      {
	if( op == "Plus" ) result = lhs + rhs;
	else if( op == "Minus" ) result = lhs - rhs; 
	else if( op == "Multiply" ) result = lhs * rhs; 
	else if( op == "Divide" ) result = lhs / rhs; 
	else error = "WorkspaceAlgebraHelper::performBinaryOp - Unknown binary operation requested: " + op;
      }
      if( !error.empty() )
      {
	throw std::runtime_error(error);
      }
      Mantid::API::AnalysisDataService::Instance().addOrReplace(name, result);
      return result;
    }

    /** 
    * Perform the given binary operation on a workspace and a double
    * @param lhs The input workspace
    * @param rhs The input value
    * @param op The operation
    * @param inplace If true, then the lhs argument is replaced by the result of the operation.
    * @return A shared pointer to the result workspace
    */
    WorkspaceAlgebraHelper::wraptype_ptr 
    WorkspaceAlgebraHelper::performBinaryOp(const wraptype_ptr inputWS, const double value, 
					    const std::string& op, const std::string & name, bool inplace)
    {
      wraptype_ptr result;
      std::string error("");
      if( inplace )
      {
	if( op == "Plus" ) inputWS += value;
	else if( op == "Minus" ) inputWS -= value;
	else if( op == "Multiply" ) inputWS *= value;
	else if( op == "Divide" ) inputWS /= value;
	else error = "WorkspaceAlgebraHelper::performBinaryOp - Unknown inplace binary operation requested: " + op;
        result = inputWS;
      }
      else
      {
	if( op == "Plus" ) result = inputWS + value;
	else if( op == "Minus" ) result = inputWS - value; 
	else if( op == "Multiply" ) result = inputWS * value; 
	else if( op == "Divide" ) result = inputWS / value; 
	else error = "WorkspaceAlgebraHelper::performBinaryOp - Unknown binary operation requested: " + op;
      }
      if( !error.empty() )
      {
	throw std::runtime_error(error);
      }
      Mantid::API::AnalysisDataService::Instance().addOrReplace(name, result);
      return result;
    }

  }

}
