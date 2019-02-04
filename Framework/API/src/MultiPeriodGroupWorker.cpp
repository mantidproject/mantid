// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/MultiPeriodGroupWorker.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/Strings.h"

using namespace Mantid::Kernel;

namespace Mantid {
namespace API {

/**
 * Constructor
 * @param workspacePropertyName : Property name to treat as source of
 * multiperiod workspaces.
 */
MultiPeriodGroupWorker::MultiPeriodGroupWorker(
    const std::string &workspacePropertyName)
    : m_workspacePropertyName(workspacePropertyName) {}

/**
 * Try to add the input workspace to the multiperiod input group list.
 * @param ws: candidate workspace
 * @param vecMultiPeriodWorkspaceGroups: Vector of multi period workspace
 * groups.
 * @param vecWorkspaceGroups: Vector of non-multi period workspace groups.
 */
void MultiPeriodGroupWorker::tryAddInputWorkspaceToInputGroups(
    Workspace_sptr ws,
    MultiPeriodGroupWorker::VecWSGroupType &vecMultiPeriodWorkspaceGroups,
    MultiPeriodGroupWorker::VecWSGroupType &vecWorkspaceGroups) const {
  WorkspaceGroup_sptr inputGroup =
      boost::dynamic_pointer_cast<WorkspaceGroup>(ws);
  if (inputGroup) {
    if (inputGroup->isMultiperiod()) {
      vecMultiPeriodWorkspaceGroups.push_back(inputGroup);
    } else {
      vecWorkspaceGroups.push_back(inputGroup);
    }
  }
}

MultiPeriodGroupWorker::VecWSGroupType
MultiPeriodGroupWorker::findMultiPeriodGroups(
    Algorithm const *const sourceAlg) const {
  if (!sourceAlg->isInitialized()) {
    throw std::invalid_argument("Algorithm must be initialized");
  }
  VecWSGroupType vecMultiPeriodWorkspaceGroups;
  VecWSGroupType vecWorkspaceGroups;

  // Handles the case in which the algorithm is providing a non-workspace
  // property as an input.
  // This is currenly the case for algorithms that take an array of strings as
  // an input where each entry is the name of a workspace.
  if (this->useCustomWorkspaceProperty()) {
    using WorkspaceNameType = std::vector<std::string>;

    // Perform a check that the input property is the correct type.
    Property *inputProperty =
        sourceAlg->getProperty(this->m_workspacePropertyName);

    if (!dynamic_cast<ArrayProperty<std::string> *>(inputProperty)) {
      throw std::runtime_error("Support for custom input workspaces that are "
                               "not string Arrays are not currently "
                               "supported.");
      /*Note that we could extend this algorithm to cover other input property
       * types if required, but we don't need that funtionality now.*/
    }

    WorkspaceNameType workspaces =
        sourceAlg->getProperty(this->m_workspacePropertyName);

    // Inspect all the input workspaces in the ArrayProperty input.
    for (auto &workspace : workspaces) {
      Workspace_sptr ws = AnalysisDataService::Instance().retrieve(workspace);
      if (!ws) {
        throw Kernel::Exception::NotFoundError("Workspace", workspace);
      }
      tryAddInputWorkspaceToInputGroups(ws, vecMultiPeriodWorkspaceGroups,
                                        vecWorkspaceGroups);
    }
  } else {
    using WorkspaceVector = std::vector<boost::shared_ptr<Workspace>>;
    WorkspaceVector inWorkspaces;
    sourceAlg->findWorkspaces(inWorkspaces, Direction::Input);
    for (auto &inWorkspace : inWorkspaces) {
      tryAddInputWorkspaceToInputGroups(
          inWorkspace, vecMultiPeriodWorkspaceGroups, vecWorkspaceGroups);
    }
  }

  if (!vecMultiPeriodWorkspaceGroups.empty() && !vecWorkspaceGroups.empty()) {
    throw std::invalid_argument(
        "The input contains a mix of multi-period and other workspaces.");
  }

  validateMultiPeriodGroupInputs(vecMultiPeriodWorkspaceGroups);

  return vecMultiPeriodWorkspaceGroups;
}

bool MultiPeriodGroupWorker::useCustomWorkspaceProperty() const {
  return !this->m_workspacePropertyName.empty();
}

/**
 * Creates a list of input workspaces as a string for a given period using all
 * nested workspaces at that period within all group workspaces.
 *
 * This requires a little explanation, because this is the reason that this
 * algorithm needs a customised overriden checkGroups and processGroups
 * method:
 *
 * Say you have two multiperiod group workspaces A and B and an output workspace
 * C. A contains matrix workspaces A_1 and A_2, and B contains matrix workspaces
 * B_1 and B2. Because this is multiperiod data. A_1 and B_1 share the same
 * period, as do A_2 and B_2. So merging must be with respect to workspaces of
 * equivalent periods. Therefore, merging must be A_1 + B_1 = C_1 and
 * A_2 + B_2 = C_2. This method constructs the inputs for a nested call to
 * MultiPeriodGroupAlgorithm in this manner.
 *
 * @param periodIndex : zero based index denoting the period.
 * @param vecWorkspaceGroups : Vector of workspace groups
 * @return comma separated string of input workspaces.
 */
std::string MultiPeriodGroupWorker::createFormattedInputWorkspaceNames(
    const size_t &periodIndex, const VecWSGroupType &vecWorkspaceGroups) const {
  std::string prefix;
  std::string inputWorkspaces;
  for (const auto &vecWorkspaceGroup : vecWorkspaceGroups) {
    inputWorkspaces +=
        prefix + vecWorkspaceGroup->getItem(periodIndex)->getName();
    prefix = ",";
  }
  return inputWorkspaces;
}

/**
 * 1) Looks for input workspace properties that are of type WorkspaceGroup.
 * 2) If a multiperiod workspace has been set to that property then ..
 * 3) Extracts the individual period workspace from that WorkspaceGroup
 * 4) Manually sets that individual period workspace as the corresponding
 * property on the targetAlgorithm.
 * Copy input workspaces assuming we are working with multi-period groups
 * workspace inputs.
 * @param targetAlg: The spawned algorithm to set the properties on.
 * @param sourceAlg: Algorithm being executed with multiperiod group workspaces.
 * @param periodNumber: The relevant period number used to index into the group
 * workspaces
 */
void MultiPeriodGroupWorker::copyInputWorkspaceProperties(
    IAlgorithm *targetAlg, IAlgorithm *sourceAlg,
    const int &periodNumber) const {
  std::vector<Property *> props = sourceAlg->getProperties();
  for (auto prop : props) {
    if (prop) {
      if (prop->direction() == Direction::Input) {
        if (const IWorkspaceProperty *wsProp =
                dynamic_cast<IWorkspaceProperty *>(prop)) {
          if (WorkspaceGroup_sptr inputws =
                  boost::dynamic_pointer_cast<WorkspaceGroup>(
                      wsProp->getWorkspace())) {
            if (inputws->isMultiperiod()) {
              targetAlg->setProperty(prop->name(),
                                     inputws->getItem(periodNumber - 1));
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
bool MultiPeriodGroupWorker::processGroups(
    Algorithm *const sourceAlg,
    const VecWSGroupType &vecMultiPeriodGroups) const {
  // If we are not processing multiperiod groups, use the base behaviour.
  if (vecMultiPeriodGroups.empty()) {
    return false; // Indicates that this is not a multiperiod group workspace.
  }
  Property *outputWorkspaceProperty = sourceAlg->getProperty("OutputWorkspace");
  const std::string outName = outputWorkspaceProperty->value();

  const size_t nPeriods = vecMultiPeriodGroups[0]->size();
  WorkspaceGroup_sptr outputWS = boost::make_shared<WorkspaceGroup>();
  AnalysisDataService::Instance().addOrReplace(outName, outputWS);

  double progress_proportion = 1.0 / static_cast<double>(nPeriods);
  // Loop through all the periods. Create spawned algorithms of the same type as
  // this to process pairs from the input groups.
  for (size_t i = 0; i < nPeriods; ++i) {
    const int periodNumber = static_cast<int>(i + 1);
    // use create Child Algorithm that look like this one
    Algorithm_sptr alg = sourceAlg->createChildAlgorithm(
        sourceAlg->name(), progress_proportion * periodNumber,
        progress_proportion * (1 + periodNumber), sourceAlg->isLogging(),
        sourceAlg->version());
    if (!alg) {
      throw std::runtime_error("Algorithm creation failed.");
    }
    // Don't make the new algorithm a child so that it's workspaces are stored
    // correctly
    alg->setChild(false);
    alg->setRethrows(true);
    alg->initialize();
    // Copy properties that aren't workspaces properties.
    sourceAlg->copyNonWorkspaceProperties(alg.get(), periodNumber);

    if (this->useCustomWorkspaceProperty()) {
      const std::string inputWorkspaces =
          createFormattedInputWorkspaceNames(i, vecMultiPeriodGroups);
      // Set the input workspace property.
      alg->setPropertyValue(this->m_workspacePropertyName, inputWorkspaces);
    } else {
      // Configure input properties that are group workspaces.
      copyInputWorkspaceProperties(alg.get(), sourceAlg, periodNumber);
    }
    const std::string outName_i = outName + "_" + Strings::toString(i + 1);
    alg->setPropertyValue("OutputWorkspace", outName_i);
    // Run the spawned algorithm.
    if (!alg->execute()) {
      throw std::runtime_error("Execution of " + sourceAlg->name() +
                               " for group entry " + Strings::toString(i + 1) +
                               " failed.");
    }
    // Add the output workpace from the spawned algorithm to the group.
    outputWS->add(outName_i);
  }

  sourceAlg->setProperty("OutputWorkspace", outputWS);

  return true;
}

/**
 * Validate the multiperiods workspace groups. Gives the opportunity to exit
 * processing if things don't look right.
 * @param vecMultiPeriodGroups : vector of multiperiod groups.
 */
void MultiPeriodGroupWorker::validateMultiPeriodGroupInputs(
    const VecWSGroupType &vecMultiPeriodGroups) const {
  const size_t multiPeriodGroupsSize = vecMultiPeriodGroups.size();

  if (multiPeriodGroupsSize > 0) {
    const size_t benchMarkGroupSize = vecMultiPeriodGroups[0]->size();
    for (size_t i = 0; i < multiPeriodGroupsSize; ++i) {
      WorkspaceGroup_sptr currentGroup = vecMultiPeriodGroups[i];
      if (currentGroup->size() != benchMarkGroupSize) {
        throw std::runtime_error("Not all the input Multi-period-group input "
                                 "workspaces are the same size.");
      }
      for (size_t j = 0; j < currentGroup->size(); ++j) {
        MatrixWorkspace_const_sptr currentNestedWS =
            boost::dynamic_pointer_cast<const MatrixWorkspace>(
                currentGroup->getItem(j));
        Property *nPeriodsProperty =
            currentNestedWS->run().getLogData("nperiods");
        size_t nPeriods = std::stoul(nPeriodsProperty->value());
        if (nPeriods != benchMarkGroupSize) {
          throw std::runtime_error("Missmatch between nperiods log and the "
                                   "number of workspaces in the input group: " +
                                   vecMultiPeriodGroups[i]->getName());
        }
        Property *currentPeriodProperty =
            currentNestedWS->run().getLogData("current_period");
        size_t currentPeriod = std::stoul(currentPeriodProperty->value());
        if (currentPeriod != (j + 1)) {
          throw std::runtime_error("Multiperiod group workspaces must be "
                                   "ordered by current_period. Correct: " +
                                   currentNestedWS->getName());
        }
      }
    }
  }
}

} // namespace API
} // namespace Mantid
