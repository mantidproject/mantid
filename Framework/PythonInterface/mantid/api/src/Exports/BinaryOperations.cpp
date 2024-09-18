// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidPythonInterface/api/BinaryOperations.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceOpOverloads.h"
#include "MantidPythonInterface/core/Policies/AsType.h"
#include "MantidPythonInterface/core/ReleaseGlobalInterpreterLock.h"

#include <boost/python/def.hpp>
#include <boost/python/return_value_policy.hpp>

void export_BinaryOperations() {
  using namespace Mantid::API;
  using namespace Mantid::PythonInterface::Policies;
  using namespace boost::python;

  // Typedefs the various function types
  using binary_fn_md_md = IMDWorkspace_sptr (*)(const IMDWorkspace_sptr, const IMDWorkspace_sptr, const std::string &,
                                                const std::string &, bool, bool);
  using binary_fn_md_gp = WorkspaceGroup_sptr (*)(const IMDWorkspace_sptr, const WorkspaceGroup_sptr,
                                                  const std::string &, const std::string &, bool, bool);
  using binary_fn_gp_md = WorkspaceGroup_sptr (*)(const WorkspaceGroup_sptr, const IMDWorkspace_sptr,
                                                  const std::string &, const std::string &, bool, bool);
  using binary_fn_gp_gp = WorkspaceGroup_sptr (*)(const WorkspaceGroup_sptr, const WorkspaceGroup_sptr,
                                                  const std::string &, const std::string &, bool, bool);

  using binary_fn_mh_mh = IMDHistoWorkspace_sptr (*)(const IMDHistoWorkspace_sptr, const IMDHistoWorkspace_sptr,
                                                     const std::string &, const std::string &, bool, bool);

  using binary_fn_md_db =
      IMDWorkspace_sptr (*)(const IMDWorkspace_sptr, double, const std::string &, const std::string &, bool, bool);
  using binary_fn_mh_db = IMDHistoWorkspace_sptr (*)(const IMDHistoWorkspace_sptr, double, const std::string &,
                                                     const std::string &, bool, bool);
  using binary_fn_gp_db =
      WorkspaceGroup_sptr (*)(const WorkspaceGroup_sptr, double, const std::string &, const std::string &, bool, bool);

  // Always a return a Workspace_sptr
  using ReturnWorkspaceSptr = return_value_policy<AsType<Workspace_sptr>>;

  // Binary operations that return a workspace
  using boost::python::def;
  using Mantid::PythonInterface::performBinaryOp;
  using Mantid::PythonInterface::performBinaryOpWithDouble;

  def("performBinaryOp", (binary_fn_md_md)&performBinaryOp, ReturnWorkspaceSptr());
  def("performBinaryOp", (binary_fn_md_gp)&performBinaryOp, ReturnWorkspaceSptr());
  def("performBinaryOp", (binary_fn_gp_md)&performBinaryOp, ReturnWorkspaceSptr());
  def("performBinaryOp", (binary_fn_gp_gp)&performBinaryOp, ReturnWorkspaceSptr());
  def("performBinaryOp", (binary_fn_mh_mh)&performBinaryOp, ReturnWorkspaceSptr());

  def("performBinaryOp", (binary_fn_md_db)&performBinaryOpWithDouble, ReturnWorkspaceSptr());
  def("performBinaryOp", (binary_fn_mh_db)&performBinaryOpWithDouble, ReturnWorkspaceSptr());
  def("performBinaryOp", (binary_fn_gp_db)&performBinaryOpWithDouble, ReturnWorkspaceSptr());
}

