// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/ScriptBuilder.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/AlgorithmHistory.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/HistoryItem.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/PropertyHistory.h"
#include "MantidKernel/WarningSuppressions.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/range/algorithm/remove_if.hpp>
#include <boost/utility.hpp>
#include <set>
#include <utility>

namespace Mantid::API {

using Mantid::Kernel::PropertyHistory_const_sptr;
using Mantid::Kernel::PropertyHistory_sptr;

namespace {
Mantid::Kernel::Logger g_log("ScriptBuilder");
}

const std::string COMMENT_ALG = "Comment";

ScriptBuilder::ScriptBuilder(const std::shared_ptr<HistoryView> &view, std::string versionSpecificity,
                             bool appendTimestamp, std::vector<std::string> ignoreTheseAlgs,
                             std::vector<std::vector<std::string>> ignoreTheseAlgProperties, bool appendExecCount)
    : m_historyItems(view->getAlgorithmsList()), m_output(), m_versionSpecificity(std::move(versionSpecificity)),
      m_timestampCommands(appendTimestamp), m_algsToIgnore(std::move(ignoreTheseAlgs)),
      m_propertiesToIgnore(std::move(ignoreTheseAlgProperties)), m_execCount(appendExecCount) {}

/**
 * Build a python script for each algorithm included in the history view.
 *
 * @return a formatted python string of the history
 */
const std::string ScriptBuilder::build() {
  std::ostringstream os;
  os << "from mantid.simpleapi import *\n\n";
  auto iter = m_historyItems.begin();
  for (; iter != m_historyItems.end(); ++iter) {
    writeHistoryToStream(os, iter);
  }
  return os.str();
}

/**
 * Write out an algorithm to the script.
 *
 * If the entry is unrolled this will recurse and output the children of
 * that entry instead. If not, it will just output the algorithm to the stream.
 *
 * @param os :: output string stream to append algorithms to.
 * @param iter :: reference to the iterator pointing to the vector of history
 *items
 * @param depth :: count of how far we've recursed into the history
 */
void ScriptBuilder::writeHistoryToStream(std::ostringstream &os, std::vector<HistoryItem>::const_iterator &iter,
                                         int depth) {
  auto algHistory = iter->getAlgorithmHistory();
  if (iter->isUnrolled()) {
    os << "\n";
    os << std::string(depth, '#');
    os << " Child algorithms of " << algHistory->name() << "\n";

    // don't create a line for the algorithm, just output its children
    buildChildren(os, iter, depth + 1);

    os << std::string(depth, '#');
    os << " End of child algorithms of " << algHistory->name() << "\n";

    if (boost::next(iter) == m_historyItems.end() || !boost::next(iter)->isUnrolled()) {

      if (m_timestampCommands) {
        os << " # " << algHistory->executionDate().toISO8601String();
      }

      os << "\n";
    }
  } else {
    // create the string for this algorithm if not found to be in the ignore
    // list
    if (!(std::find(m_algsToIgnore.begin(), m_algsToIgnore.end(), algHistory->name()) != m_algsToIgnore.end())) {
      createStringForAlg(os, algHistory);
    }
  }
}

void ScriptBuilder::createStringForAlg(std::ostringstream &os,
                                       std::shared_ptr<const Mantid::API::AlgorithmHistory> &algHistory) {
  os << buildAlgorithmString(*algHistory);
  if (m_timestampCommands) {
    os << " # " << algHistory->executionDate().toISO8601String();
  }

  if (m_execCount) {
    if (m_timestampCommands) {
      os << " execCount: " << algHistory->execCount();
    } else {
      os << " # execCount: " << algHistory->execCount();
    }
  }

  os << "\n";
}

/**
 * Iterate over each of the items children and output them to the script.
 *
 * This moves the iterator forward over each of the child records and writes
 *them to
 * the stream.
 *
 * @param os :: output string stream to append algorithms to.
 * @param iter :: reference to the iterator pointing to the vector of history
 *items
 * @param depth :: count of how far we've recursed into the history
 */
void ScriptBuilder::buildChildren(std::ostringstream &os, std::vector<HistoryItem>::const_iterator &iter, int depth) {
  size_t numChildren = iter->numberOfChildren();
  ++iter; // move to first child
  for (size_t i = 0; i < numChildren && iter != m_historyItems.end(); ++i, ++iter) {
    writeHistoryToStream(os, iter, depth);
  }
  --iter;
}

/**
 * Build the script output for a single comment
 *
 * @param algHistory :: pointer to an algorithm history object
 * @returns std::string to run this algorithm
 */
const std::string ScriptBuilder::buildCommentString(const AlgorithmHistory &algHistory) {
  std::ostringstream comment;
  const std::string name = algHistory.name();
  if (name == COMMENT_ALG) {
    auto props = algHistory.getProperties();
    for (auto &prop : props) {
      if (prop->name() == "Text") {
        comment << "# " << prop->value();
      }
    }
  }
  return comment.str();
}

/**
 * Build the script output for a single algorithm
 *
 * @param algHistory :: pointer to an algorithm history object
 * @returns std::string to run this algorithm
 */
const std::string ScriptBuilder::buildAlgorithmString(const AlgorithmHistory &algHistory) {
  std::ostringstream properties;
  const std::string name = algHistory.name();

  if (name == COMMENT_ALG)
    return buildCommentString(algHistory);

  auto props = algHistory.getProperties();

  try {
    Mantid::API::Algorithm_sptr algFresh;
    // create a fresh version of the algorithm - unmanaged
    if (name == "Load") {
      int version = std::stoi(algHistory.getPropertyValue("LoaderVersion"));
      algFresh = AlgorithmManager::Instance().createUnmanaged(algHistory.getPropertyValue("LoaderName"), version);
    } else {
      algFresh = AlgorithmManager::Instance().createUnmanaged(name, algHistory.version());
    }
    algFresh->initialize();

    const auto &propsFresh = algFresh->getProperties();
    // just get the names out of the fresh alg properties
    std::set<std::string> freshPropNames;
    for (const auto &propFresh : propsFresh) {
      freshPropNames.insert(propFresh->name());
    }

    // remove output properties that are not present on a fresh algorithm
    // i.e. remove dynamically added properties
    for (auto prop_iter = props.begin(); prop_iter != props.end();) {
      if (freshPropNames.find((*prop_iter)->name()) == freshPropNames.end() &&
          (*prop_iter)->direction() == Kernel::Direction::Output) {
        prop_iter = props.erase(prop_iter);
      } else {
        ++prop_iter;
      }
    }

  } catch (std::exception &) {
    g_log.error() << "Could not create a fresh version of " << name << " version " << algHistory.version() << "\n";
  }

  const bool storeInADS = algHistory.getStoreInADS();

  for (auto &propIter : props) {
    if (!storeInADS && propIter->name() == "OutputWorkspace") {
      continue;
    }
    std::string prop = buildPropertyString(*propIter, name);
    if (prop.length() > 0) {
      properties << prop << ", ";
    }
  }

  // Three cases, we can either specify the version of every algorithm...
  if (m_versionSpecificity == "all") {
    properties << "Version=" << algHistory.version() << ", ";
  } else if (m_versionSpecificity == "old") {
    //...or only specify algorithm versions when they're not the newest version
    const auto &algName = algHistory.name();
    auto &algFactory = API::AlgorithmFactory::Instance();
    int latestVersion = 0;

    if (algFactory.exists(algName)) { // Check the alg still exists in Mantid
      latestVersion = AlgorithmFactory::Instance().highestVersion(algName);
    }
    // If a newer version of this algorithm exists, then this must be an old
    // version.
    if (latestVersion > algHistory.version()) {
      properties << "Version=" << algHistory.version() << ", ";
    }
  }
  // Third case is we never specify the version, so do nothing.

  std::string assignmentStatement("");
  if (!storeInADS) {
    properties << "StoreInADS=False, ";
    const auto it =
        std::find_if(props.cbegin(), props.cend(), [](const auto &prop) { return prop->name() == "OutputWorkspace"; });
    if (it != props.cend()) {
      assignmentStatement = (*it)->value() + " = ";
    }
  }

  std::string propStr = properties.str();
  if (propStr.length() > 0) {
    // remove trailing comma & space
    propStr.erase(propStr.size() - 1);
    propStr.erase(propStr.size() - 1);
  }

  std::string historyEntry = assignmentStatement + name + "(" + propStr + ")";
  historyEntry.erase(boost::remove_if(historyEntry, boost::is_any_of("\n\r")), historyEntry.end());
  return historyEntry;
}

GNU_DIAG_OFF("maybe-uninitialized")

/**
 * Build the script output for a single property
 *
 * @param propHistory :: reference to a property history object
 * @param algName :: reference to the algorithm that the property is from's name
 * @returns std::string for this property
 */
const std::string ScriptBuilder::buildPropertyString(const Mantid::Kernel::PropertyHistory &propHistory,
                                                     const std::string &algName) {
  using Mantid::Kernel::Direction;

  // If the property is to be ignored then return with an empty string
  if (std::find_if(m_propertiesToIgnore.begin(), m_propertiesToIgnore.end(),
                   [&propHistory, algName](std::vector<std::string> &c) -> bool {
                     return algName == c[0] && propHistory.name() == c[1];
                   }) != m_propertiesToIgnore.end()) {
    return "";
  }

  std::string prop;
  // No need to specify value for default properties
  if (!propHistory.isDefault()) {
    // Create a vector of all non workspace property type names
    std::vector<std::string> nonWorkspaceTypes{"number", "boolean", "string"};
    // Do not give values to output properties other than workspace properties
    if (find(nonWorkspaceTypes.begin(), nonWorkspaceTypes.end(), propHistory.type()) != nonWorkspaceTypes.end() &&
        propHistory.direction() == Direction::Output) {
      // If algs are to be ignored (Common use case is project recovery) ignore
      if (m_algsToIgnore.size() == 0) {
        g_log.debug() << "Ignoring property " << propHistory.name() << " of type " << propHistory.type() << '\n';
      }
      // Handle numerical properties
    } else if (propHistory.type() == "number") {
      prop = propHistory.name() + "=" + propHistory.value();
      // Handle boolean properties
    } else if (propHistory.type() == "boolean") {
      std::string value = (propHistory.value() == "1" ? "True" : "False");
      prop = propHistory.name() + "=" + value;
      // Handle all other property types
    } else {
      std::string opener = "='";
      std::string closer = "'";
      if (propHistory.value().find('\\') != std::string::npos) {
        opener = "=r'";
      } else if (propHistory.pythonVariable()) {
        opener = "=";
        closer = "";
      }

      prop = propHistory.name() + opener + propHistory.value() + closer;
    }
  }

  return prop;
}

GNU_DIAG_ON("maybe-uninitialized")

} // namespace Mantid::API
