#include "MantidPythonInterface/api/BinaryOperations.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceOpOverloads.h"
#include "MantidPythonInterface/kernel/Policies/AsType.h"

#include <boost/python/def.hpp>
#include <boost/python/return_value_policy.hpp>

void export_BinaryOperations() {
  using namespace Mantid::API;
  using namespace Mantid::PythonInterface::Policies;
  using namespace boost::python;

  // Typedefs the various function types
  using binary_fn_md_md = IMDWorkspace_sptr (
      *)(const IMDWorkspace_sptr, const IMDWorkspace_sptr, const std::string &,
         const std::string &, bool, bool);
  using binary_fn_md_gp = WorkspaceGroup_sptr (
      *)(const IMDWorkspace_sptr, const WorkspaceGroup_sptr,
         const std::string &, const std::string &, bool, bool);
  using binary_fn_gp_md = WorkspaceGroup_sptr (
      *)(const WorkspaceGroup_sptr, const IMDWorkspace_sptr,
         const std::string &, const std::string &, bool, bool);
  using binary_fn_gp_gp = WorkspaceGroup_sptr (
      *)(const WorkspaceGroup_sptr, const WorkspaceGroup_sptr,
         const std::string &, const std::string &, bool, bool);

  using binary_fn_mh_mh = IMDHistoWorkspace_sptr (
      *)(const IMDHistoWorkspace_sptr, const IMDHistoWorkspace_sptr,
         const std::string &, const std::string &, bool, bool);

  using binary_fn_md_db = IMDWorkspace_sptr (
      *)(const IMDWorkspace_sptr, double, const std::string &,
         const std::string &, bool, bool);
  using binary_fn_mh_db = IMDHistoWorkspace_sptr (
      *)(const IMDHistoWorkspace_sptr, double, const std::string &,
         const std::string &, bool, bool);
  using binary_fn_gp_db = WorkspaceGroup_sptr (
      *)(const WorkspaceGroup_sptr, double, const std::string &,
         const std::string &, bool, bool);

  // Always a return a Workspace_sptr
  using ReturnWorkspaceSptr = return_value_policy<AsType<Workspace_sptr>>;

  // Binary operations that return a workspace
  using boost::python::def;
  using Mantid::PythonInterface::performBinaryOp;
  using Mantid::PythonInterface::performBinaryOpWithDouble;

  def("performBinaryOp", (binary_fn_md_md)&performBinaryOp,
      ReturnWorkspaceSptr());
  def("performBinaryOp", (binary_fn_md_gp)&performBinaryOp,
      ReturnWorkspaceSptr());
  def("performBinaryOp", (binary_fn_gp_md)&performBinaryOp,
      ReturnWorkspaceSptr());
  def("performBinaryOp", (binary_fn_gp_gp)&performBinaryOp,
      ReturnWorkspaceSptr());
  def("performBinaryOp", (binary_fn_mh_mh)&performBinaryOp,
      ReturnWorkspaceSptr());

  def("performBinaryOp", (binary_fn_md_db)&performBinaryOpWithDouble,
      ReturnWorkspaceSptr());
  def("performBinaryOp", (binary_fn_mh_db)&performBinaryOpWithDouble,
      ReturnWorkspaceSptr());
  def("performBinaryOp", (binary_fn_gp_db)&performBinaryOpWithDouble,
      ReturnWorkspaceSptr());
}

