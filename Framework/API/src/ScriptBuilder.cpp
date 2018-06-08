//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AlgorithmHistory.h"
#include "MantidKernel/PropertyHistory.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/HistoryItem.h"
#include "MantidAPI/ScriptBuilder.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/Logger.h"

#include <boost/utility.hpp>
#include <set>

namespace Mantid {
namespace API {

using Mantid::Kernel::PropertyHistory_sptr;
using Mantid::Kernel::PropertyHistory_const_sptr;

namespace {
Mantid::Kernel::Logger g_log("ScriptBuilder");
}

const std::string COMMENT_ALG = "Comment";

ScriptBuilder::ScriptBuilder(boost::shared_ptr<HistoryView> view,
                             std::string versionSpecificity)
    : m_historyItems(view->getAlgorithmsList()), m_output(),
      m_versionSpecificity(versionSpecificity) {}

/**
 * Build a python script for each algorithm included in the history view.
 *
 * @return a formatted python string of the history
 */
const std::string ScriptBuilder::build() {
  std::ostringstream os;
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
void ScriptBuilder::writeHistoryToStream(
    std::ostringstream &os, std::vector<HistoryItem>::const_iterator &iter,
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

    if (boost::next(iter) == m_historyItems.end() ||
        !boost::next(iter)->isUnrolled()) {
      os << "\n";
    }
  } else {
    // create the string for this algorithm
    os << buildAlgorithmString(algHistory) << "\n";
  }
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
void ScriptBuilder::buildChildren(
    std::ostringstream &os, std::vector<HistoryItem>::const_iterator &iter,
    int depth) {
  size_t numChildren = iter->numberOfChildren();
  ++iter; // move to first child
  for (size_t i = 0; i < numChildren && iter != m_historyItems.end();
       ++i, ++iter) {
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
const std::string
ScriptBuilder::buildCommentString(AlgorithmHistory_const_sptr algHistory) {
  std::ostringstream comment;
  const std::string name = algHistory->name();
  if (name == COMMENT_ALG) {
    auto props = algHistory->getProperties();
    for (auto &prop : props) {
      if (prop->name() == "Note") {
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
const std::string
ScriptBuilder::buildAlgorithmString(AlgorithmHistory_const_sptr algHistory) {
  std::ostringstream properties;
  const std::string name = algHistory->name();
  std::string prop;

  if (name == COMMENT_ALG)
    return buildCommentString(algHistory);

  auto props = algHistory->getProperties();

  try {
    // create a fresh version of the algorithm - unmanaged
    IAlgorithm_sptr algFresh = AlgorithmManager::Instance().createUnmanaged(
        name, algHistory->version());
    algFresh->initialize();

    const auto &propsFresh = algFresh->getProperties();
    // just get the names out of the fresh alg properties
    std::set<std::string> freshPropNames;
    for (const auto &propFresh : propsFresh) {
      freshPropNames.insert(propFresh->name());
    }

    // remove properties that are not present on a fresh algorithm
    // i.e. remove dynamically added properties
    for (auto prop_iter = props.begin(); prop_iter != props.end();) {
      if (freshPropNames.find((*prop_iter)->name()) == freshPropNames.end()) {
        prop_iter = props.erase(prop_iter);
      } else {
        ++prop_iter;
      }
    }

  } catch (std::exception &) {
    g_log.error() << "Could not create a fresh version of " << name
                  << " version " << algHistory->version() << "\n";
  }

  for (auto &propIter : props) {
    prop = buildPropertyString(propIter);
    if (prop.length() > 0) {
      properties << prop << ", ";
    }
  }

  // Three cases, we can either specify the version of every algorithm...
  if (m_versionSpecificity == "all") {
    properties << "Version=" << algHistory->version() << ", ";
  } else if (m_versionSpecificity == "old") {
    //...or only specify algorithm versions when they're not the newest version
    bool oldVersion = false;

    std::vector<AlgorithmDescriptor> descriptors =
        AlgorithmFactory::Instance().getDescriptors();
    for (auto &descriptor : descriptors) {
      // If a newer version of this algorithm exists, then this must be an old
      // version.
      if (descriptor.name == algHistory->name() &&
          descriptor.version > algHistory->version()) {
        oldVersion = true;
        break;
      }
    }

    if (oldVersion) {
      properties << "Version=" << algHistory->version() << ", ";
    }
  }
  // Third case is we never specify the version, so do nothing.

  std::string propStr = properties.str();
  if (propStr.length() > 0) {
    // remove trailing comma & space
    propStr.erase(propStr.size() - 1);
    propStr.erase(propStr.size() - 1);
  }

  return name + "(" + propStr + ") # " +
         algHistory->executionDate().toISO8601String();
}

/**
 * Build the script output for a single property
 *
 * @param propHistory :: reference to a property history object
 * @returns std::string for this property
 */
const std::string
ScriptBuilder::buildPropertyString(PropertyHistory_const_sptr propHistory) {
  using Mantid::Kernel::Direction;

  // Create a vector of all non workspace property type names
  std::vector<std::string> nonWorkspaceTypes{"number", "boolean", "string"};

  std::string prop;
  // No need to specify value for default properties
  if (!propHistory->isDefault()) {
    // Do not give values to output properties other than workspace properties
    if (find(nonWorkspaceTypes.begin(), nonWorkspaceTypes.end(),
             propHistory->type()) != nonWorkspaceTypes.end() &&
        propHistory->direction() == Direction::Output) {
      g_log.debug() << "Ignoring property " << propHistory->name()
                    << " of type " << propHistory->type() << '\n';
      // Handle numerical properties
    } else if (propHistory->type() == "number") {
      prop = propHistory->name() + "=" + propHistory->value();
      // Handle boolean properties
    } else if (propHistory->type() == "boolean") {
      std::string value = (propHistory->value() == "1" ? "True" : "False");
      prop = propHistory->name() + "=" + value;
      // Handle all other property types
    } else {
      std::string opener = "='";
      if (propHistory->value().find('\\') != std::string::npos) {
        opener = "=r'";
      }

      prop = propHistory->name() + opener + propHistory->value() + "'";
    }
  }

  return prop;
}

} // namespace API
} // namespace Mantid
