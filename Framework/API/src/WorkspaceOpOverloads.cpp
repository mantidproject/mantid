// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/WorkspaceOpOverloads.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/IWorkspaceProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/Property.h"

#include <numeric>

namespace Mantid {
namespace API {
namespace OperatorOverloads {

/** Performs a binary operation on two workspaces
 *  @param algorithmName :: The name of the binary operation to perform
 *  @param lhs :: left hand side workspace shared pointer
 *  @param rhs :: right hand side workspace shared pointer
 *  @param lhsAsOutput :: If true, indicates that the lhs input is the same
 * workspace as the output one
 *  @param child :: If true the algorithm is run as a child
 *  @param name :: If child is true and this is not an inplace operation then
 * this name is used as output
 *  @param rethrow :: A flag indicating whether to rethrow exceptions
 *  @return The result in a workspace shared pointer
 */
template <typename LHSType, typename RHSType, typename ResultType>
ResultType executeBinaryOperation(const std::string &algorithmName,
                                  const LHSType lhs, const RHSType rhs,
                                  bool lhsAsOutput, bool child,
                                  const std::string &name, bool rethrow) {
  IAlgorithm_sptr alg =
      AlgorithmManager::Instance().createUnmanaged(algorithmName);
  alg->setChild(child);
  alg->setRethrows(rethrow);
  alg->initialize();

  if (lhs->getName().empty()) {
    alg->setProperty<LHSType>("LHSWorkspace", lhs);
  } else {
    alg->setPropertyValue("LHSWorkspace", lhs->getName());
  }
  if (rhs->getName().empty()) {
    alg->setProperty<RHSType>("RHSWorkspace", rhs);
  } else {
    alg->setPropertyValue("RHSWorkspace", rhs->getName());
  }
  if (lhsAsOutput) {
    if (!lhs->getName().empty()) {
      alg->setPropertyValue("OutputWorkspace", lhs->getName());
    } else {
      alg->setAlwaysStoreInADS(false);
      alg->setPropertyValue("OutputWorkspace", "dummy-output-name");
      alg->setProperty<LHSType>("OutputWorkspace", lhs);
    }
  } else {
    if (name.empty()) {
      alg->setAlwaysStoreInADS(false);
      alg->setPropertyValue("OutputWorkspace", "dummy-output-name");
    } else {
      alg->setPropertyValue("OutputWorkspace", name);
    }
  }

  alg->execute();

  if (!alg->isExecuted()) {
    std::string message = "Error while executing operation: " + algorithmName;
    throw std::runtime_error(message);
  }

  // Get the output workspace property
  if (!alg->getAlwaysStoreInADS()) {
    API::MatrixWorkspace_sptr result = alg->getProperty("OutputWorkspace");
    return std::dynamic_pointer_cast<typename ResultType::element_type>(result);
  } else {
    API::Workspace_sptr result = API::AnalysisDataService::Instance().retrieve(
        alg->getPropertyValue("OutputWorkspace"));
    return std::dynamic_pointer_cast<typename ResultType::element_type>(result);
  }
}

template MANTID_API_DLL MatrixWorkspace_sptr executeBinaryOperation(
    const std::string &, const MatrixWorkspace_sptr, const MatrixWorkspace_sptr,
    bool, bool, const std::string &, bool);
template MANTID_API_DLL WorkspaceGroup_sptr executeBinaryOperation(
    const std::string &, const WorkspaceGroup_sptr, const WorkspaceGroup_sptr,
    bool, bool, const std::string &, bool);
template MANTID_API_DLL WorkspaceGroup_sptr executeBinaryOperation(
    const std::string &, const WorkspaceGroup_sptr, const MatrixWorkspace_sptr,
    bool, bool, const std::string &, bool);
template MANTID_API_DLL WorkspaceGroup_sptr executeBinaryOperation(
    const std::string &, const MatrixWorkspace_sptr, const WorkspaceGroup_sptr,
    bool, bool, const std::string &, bool);

template MANTID_API_DLL IMDWorkspace_sptr executeBinaryOperation(
    const std::string &, const IMDWorkspace_sptr, const IMDWorkspace_sptr, bool,
    bool, const std::string &, bool);
template MANTID_API_DLL WorkspaceGroup_sptr executeBinaryOperation(
    const std::string &, const WorkspaceGroup_sptr, const IMDWorkspace_sptr,
    bool, bool, const std::string &, bool);
template MANTID_API_DLL WorkspaceGroup_sptr executeBinaryOperation(
    const std::string &, const IMDWorkspace_sptr, const WorkspaceGroup_sptr,
    bool, bool, const std::string &, bool);

template MANTID_API_DLL IMDWorkspace_sptr executeBinaryOperation(
    const std::string &, const IMDWorkspace_sptr, const MatrixWorkspace_sptr,
    bool, bool, const std::string &, bool);
template MANTID_API_DLL IMDWorkspace_sptr executeBinaryOperation(
    const std::string &, const MatrixWorkspace_sptr, const IMDWorkspace_sptr,
    bool, bool, const std::string &, bool);

template MANTID_API_DLL IMDHistoWorkspace_sptr executeBinaryOperation(
    const std::string &, const IMDHistoWorkspace_sptr,
    const IMDHistoWorkspace_sptr, bool, bool, const std::string &, bool);
template MANTID_API_DLL IMDHistoWorkspace_sptr executeBinaryOperation(
    const std::string &, const IMDHistoWorkspace_sptr,
    const MatrixWorkspace_sptr, bool, bool, const std::string &, bool);
template MANTID_API_DLL IMDHistoWorkspace_sptr executeBinaryOperation(
    const std::string &, const MatrixWorkspace_sptr,
    const IMDHistoWorkspace_sptr, bool, bool, const std::string &, bool);

/** Build up an BinaryOperationTable for performing a binary operation
 * e.g. lhs = (lhs + rhs)
 * where the spectra in rhs are to go into lhs.
 * This function looks to match the detector IDs in rhs to those in the lhs.
 *
 * @param lhs :: matrix workspace in which the operation is being done.
 * @param rhs :: matrix workspace on the right hand side of the operand
 * @return map from detector ID to workspace index for the RHS workspace.
 *        NULL if there is not a 1:1 mapping from detector ID to workspace index
 *(e.g more than one detector per pixel).
 */
BinaryOperationTable_sptr
buildBinaryOperationTable(const MatrixWorkspace_const_sptr &lhs,
                          const MatrixWorkspace_const_sptr &rhs) {
  // An addition table is a list of pairs:
  //  First int = workspace index in the EW being added
  //  Second int = workspace index to which it will be added in the OUTPUT EW.
  //  -1 if it should add a new entry at the end.
  auto table = std::make_shared<BinaryOperationTable>();

  auto rhs_nhist = static_cast<int>(rhs->getNumberHistograms());
  auto lhs_nhist = static_cast<int>(lhs->getNumberHistograms());

  // Initialize the table; filled with -1 meaning no match
  table->resize(lhs_nhist, -1);

  const detid2index_map rhs_det_to_wi = rhs->getDetectorIDToWorkspaceIndexMap();

  PARALLEL_FOR_NO_WSP_CHECK()
  for (int lhsWI = 0; lhsWI < lhs_nhist; lhsWI++) {
    bool done = false;

    // List of detectors on lhs side
    const auto &lhsDets = lhs->getSpectrum(lhsWI).getDetectorIDs();

    // ----------------- Matching Workspace Indices and Detector IDs
    // --------------------------------------
    // First off, try to match the workspace indices. Most times, this will be
    // ok right away.
    int64_t rhsWI = lhsWI;
    if (rhsWI < rhs_nhist) // don't go out of bounds
    {
      // Get the detector IDs at that workspace index.
      const auto &rhsDets = rhs->getSpectrum(rhsWI).getDetectorIDs();

      // Checks that lhsDets is a subset of rhsDets
      if (std::includes(rhsDets.begin(), rhsDets.end(), lhsDets.begin(),
                        lhsDets.end())) {
        // We found the workspace index right away. No need to keep looking
        (*table)[lhsWI] = rhsWI;
        done = true;
      }
    }

    // ----------------- Scrambled Detector IDs with one Detector per Spectrum
    // --------------------------------------
    if (!done && (lhsDets.size() == 1)) {
      // Didn't find it. Try to use the RHS map.

      // First, we have to get the (single) detector ID of the LHS
      auto lhsDets_it = lhsDets.cbegin();
      detid_t lhs_detector_ID = *lhsDets_it;

      // Now we use the RHS map to find it. This only works if both the lhs and
      // rhs have 1 detector per pixel
      auto map_it = rhs_det_to_wi.find(lhs_detector_ID);
      if (map_it != rhs_det_to_wi.end()) {
        rhsWI = map_it->second; // This is the workspace index in the RHS that
                                // matched lhs_detector_ID
      } else {
        // Did not find it!
        rhsWI = -1; // Marker to mean its not in the LHS.

        //            std::ostringstream mess;
        //            mess << "BinaryOperation: cannot find a RHS spectrum that
        //            contains the detectors in LHS workspace index " << lhsWI
        //            << "\n";
        //            throw std::runtime_error(mess.str());
      }
      (*table)[lhsWI] = rhsWI;
      done = true; // Great, we did it.
    }

    // ----------------- LHS detectors are subset of RHS, which are Grouped
    // --------------------------------------
    if (!done) {

      // Didn't find it? Now we need to iterate through the output workspace to
      //  match the detector ID.
      // NOTE: This can be SUPER SLOW!
      for (rhsWI = 0; rhsWI < static_cast<int64_t>(rhs_nhist); rhsWI++) {
        const auto &rhsDets = rhs->getSpectrum(rhsWI).getDetectorIDs();

        // Checks that lhsDets is a subset of rhsDets
        if (std::includes(rhsDets.begin(), rhsDets.end(), lhsDets.begin(),
                          lhsDets.end())) {
          // This one is right. Now we can stop looking.
          (*table)[lhsWI] = rhsWI;
          done = true;
          continue;
        }
      }
    }

    // ------- Still nothing ! -----------
    if (!done) {
      (*table)[lhsWI] = -1;

      //          std::ostringstream mess;
      //          mess << "BinaryOperation: cannot find a RHS spectrum that
      //          contains the detectors in LHS workspace index " << lhsWI <<
      //          "\n";
      //          throw std::runtime_error(mess.str());
    }
  }

  return table;
}
} // namespace OperatorOverloads

/** Performs a comparison operation on two workspaces, using the
 * CompareWorkspaces algorithm
 *
 *  @param lhs :: left hand side workspace shared pointer
 *  @param rhs :: right hand side workspace shared pointer
 *  @param tolerance :: acceptable difference for floating point numbers
 *  @return bool, true if workspaces match
 */
bool equals(const MatrixWorkspace_sptr &lhs, const MatrixWorkspace_sptr &rhs,
            double tolerance) {
  IAlgorithm_sptr alg =
      AlgorithmManager::Instance().createUnmanaged("CompareWorkspaces");
  alg->setChild(true);
  alg->setRethrows(false);
  alg->initialize();
  alg->setProperty<MatrixWorkspace_sptr>("Workspace1", lhs);
  alg->setProperty<MatrixWorkspace_sptr>("Workspace2", rhs);
  alg->setProperty<double>("Tolerance", tolerance);
  // Rest: use default

  alg->execute();
  if (!alg->isExecuted()) {
    std::string message = "Error while executing operation: CompareWorkspaces";
    throw std::runtime_error(message);
  }
  return alg->getProperty("Result");
}

/** Creates a temporary single value workspace the error is set to zero
 *  @param rhsValue :: the value to use
 *  @return The value in a workspace shared pointer
 */
static MatrixWorkspace_sptr createWorkspaceSingleValue(const double &rhsValue) {
  MatrixWorkspace_sptr retVal =
      WorkspaceFactory::Instance().create("WorkspaceSingleValue", 1, 1, 1);
  retVal->dataY(0)[0] = rhsValue;

  return retVal;
}

using OperatorOverloads::executeBinaryOperation;

/** Adds two workspaces
 *  @param lhs :: left hand side workspace shared pointer
 *  @param rhs :: right hand side workspace shared pointer
 *  @return The result in a workspace shared pointer
 */
MatrixWorkspace_sptr operator+(const MatrixWorkspace_sptr &lhs,
                               const MatrixWorkspace_sptr &rhs) {
  return executeBinaryOperation<MatrixWorkspace_sptr, MatrixWorkspace_sptr,
                                MatrixWorkspace_sptr>("Plus", lhs, rhs);
}

/** Adds a workspace to a single value
 *  @param lhs ::      left hand side workspace shared pointer
 *  @param rhsValue :: the single value
 *  @return The result in a workspace shared pointer
 */
MatrixWorkspace_sptr operator+(const MatrixWorkspace_sptr &lhs,
                               const double &rhsValue) {
  return executeBinaryOperation<MatrixWorkspace_sptr, MatrixWorkspace_sptr,
                                MatrixWorkspace_sptr>(
      "Plus", lhs, createWorkspaceSingleValue(rhsValue));
}

/** Subtracts two workspaces
 *  @param lhs :: left hand side workspace shared pointer
 *  @param rhs :: right hand side workspace shared pointer
 *  @return The result in a workspace shared pointer
 */
MatrixWorkspace_sptr operator-(const MatrixWorkspace_sptr &lhs,
                               const MatrixWorkspace_sptr &rhs) {
  return executeBinaryOperation<MatrixWorkspace_sptr, MatrixWorkspace_sptr,
                                MatrixWorkspace_sptr>("Minus", lhs, rhs);
}

/** Subtracts  a single value from a workspace
 *  @param lhs ::      left hand side workspace shared pointer
 *  @param rhsValue :: the single value
 *  @return The result in a workspace shared pointer
 */
MatrixWorkspace_sptr operator-(const MatrixWorkspace_sptr &lhs,
                               const double &rhsValue) {
  return executeBinaryOperation<MatrixWorkspace_sptr, MatrixWorkspace_sptr,
                                MatrixWorkspace_sptr>(
      "Minus", lhs, createWorkspaceSingleValue(rhsValue));
}

/** Subtracts a workspace from a single value
 *  @param lhsValue :: the single value
 *  @param rhs :: right-hand side workspace shared pointer
 *  @return The result in a workspace shared pointer
 */
MatrixWorkspace_sptr operator-(const double &lhsValue,
                               const MatrixWorkspace_sptr &rhs) {
  return executeBinaryOperation<MatrixWorkspace_sptr, MatrixWorkspace_sptr,
                                MatrixWorkspace_sptr>(
      "Minus", createWorkspaceSingleValue(lhsValue), rhs);
}
/** Multiply two workspaces
 *  @param lhs :: left hand side workspace shared pointer
 *  @param rhs :: right hand side workspace shared pointer
 *  @return The result in a workspace shared pointer
 */
MatrixWorkspace_sptr operator*(const MatrixWorkspace_sptr &lhs,
                               const MatrixWorkspace_sptr &rhs) {
  return executeBinaryOperation<MatrixWorkspace_sptr, MatrixWorkspace_sptr,
                                MatrixWorkspace_sptr>("Multiply", lhs, rhs);
}

/** Multiply a workspace and a single value
 *  @param lhs ::      left hand side workspace shared pointer
 *  @param rhsValue :: the single value
 *  @return The result in a workspace shared pointer
 */
MatrixWorkspace_sptr operator*(const MatrixWorkspace_sptr &lhs,
                               const double &rhsValue) {
  return executeBinaryOperation<MatrixWorkspace_sptr, MatrixWorkspace_sptr,
                                MatrixWorkspace_sptr>(
      "Multiply", lhs, createWorkspaceSingleValue(rhsValue));
}

/** Multiply a workspace and a single value. Allows you to write, e.g.,
 * 2*workspace.
 *  @param lhsValue :: the single value
 *  @param rhs ::      workspace shared pointer
 *  @return The result in a workspace shared pointer
 */
MatrixWorkspace_sptr operator*(const double &lhsValue,
                               const MatrixWorkspace_sptr &rhs) {
  return executeBinaryOperation<MatrixWorkspace_sptr, MatrixWorkspace_sptr,
                                MatrixWorkspace_sptr>(
      "Multiply", createWorkspaceSingleValue(lhsValue), rhs);
}

/** Divide two workspaces
 *  @param lhs :: left hand side workspace shared pointer
 *  @param rhs :: right hand side workspace shared pointer
 *  @return The result in a workspace shared pointer
 */
MatrixWorkspace_sptr operator/(const MatrixWorkspace_sptr &lhs,
                               const MatrixWorkspace_sptr &rhs) {
  return executeBinaryOperation<MatrixWorkspace_sptr, MatrixWorkspace_sptr,
                                MatrixWorkspace_sptr>("Divide", lhs, rhs);
}

/** Divide a workspace by a single value
 *  @param lhs ::      left hand side workspace shared pointer
 *  @param rhsValue :: the single value
 *  @return The result in a workspace shared pointer
 */
MatrixWorkspace_sptr operator/(const MatrixWorkspace_sptr &lhs,
                               const double &rhsValue) {
  return executeBinaryOperation<MatrixWorkspace_sptr, MatrixWorkspace_sptr,
                                MatrixWorkspace_sptr>(
      "Divide", lhs, createWorkspaceSingleValue(rhsValue));
}

/** Divide a single value and a workspace. Allows you to write, e.g.,
 * 2/workspace.
 *  @param lhsValue :: the single value
 *  @param rhs ::      workspace shared pointer
 *  @return The result in a workspace shared pointer
 */
MatrixWorkspace_sptr operator/(const double &lhsValue,
                               const MatrixWorkspace_sptr &rhs) {
  return executeBinaryOperation<MatrixWorkspace_sptr, MatrixWorkspace_sptr,
                                MatrixWorkspace_sptr>(
      "Divide", createWorkspaceSingleValue(lhsValue), rhs);
}

/** Adds two workspaces
 *  @param lhs :: left hand side workspace shared pointer
 *  @param rhs :: right hand side workspace shared pointer
 *  @return The result in a workspace shared pointer
 */
MatrixWorkspace_sptr operator+=(const MatrixWorkspace_sptr &lhs,
                                const MatrixWorkspace_sptr &rhs) {
  return executeBinaryOperation<MatrixWorkspace_sptr, MatrixWorkspace_sptr,
                                MatrixWorkspace_sptr>("Plus", lhs, rhs, true);
}

/** Adds a single value to a workspace
 *  @param lhs ::      workspace shared pointer
 *  @param rhsValue :: the single value
 *  @return The result in a workspace shared pointer
 */
MatrixWorkspace_sptr operator+=(const MatrixWorkspace_sptr &lhs,
                                const double &rhsValue) {
  return executeBinaryOperation<MatrixWorkspace_sptr, MatrixWorkspace_sptr,
                                MatrixWorkspace_sptr>(
      "Plus", lhs, createWorkspaceSingleValue(rhsValue), true);
}

/** Subtracts two workspaces
 *  @param lhs :: left hand side workspace shared pointer
 *  @param rhs :: right hand side workspace shared pointer
 *  @return The result in a workspace shared pointer
 */
MatrixWorkspace_sptr operator-=(const MatrixWorkspace_sptr &lhs,
                                const MatrixWorkspace_sptr &rhs) {
  return executeBinaryOperation<MatrixWorkspace_sptr, MatrixWorkspace_sptr,
                                MatrixWorkspace_sptr>("Minus", lhs, rhs, true);
}

/** Subtracts a single value from a workspace
 *  @param lhs ::      workspace shared pointer
 *  @param rhsValue :: the single value
 *  @return The result in a workspace shared pointer
 */
MatrixWorkspace_sptr operator-=(const MatrixWorkspace_sptr &lhs,
                                const double &rhsValue) {
  return executeBinaryOperation<MatrixWorkspace_sptr, MatrixWorkspace_sptr,
                                MatrixWorkspace_sptr>(
      "Minus", lhs, createWorkspaceSingleValue(rhsValue), true);
}

/** Multiply two workspaces
 *  @param lhs :: left hand side workspace shared pointer
 *  @param rhs :: right hand side workspace shared pointer
 *  @return The result in a workspace shared pointer
 */
MatrixWorkspace_sptr operator*=(const MatrixWorkspace_sptr &lhs,
                                const MatrixWorkspace_sptr &rhs) {
  return executeBinaryOperation<MatrixWorkspace_sptr, MatrixWorkspace_sptr,
                                MatrixWorkspace_sptr>("Multiply", lhs, rhs,
                                                      true);
}

/** Multiplies a workspace by a single value
 *  @param lhs ::      workspace shared pointer
 *  @param rhsValue :: the single value
 *  @return The result in a workspace shared pointer
 */
MatrixWorkspace_sptr operator*=(const MatrixWorkspace_sptr &lhs,
                                const double &rhsValue) {
  return executeBinaryOperation<MatrixWorkspace_sptr, MatrixWorkspace_sptr,
                                MatrixWorkspace_sptr>(
      "Multiply", lhs, createWorkspaceSingleValue(rhsValue), true);
}

/** Divide two workspaces
 *  @param lhs :: left hand side workspace shared pointer
 *  @param rhs :: right hand side workspace shared pointer
 *  @return The result in a workspace shared pointer
 */
MatrixWorkspace_sptr operator/=(const MatrixWorkspace_sptr &lhs,
                                const MatrixWorkspace_sptr &rhs) {
  return executeBinaryOperation<MatrixWorkspace_sptr, MatrixWorkspace_sptr,
                                MatrixWorkspace_sptr>("Divide", lhs, rhs, true);
}

/** Divides a workspace by a single value
 *  @param lhs ::      workspace shared pointer
 *  @param rhsValue :: the single value
 *  @return The result in a workspace shared pointer
 */
MatrixWorkspace_sptr operator/=(const MatrixWorkspace_sptr &lhs,
                                const double &rhsValue) {
  return executeBinaryOperation<MatrixWorkspace_sptr, MatrixWorkspace_sptr,
                                MatrixWorkspace_sptr>(
      "Divide", lhs, createWorkspaceSingleValue(rhsValue), true);
}

//----------------------------------------------------------------------
// Now the WorkspaceHelpers methods
//----------------------------------------------------------------------

/** Checks whether the bins (X values) of two workspace are the same
 *  @param ws1 :: The first workspace
 *  @param ws2 :: The second workspace
 *  @param firstOnly :: If true, only the first spectrum is checked. If false
 * (the default), all spectra
 *                   are checked and the two workspaces must have the same
 * number of spectra
 *  @return True if the test passes
 */
bool WorkspaceHelpers::matchingBins(const MatrixWorkspace &ws1,
                                    const MatrixWorkspace &ws2,
                                    const bool firstOnly) {
  // First of all, the first vector must be the same size
  if (ws1.x(0).size() != ws2.x(0).size())
    return false;

  // Now check the first spectrum
  const double firstWS = std::accumulate(ws1.x(0).begin(), ws1.x(0).end(), 0.);
  const double secondWS = std::accumulate(ws2.x(0).begin(), ws2.x(0).end(), 0.);
  if (std::abs(firstWS) < 1.0E-7 && std::abs(secondWS) < 1.0E-7) {
    for (size_t i = 0; i < ws1.x(0).size(); i++) {
      if (std::abs(ws1.x(0)[i] - ws2.x(0)[i]) > 1.0E-7)
        return false;
    }
  } else if (std::abs(firstWS - secondWS) /
                 std::max<double>(std::abs(firstWS), std::abs(secondWS)) >
             1.0E-7)
    return false;

  // If we were only asked to check the first spectrum, return now
  if (firstOnly)
    return true;

  // Check that total size of workspace is the same
  if (ws1.size() != ws2.size())
    return false;
  // If that passes then check whether all the X vectors are shared
  if (sharedXData(ws1) && sharedXData(ws2))
    return true;

  // If that didn't pass then explicitly check 1 in 10 of the vectors (min 10,
  // max 100)
  const size_t numHist = ws1.getNumberHistograms();
  size_t numberToCheck = numHist / 10;
  if (numberToCheck < 10)
    numberToCheck = 10;
  if (numberToCheck > 100)
    numberToCheck = 100;
  size_t step = numHist / numberToCheck;
  if (!step)
    step = 1;
  for (size_t i = step; i < numHist; i += step) {
    const double firstWSLoop =
        std::accumulate(ws1.x(i).begin(), ws1.x(i).end(), 0.);
    const double secondWSLoop =
        std::accumulate(ws2.x(i).begin(), ws2.x(i).end(), 0.);
    if (std::abs(firstWSLoop) < 1.0E-7 && std::abs(secondWSLoop) < 1.0E-7) {
      for (size_t j = 0; j < ws1.x(i).size(); j++) {
        if (std::abs(ws1.x(i)[j] - ws2.x(i)[j]) > 1.0E-7)
          return false;
      }
    } else if (std::abs(firstWSLoop - secondWSLoop) /
                   std::max<double>(std::abs(firstWSLoop),
                                    std::abs(secondWSLoop)) >
               1.0E-7)
      return false;
  }

  return true;
}

/// Checks whether all the X vectors in a workspace are the same one underneath
bool WorkspaceHelpers::sharedXData(const MatrixWorkspace &WS) {
  const double &first = WS.x(0)[0];
  const size_t numHist = WS.getNumberHistograms();
  for (size_t i = 1; i < numHist; ++i) {
    if (&first != &(WS.x(i)[0]))
      return false;
  }
  return true;
}

/** Divides the data in a workspace by the bin width to make it a distribution.
 *  Can also reverse this operation (i.e. multiply by the bin width).
 *  Sets the isDistribution() flag accordingly.
 *  @param workspace :: The workspace on which to carry out the operation
 *  @param forwards :: If true (the default) divides by bin width, if false
 * multiplies
 */
void WorkspaceHelpers::makeDistribution(const MatrixWorkspace_sptr &workspace,
                                        const bool forwards) {
  // If we're not able to get a writable reference to Y, then this is an event
  // workspace, which we can't operate on.
  if (workspace->id() == "EventWorkspace")
    throw std::runtime_error("Event workspaces cannot be directly converted "
                             "into distributions.");

  const size_t numberOfSpectra = workspace->getNumberHistograms();
  if (workspace->histogram(0).xMode() ==
      HistogramData::Histogram::XMode::Points) {
    throw std::runtime_error(
        "Workspace is using point data for x (should be bin edges).");
  }
  for (size_t i = 0; i < numberOfSpectra; ++i) {
    if (forwards) {
      workspace->convertToFrequencies(i);
    } else {
      workspace->convertToCounts(i);
    }
  }
}

} // namespace API
} // namespace Mantid
