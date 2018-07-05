//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/HistoryItem.h"
#include "MantidAPI/NotebookBuilder.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/Logger.h"

#include <boost/utility.hpp>

namespace Mantid {
namespace API {

using Mantid::Kernel::PropertyHistory_sptr;
using Mantid::Kernel::PropertyHistory_const_sptr;

namespace {
Mantid::Kernel::Logger g_log("NotebookBuilder");
}

NotebookBuilder::NotebookBuilder(boost::shared_ptr<HistoryView> view,
                                 std::string versionSpecificity)
    : m_historyItems(view->getAlgorithmsList()), m_output(),
      m_versionSpecificity(versionSpecificity),
      m_nb_writer(new NotebookWriter()) {}

/**
 * Build an ipython notebook for algorithms included in the history view.
 *
 * @param ws_name :: workspace name
 * @param ws_title :: workspace title
 * @param ws_comment :: workspace comment
 * @return a formatted ipython notebook string of the history
 */
const std::string NotebookBuilder::build(std::string ws_name,
                                         std::string ws_title,
                                         std::string ws_comment) {
  // record workspace details in notebook
  std::string workspace_details;
  workspace_details = "Workspace History: " + ws_name + "\n";
  workspace_details += "------------------------\n";
  workspace_details += ws_title + "\n";
  workspace_details += ws_comment;
  m_nb_writer->markdownCell(workspace_details);

  auto iter = m_historyItems.begin();
  for (; iter != m_historyItems.end(); ++iter) {
    writeHistoryToStream(iter);
  }
  return m_nb_writer->writeNotebook();
}

/**
 * Write out an algorithm to the notebook.
 * If the entry is unrolled this will recurse and output the children of
 * that entry instead. If not, it will just output the algorithm to the stream.
 *
 * @param iter :: reference to the iterator pointing to the vector of history
 *items
 */
void NotebookBuilder::writeHistoryToStream(
    std::vector<HistoryItem>::const_iterator &iter) {
  auto algHistory = iter->getAlgorithmHistory();
  if (iter->isUnrolled()) {

    m_nb_writer->markdownCell(std::string("Child algorithms of ") +
                              algHistory->name());

    buildChildren(iter);

    m_nb_writer->markdownCell(std::string("End of child algorithms of ") +
                              algHistory->name());

  } else {
    // create the string for this algorithm
    m_nb_writer->codeCell(buildAlgorithmString(algHistory));
  }
}

/**
 * Iterate over each of the items children and output them to the script.
 * This moves the iterator forward over each of the child records and writes
 * them to the stream.
 *
 * @param iter :: reference to the iterator pointing to the vector of history
 *items
 */
void NotebookBuilder::buildChildren(
    std::vector<HistoryItem>::const_iterator &iter) {
  size_t numChildren = iter->numberOfChildren();
  ++iter; // move to first child
  for (size_t i = 0; i < numChildren && iter != m_historyItems.end();
       ++i, ++iter) {
    writeHistoryToStream(iter);
  }
  --iter;
}

/**
 * Build the script output for a single algorithm
 *
 * @param algHistory :: pointer to an algorithm history object
 * @returns std::string to run this algorithm
 */
const std::string
NotebookBuilder::buildAlgorithmString(AlgorithmHistory_const_sptr algHistory) {
  std::ostringstream properties;
  const std::string name = algHistory->name();
  std::string prop;

  auto props = algHistory->getProperties();
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
	const auto &algName = algHistory->name();

	auto descriptor = AlgorithmFactory::Instance().getDescriptor(algName);

	// If a newer version of this algorithm exists, then this must be an old
	// version.
	if (descriptor.name == algHistory->name() &&
		descriptor.version > algHistory->version()) {
		oldVersion = true;
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

  return name + "(" + propStr + ")";
}

/**
 * Build the script output for a single property
 *
 * @param propHistory :: reference to a property history object
 * @returns std::string for this property
 */
const std::string
NotebookBuilder::buildPropertyString(PropertyHistory_const_sptr propHistory) {
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