namespace Mantid {
namespace PythonInterface {
using namespace Mantid::API;

/** Binary operation for two workspaces. Generic for IMDWorkspaces or
 *MatrixWorkspaces...
 * Called by python overloads for _binary_op (see api_exports.cpp)
 *
 * @param lhs :: the left hand side workspace of the operation
 * @param rhs :: the right hand side workspace of the operation
 * @param op :: The operation
 * @param name :: The output name
 * @param inplace :: is this is an inplace operation (i.e. does the output
 *overwrite the lhs
 * @param reverse :: Unused parameter. Here for consistent interface
 * @returns The resulting workspace
 */
template <typename LHSType, typename RHSType, typename ResultType>
ResultType performBinaryOp(const LHSType lhs, const RHSType rhs,
                           const std::string &op, const std::string &name,
                           bool inplace, bool reverse) {
  std::string algoName = op;

  // ----- Determine which version of the algo should be called -----
  MatrixWorkspace_const_sptr lhs_mat =
      boost::dynamic_pointer_cast<const MatrixWorkspace>(lhs);
  MatrixWorkspace_const_sptr rhs_mat =
      boost::dynamic_pointer_cast<const MatrixWorkspace>(rhs);
  WorkspaceGroup_const_sptr lhs_grp =
      boost::dynamic_pointer_cast<const WorkspaceGroup>(lhs);
  WorkspaceGroup_const_sptr rhs_grp =
      boost::dynamic_pointer_cast<const WorkspaceGroup>(rhs);

  if ((lhs_mat || lhs_grp) && (rhs_mat || rhs_grp))
    // Both sides are matrixworkspace - use the original algos (e..g "Plus.")
    algoName = op;
  else
    // One of the workspaces must be MDHistoWorkspace or MDEventWorkspace
    // Use the MD version, e.g. "PlusMD"
    algoName = op + "MD";

  ResultType result;
  std::string error;
  try {
    if (reverse) {
      result = API::OperatorOverloads::executeBinaryOperation<RHSType, LHSType,
                                                              ResultType>(
          algoName, rhs, lhs, inplace, false, name, true);
    } else {
      result = API::OperatorOverloads::executeBinaryOperation<LHSType, RHSType,
                                                              ResultType>(
          algoName, lhs, rhs, inplace, false, name, true);
    }
  } catch (std::runtime_error &exc) {
    error = exc.what();
    if (error == "algorithm") {
      error = "Unknown binary operation requested: " + op;
      throw std::runtime_error(error);
    } else {
      throw;
    }
  }
  return result;
}

/**
* Perform the given binary operation on a workspace and a double.
* Generic to MDWorkspaces.
* Called by python overloads for _binary_op (see api_exports.cpp)
*
* @param inputWS :: The input workspace
* @param value :: The input value
* @param op :: The operation
* @param name :: The output name
* @param inplace :: If true, then the lhs argument is replaced by the result of
*the operation.
* @param reverse :: If true then the double is the lhs argument
* @return A shared pointer to the result workspace
*/
template <typename LHSType, typename ResultType>
ResultType performBinaryOpWithDouble(const LHSType inputWS, const double value,
                                     const std::string &op,
                                     const std::string &name, bool inplace,
                                     bool reverse) {
  const std::string &algoName = op;

  // Create the single valued workspace first so that it is run as a top-level
  // algorithm
  // such that it's history can be recreated
  API::Algorithm_sptr alg = API::AlgorithmManager::Instance().createUnmanaged(
      "CreateSingleValuedWorkspace");
  alg->setChild(false);
  alg->setAlwaysStoreInADS(false);
  alg->initialize();
  alg->setProperty<double>("DataValue", value);
  const std::string tmp_name("__single_value");
  alg->setPropertyValue("OutputWorkspace", tmp_name);
  alg->execute();

  auto &ads = Mantid::API::AnalysisDataService::Instance();
  MatrixWorkspace_sptr singleValue;
  if (alg->isExecuted()) {
    singleValue = alg->getProperty("OutputWorkspace");

	// We must store this in the ADS to force the workspace
	// to have a name, so that the history entry does not
	// use a temporary name which changes from time to time
	ads.add(tmp_name, singleValue);
  } else {
    throw std::runtime_error(
        "performBinaryOp: Error in execution of CreateSingleValuedWorkspace");
  }
  // Call the function above with the single-value workspace
  ResultType result =
      performBinaryOp<LHSType, MatrixWorkspace_sptr, ResultType>(
          inputWS, singleValue, algoName, name, inplace, reverse);
  ads.remove(tmp_name);
  return result;
}

// Concrete instantations
template IMDWorkspace_sptr performBinaryOp(const IMDWorkspace_sptr,
                                           const IMDWorkspace_sptr,
                                           const std::string &,
                                           const std::string &name, bool, bool);
template WorkspaceGroup_sptr
performBinaryOp(const IMDWorkspace_sptr, const WorkspaceGroup_sptr,
                const std::string &, const std::string &name, bool, bool);
template WorkspaceGroup_sptr
performBinaryOp(const WorkspaceGroup_sptr, const IMDWorkspace_sptr,
                const std::string &, const std::string &name, bool, bool);
template WorkspaceGroup_sptr
performBinaryOp(const WorkspaceGroup_sptr, const WorkspaceGroup_sptr,
                const std::string &, const std::string &name, bool, bool);

template IMDHistoWorkspace_sptr
performBinaryOp(const IMDHistoWorkspace_sptr, const IMDHistoWorkspace_sptr,
                const std::string &, const std::string &name, bool, bool);
template IMDHistoWorkspace_sptr
performBinaryOp(const IMDHistoWorkspace_sptr, const MatrixWorkspace_sptr,
                const std::string &, const std::string &name, bool, bool);

// Double variants
template IMDWorkspace_sptr performBinaryOpWithDouble(const IMDWorkspace_sptr,
                                                     const double,
                                                     const std::string &op,
                                                     const std::string &, bool,
                                                     bool);
template IMDHistoWorkspace_sptr
performBinaryOpWithDouble(const IMDHistoWorkspace_sptr, const double,
                          const std::string &op, const std::string &, bool,
                          bool);
template WorkspaceGroup_sptr
performBinaryOpWithDouble(const WorkspaceGroup_sptr, const double,
                          const std::string &op, const std::string &, bool,
                          bool);
}
}
