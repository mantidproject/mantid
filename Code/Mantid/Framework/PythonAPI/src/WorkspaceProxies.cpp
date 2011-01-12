//---------------------------------------
// Includes
//---------------------------------------
#include <MantidPythonAPI/WorkspaceProxies.h>
#include <MantidPythonAPI/MantidVecHelper.h>

#include <MantidAPI/WorkspaceOpOverloads.h>
#include <MantidAPI/AlgorithmManager.h>
#include <MantidAPI/AnalysisDataService.h>

namespace Mantid
{
  namespace PythonAPI
  {
    using API::MatrixWorkspace_sptr;

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
    // Binary Operation helpers
    //
    //**********************************************************************************

    /** Binary operation for two workspaces
     * @param lhs the left hand side workspace of the operation
     * @param rhs the right hand side workspace of the operation
     * @param op The operation
     * @param name The output name
     * @param inplace is this is an inplace operation (i.e. does the output overwrite the lhs
     * @param reverse Unused parameter. Here for consistent interface
     * @returns The resulting workspace
     */
    MatrixWorkspace_sptr performBinaryOp(const MatrixWorkspace_sptr lhs, 
					 const MatrixWorkspace_sptr rhs, 
					 const std::string& op, const std::string & name, 
					 bool inplace,bool reverse)
    {
      MatrixWorkspace_sptr result;
      std::string error("");
      try
      {
	if( reverse ) 
	{
	  result = API::OperatorOverloads::executeBinaryOperation(op, rhs, lhs, inplace, false, name, true);
	}
	else
	{
	  result = API::OperatorOverloads::executeBinaryOperation(op, lhs, rhs, inplace, false, name, true);
	}
      }
      catch(std::runtime_error & exc)
      {
	error = exc.what();
	if( error.find("algorithm") == 0 )
	{
	  error = "Unknown binary operation requested: " + op;
	  throw std::runtime_error(error);
	}
	else
	{
	  throw;
	}
      }
      return result;
    }

    /** 
    * Perform the given binary operation on a workspace and a double
    * @param lhs The input workspace
    * @param rhs The input value
    * @param op The operation
    * @param inplace If true, then the lhs argument is replaced by the result of the operation.
    * @param reverse If true then the double is the lhs argument
    * @return A shared pointer to the result workspace
    */
    MatrixWorkspace_sptr performBinaryOp(const MatrixWorkspace_sptr inputWS, const double value, 
					 const std::string& op, const std::string & name, 
					 bool inplace, bool reverse)
    {
      // Create the single valued workspace first so that it is run as a top-level algorithm
      // such that it's history can be recreated
      API::IAlgorithm_sptr alg = API::AlgorithmManager::Instance().createUnmanaged("CreateSingleValuedWorkspace");
      alg->setChild(false);
      alg->initialize();
      alg->setProperty<double>("DataValue",value);
      const std::string & tmp_name("__tmp_double");
      alg->setPropertyValue("OutputWorkspace", tmp_name);
      alg->execute();
      MatrixWorkspace_sptr singleValue;
      API::AnalysisDataServiceImpl & data_store = API::AnalysisDataService::Instance();
      if( alg->isExecuted() )
      {
	singleValue = boost::dynamic_pointer_cast<API::MatrixWorkspace>(data_store.retrieve(tmp_name));
      }
      else
      {
	throw std::runtime_error("performBinaryOp: Error in execution of CreateSingleValuedWorkspace");
      }      
      MatrixWorkspace_sptr result = performBinaryOp(inputWS, singleValue, op, name, inplace, reverse);
      // Delete the temporary
      data_store.remove(tmp_name);
      return result;
    }

  }

}