namespace Mantid::PythonInterface {
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
ResultType performBinaryOp(const LHSType lhs, const RHSType rhs, const std::string &op, const std::string &name,
                           bool inplace, bool reverse) {
  // ----- Determine which version of the algo should be called -----
  MatrixWorkspace_const_sptr lhs_mat = std::dynamic_pointer_cast<const MatrixWorkspace>(lhs);
  MatrixWorkspace_const_sptr rhs_mat = std::dynamic_pointer_cast<const MatrixWorkspace>(rhs);
  WorkspaceGroup_const_sptr lhs_grp = std::dynamic_pointer_cast<const WorkspaceGroup>(lhs);
  WorkspaceGroup_const_sptr rhs_grp = std::dynamic_pointer_cast<const WorkspaceGroup>(rhs);

  const auto algoName = (lhs_mat || lhs_grp) && (rhs_mat || rhs_grp) ? op : op + "MD";

  ResultType result;
  try {
    ReleaseGlobalInterpreterLock releaseGIL;
    if (reverse) {
      result = API::OperatorOverloads::executeBinaryOperation<RHSType, LHSType, ResultType>(algoName, rhs, lhs, inplace,
                                                                                            false, name, true);
    } else {
      result = API::OperatorOverloads::executeBinaryOperation<LHSType, RHSType, ResultType>(algoName, lhs, rhs, inplace,
                                                                                            false, name, true);
    }
  } catch (std::runtime_error &exc) {
    std::string error = exc.what();
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
ResultType performBinaryOpWithDouble(const LHSType inputWS, const double value, const std::string &op,
                                     const std::string &name, bool inplace, bool reverse) {
  // RAII struct to add/remove workspace from ADS
  struct ScopedADSEntry {
    ScopedADSEntry(const std::string &entryName, const MatrixWorkspace_sptr &value) : name(entryName) {
      ads.addOrReplace(entryName, value);
    }
    ~ScopedADSEntry() { ads.remove(name); }

    const std::string &name;
    API::AnalysisDataServiceImpl &ads = API::AnalysisDataService::Instance();
  };

  // In order to recreate a history record of the final binary operation
  // there must be a record of the creation of the single value workspace used
  // on the RHS here. This is achieved by running CreateSingleValuedWorkspace
  // algorithm and adding the output workspace to the ADS. Adding the output
  // to the ADS is critical so that workspace.name() is updated, by the ADS, to
  // return the same string. WorkspaceProperty<TYPE>::createHistory() then
  // records the correct workspace name for input into the final binary
  // operation rather than creating a temporary name.
  auto alg = API::AlgorithmManager::Instance().createUnmanaged("CreateSingleValuedWorkspace");
  alg->setChild(false);
  // we manually store the workspace as it's easier to retrieve the correct
  // type from alg->getProperty rather than calling the ADS again and casting
  alg->setAlwaysStoreInADS(false);
  alg->initialize();
  alg->setProperty<double>("DataValue", value);
  const std::string tmpName("__python_binary_op_single_value");
  alg->setPropertyValue("OutputWorkspace", tmpName);
  { // instantiate releaseGIL in limited scope to allow for repeat in 'performBinaryOp'
    ReleaseGlobalInterpreterLock releaseGIL;
    alg->execute();
  }

  MatrixWorkspace_sptr singleValue;
  if (alg->isExecuted()) {
    singleValue = alg->getProperty("OutputWorkspace");
  } else {
    throw std::runtime_error("performBinaryOp: Error in execution of "
                             "CreateSingleValuedWorkspace");
  }
  ScopedADSEntry removeOnExit(tmpName, singleValue);
  ResultType result =
      performBinaryOp<LHSType, MatrixWorkspace_sptr, ResultType>(inputWS, singleValue, op, name, inplace, reverse);
  return result;
}

// Concrete instantations
template IMDWorkspace_sptr performBinaryOp(const IMDWorkspace_sptr, const IMDWorkspace_sptr, const std::string &,
                                           const std::string &name, bool, bool);
template WorkspaceGroup_sptr performBinaryOp(const IMDWorkspace_sptr, const WorkspaceGroup_sptr, const std::string &,
                                             const std::string &name, bool, bool);
template WorkspaceGroup_sptr performBinaryOp(const WorkspaceGroup_sptr, const IMDWorkspace_sptr, const std::string &,
                                             const std::string &name, bool, bool);
template WorkspaceGroup_sptr performBinaryOp(const WorkspaceGroup_sptr, const WorkspaceGroup_sptr, const std::string &,
                                             const std::string &name, bool, bool);

template IMDHistoWorkspace_sptr performBinaryOp(const IMDHistoWorkspace_sptr, const IMDHistoWorkspace_sptr,
                                                const std::string &, const std::string &name, bool, bool);
template IMDHistoWorkspace_sptr performBinaryOp(const IMDHistoWorkspace_sptr, const MatrixWorkspace_sptr,
                                                const std::string &, const std::string &name, bool, bool);

// Double variants
template IMDWorkspace_sptr performBinaryOpWithDouble(const IMDWorkspace_sptr, const double, const std::string &op,
                                                     const std::string &, bool, bool);
template IMDHistoWorkspace_sptr performBinaryOpWithDouble(const IMDHistoWorkspace_sptr, const double,
                                                          const std::string &op, const std::string &, bool, bool);
template WorkspaceGroup_sptr performBinaryOpWithDouble(const WorkspaceGroup_sptr, const double, const std::string &op,
                                                       const std::string &, bool, bool);
} // namespace Mantid::PythonInterface
