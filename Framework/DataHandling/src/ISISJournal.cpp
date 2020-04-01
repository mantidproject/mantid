// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
// Includes
#include "MantidDataHandling/ISISJournal.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/InternetHelper.h"

#include "Poco/SAX/SAXException.h"
#include <Poco/AutoPtr.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/NodeFilter.h>
#include <Poco/DOM/TreeWalker.h>
#include <Poco/Net/HTTPResponse.h>

#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <sstream>

namespace Mantid {
namespace DataHandling {
namespace ISISJournal {

using Kernel::InternetHelper;
using Poco::XML::Document;
using Poco::XML::DOMParser;
using Poco::XML::Element;
using Poco::XML::Node;
using Poco::XML::NodeFilter;
using Poco::XML::TreeWalker;

namespace {
static constexpr const char *URL_PREFIX = "http://data.isis.rl.ac.uk/";
static constexpr const char *INSTRUMENT_PREFIX = "journals/ndx";
static constexpr const char *JOURNAL_INDEX_FILE = "main";
static constexpr const char *JOURNAL_PREFIX = "journal_";
static constexpr const char *JOURNAL_EXT = ".xml";
static constexpr const char *NXROOT_TAG = "NXroot";
static constexpr const char *NXENTRY_TAG = "NXentry";
static constexpr const char *JOURNAL_TAG = "journal";
static constexpr const char *FILE_TAG = "file";

/* Construct the URL for a journal file containing run data for a particular
 * instrument and cycle, e.g.
 *   http://data.isis.rl.ac.uk/journals/ndxinter/journal_19_4.xml
 */
std::string constructRunsFileURL(std::string const &instrument,
                                 std::string const &cycle) {
  std::stringstream url;
  url << URL_PREFIX << INSTRUMENT_PREFIX << instrument << "/" << JOURNAL_PREFIX
      << cycle << JOURNAL_EXT;
  return url.str();
}

/* Construct the URL for the journal index file for a particular instrument,
 * e.g. http://data.isis.rl.ac.uk/journals/ndxinter/journal_main.xml
 */
std::string constructIndexFileURL(std::string const &instrument) {
  std::stringstream url;
  url << URL_PREFIX << INSTRUMENT_PREFIX << instrument << "/" << JOURNAL_PREFIX
      << JOURNAL_INDEX_FILE << JOURNAL_EXT;
  return url.str();
}

/* Get the contents of a file at a given URL
 */
std::string getURLContents(std::string const &url) {
  Kernel::InternetHelper inetHelper;
  std::stringstream serverReply;
  auto const statusCode = inetHelper.sendRequest(url, serverReply);
  if (statusCode != Poco::Net::HTTPResponse::HTTP_OK) {
    throw Kernel::Exception::InternetError(
        std::string("Failed to access journal file: ") +
        std::to_string(statusCode));
  }
  return serverReply.str();
}

/* Parse a given XML file contents into a Document object
 */
Poco::AutoPtr<Document> parse(std::string const &fileContents) {
  DOMParser domParser;
  Poco::AutoPtr<Document> xmldoc;
  try {
    xmldoc = domParser.parseString(fileContents);
  } catch (Poco::XML::SAXParseException const &ex) {
    std::ostringstream msg;
    msg << "ISISJournal: Error parsing file: " << ex.what();
    throw std::runtime_error(msg.str());
  }
  return xmldoc;
}

/* Check the given element is not null and has the given name and throw if not
 */
void validateElement(Element *element, const char *expectedName) {
  if (!element) {
    std::ostringstream msg;
    msg << "ISISJournal::validateElement() - invalid element for '"
        << NXROOT_TAG << "'\n";
    throw std::invalid_argument(msg.str());
  }

  if (element->nodeName() != expectedName) {
    std::ostringstream msg;
    msg << "ISISJournal::validateElement() - Element tag does not match '"
        << NXROOT_TAG << "'. Found " << element->nodeName() << "\n";
    throw std::invalid_argument(msg.str());
  }
}

/* Return the text value contained in the given node, or an empty string if it
 * does not contain a text value.
 */
std::string getTextValue(Node *node) {
  if (!node)
    return std::string();

  auto child = node->firstChild();
  if (child && child->nodeName() == "#text") {
    auto value = child->nodeValue();
    boost::algorithm::trim(value);
    return value;
  }

  return std::string();
}

/* Check if an element matches a set of filter criteria. Checks for a child
 * element with the filter name and checks that its value matches the given
 * filter value.
 */
bool matchesAllFilters(Element *element, ISISJournalFilters const &filters) {
  for (auto filterKvp : filters) {
    auto const childElement = element->getChildElement(filterKvp.first);
    if (getTextValue(childElement) != filterKvp.second)
      return false;
  }
  return true;
}

/* Extract a list of named "tag" values for the given element. Gets the text
 * values of the child elements with the given names and returns a map of the
 * tag name to the value. Also always adds the element name to the list so there
 * is always a results for every run.
 */
ISISJournalData getTagsForNode(Element *element, ISISJournalTags const &tags) {
  auto result = ISISJournalData{};
  // Add the run name
  result["name"] = element->getAttribute("name");
  // Add any tags in the tags list
  for (auto &tag : tags)
    result[tag] = getTextValue(element->getChildElement(tag));
  return result;
}

/* Extract a list of "tag" values for all child nodes in the given "root" or
 * parent element. Here, a "tag" is a name for a child element in the element
 * we're checking i.e. a grandchild of the root node.
 */
std::vector<ISISJournalData>
getTagsForAllNodes(Element *root, ISISJournalTags const &tags,
                   ISISJournalFilters const &filters) {
  auto results = std::vector<ISISJournalData>{};

  auto nodeIter = TreeWalker(root, NodeFilter::SHOW_ELEMENT);
  for (auto node = nodeIter.nextNode(); node; node = nodeIter.nextSibling()) {
    auto element = dynamic_cast<Element *>(node);
    validateElement(element, NXENTRY_TAG);
    if (matchesAllFilters(element, filters))
      results.emplace_back(getTagsForNode(element, tags));
  }

  return results;
}

/* Get the text values for all direct child elements of a given parent
 * element. The child elements should all have the given tag name - throws if
 * not.
 */
std::vector<std::string> getTagValuesForChildElements(Element *root,
                                                      const char *tag) {
  auto results = std::vector<std::string>{};

  auto nodeIter = TreeWalker(root, NodeFilter::SHOW_ELEMENT);
  for (auto node = nodeIter.nextNode(); node; node = nodeIter.nextSibling()) {
    auto element = dynamic_cast<Element *>(node);
    validateElement(element, tag);
    results.emplace_back(element->getAttribute("name"));
  }

  return results;
}

/* Convert a cycle filename to the cycle name or return an empty string if it
 * doesn't match the required pattern
 */
std::string convertFilenameToCycleName(std::string const &filename) {
  boost::regex pattern("[0-9]+_[0-9]+");
  boost::smatch matches;
  boost::regex_search(filename, matches, pattern);

  if (matches.size() == 1)
    return matches[0];

  return std::string();
}

/* Convert a list of cycle filenames to a list of cycle names e.g.
 * journal_19_4.xml -> 19_4. Also exlcudes files from the list if they do not
 * match the required pattern.
 */
std::vector<std::string>
convertFilenamesToCycleNames(std::vector<std::string> const &filenames) {
  auto cycles = std::vector<std::string>();
  cycles.reserve(filenames.size());
  for (const auto &filename : filenames) {
    auto cycle = convertFilenameToCycleName(filename);
    if (!cycle.empty())
      cycles.emplace_back(std::move(cycle));
  }
  return cycles;
}
} // namespace

/** Get specified data from a journal file for all runs that match given filter
 * criteria.
 *
 * @param fileContents : the XML journal file contents
 * @param tags : the tag names of the required values to be returned e.g.
 * "run_number"
 * @param filters : optional tag names and values to filter the results by
 */
std::vector<ISISJournalData>
getRunDataFromFile(std::string const &fileContents, ISISJournalTags const &tags,
                   ISISJournalFilters const &filters) {
  auto xmldoc = parse(fileContents);
  auto root = xmldoc->documentElement();
  validateElement(root, NXROOT_TAG);
  return getTagsForAllNodes(root, tags, filters);
}

/** Get specified data for all runs in a specific instrument and cycle that
 * match given filter criteria.
 *
 * @param instrument : the instrument to request data for
 * @param cycle : the ISIS cycle the required data is from e.g. "19_4"
 * @param tags : the tag names of the required values to be returned e.g.
 * "run_number"
 * @param filters : optional tag names and values to filter the results by
 */
std::vector<ISISJournalData> getRunData(std::string const &instrument,
                                        std::string const &cycle,
                                        ISISJournalTags const &tags,
                                        ISISJournalFilters const &filters) {
  auto const url = constructRunsFileURL(instrument, cycle);
  auto fileContents = getURLContents(url);
  return getRunDataFromFile(fileContents, tags, filters);
}

/** Get the list of cycle names from the given index file
 * @param fileContents : the contents of the XML index file
 * @returns : a list of cycle names
 */
std::vector<std::string> getCycleListFromFile(std::string const &fileContents) {
  auto xmldoc = parse(fileContents);
  auto root = xmldoc->documentElement();
  validateElement(root, JOURNAL_TAG);
  auto filenames = getTagValuesForChildElements(root, FILE_TAG);
  return convertFilenamesToCycleNames(filenames);
}

/** Get the list of cycle names for the given instrument
 * @param instrument : the instrument name
 * @returns : a list of cycle names
 */
std::vector<std::string> getCycleList(std::string const &instrument) {
  auto const url = constructIndexFileURL(instrument);
  auto fileContents = getURLContents(url);
  return getCycleList(fileContents);
}
} // namespace ISISJournal
} // namespace DataHandling
} // namespace Mantid
