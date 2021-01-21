// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
// Includes
#include "MantidDataHandling/ISISJournal.h"
#include "MantidKernel/Exception.h"

#include "Poco/SAX/SAXException.h"
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

using Kernel::InternetHelper;
using Poco::XML::Document;
using Poco::XML::DOMParser;
using Poco::XML::Element;
using Poco::XML::Node;
using Poco::XML::NodeFilter;
using Poco::XML::TreeWalker;

namespace {
static constexpr const char *URL_PREFIX = "http://data.isis.rl.ac.uk/journals/ndx";
static constexpr const char *INDEX_FILE_NAME = "main";
static constexpr const char *JOURNAL_PREFIX = "/journal_";
static constexpr const char *JOURNAL_EXT = ".xml";
static constexpr const char *NXROOT_TAG = "NXroot";
static constexpr const char *NXENTRY_TAG = "NXentry";
static constexpr const char *JOURNAL_TAG = "journal";
static constexpr const char *FILE_TAG = "file";

/** Construct the URL for a journal file containing run data for a particular
 * instrument and cycle, e.g.
 *   http://data.isis.rl.ac.uk/journals/ndxinter/journal_19_4.xml
 * @param instrument : the ISIS instrument name (case insensitive)
 * @param name : the file name excluding the "journal_" prefix and extension,
 * e.g. "19_4" for "journal_19_4.xml"
 * @returns : the URL as a string
 */
std::string constructURL(std::string instrument, std::string const &name) {
  boost::algorithm::to_lower(instrument);
  std::ostringstream url;
  url << URL_PREFIX << instrument << JOURNAL_PREFIX << name << JOURNAL_EXT;
  return url.str();
}

/** Parse a given XML string into a Document object
 * @param xmlString : the XML file contents as a string
 * @returns : the XML Document
 * @throws : if there was an error parsing the file
 */
Poco::AutoPtr<Document> parse(std::string const &xmlString) {
  DOMParser domParser;
  Poco::AutoPtr<Document> xmldoc;
  try {
    xmldoc = domParser.parseString(xmlString);
  } catch (Poco::XML::SAXParseException const &ex) {
    std::ostringstream msg;
    msg << "ISISJournal: Error parsing file: " << ex.what();
    throw std::runtime_error(msg.str());
  }
  return xmldoc;
}

/** Check the given element is valid.
 * @param expectedName : the node name that the element is expected to have
 * @throws : if the element is null or does not match the expected name
 */
void throwIfNotValid(Element *element, const char *expectedName) {
  if (!element) {
    std::ostringstream msg;
    msg << "ISISJournal::throwIfNotValid() - invalid element for '" << expectedName << "'\n";
    throw std::invalid_argument(msg.str());
  }

  if (element->nodeName() != expectedName) {
    std::ostringstream msg;
    msg << "ISISJournal::throwIfNotValid() - Element name does not match '" << expectedName << "'. Found "
        << element->nodeName() << "\n";
    throw std::invalid_argument(msg.str());
  }
}

/** Get the text contained in a node.
 * @param node : the node
 * @returns : the value contained in the child #text node, or an empty string if
 * it does not contain a text value.
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

/** Check if an element matches a set of filter criteria.
 * @param element : the element to check
 * @param filters : a map of names and values to check against
 * @returns : true if, for all filters, the element has a child node with
 * the filter name that contains text matching the filter value
 */
bool matchesAllFilters(Element *element, ISISJournal::RunData const &filters) {
  for (auto const &filterKvp : filters) {
    auto const childElement = element->getChildElement(filterKvp.first);
    if (getTextValue(childElement) != filterKvp.second)
      return false;
  }
  return true;
}

/** Utility function to create the run data for an element.
 * @param element : the element to get run data for
 * @returns : a map of name->value pairs initialised with the mandatory data
 * that is returned for all runs (currently just the name attribute, e.g.
 * "name": "INTER00013460")
 */
ISISJournal::RunData createRunDataForElement(Element *element) {
  return ISISJournal::RunData{{"name", element->getAttribute("name")}};
}

/** Extract a list of named values for the given element.
 * @param element : the element to extract values for
 * @param valuesToLookup : a list of values to extract (which may be empty if
 * nothing is requested)
 * @param result : a map of names to values to which we add key-value pairs for
 * each requested value
 */
void addValuesForElement(Element *element, std::vector<std::string> const &valuesToLookup,
                         ISISJournal::RunData &result) {
  for (auto &name : valuesToLookup)
    result[name] = getTextValue(element->getChildElement(name));
}

/** Extract a list of named values for all child nodes in the given parent
 * element.
 * @param parentElement : the element containing all child nodes we want to
 * check
 * @param valuesToLookup : the names of the values to extract from within the
 * child nodes
 * @param filters : a map of names and values to filter the results by
 * @returns : a map of name->value pairs containing mandatory run data as well
 * as the values that were requested
 */
std::vector<ISISJournal::RunData> getValuesForAllElements(Element *parentElement,
                                                          std::vector<std::string> const &valuesToLookup,
                                                          ISISJournal::RunData const &filters) {
  auto results = std::vector<ISISJournal::RunData>{};

  auto nodeIter = TreeWalker(parentElement, NodeFilter::SHOW_ELEMENT);
  for (auto node = nodeIter.nextNode(); node; node = nodeIter.nextSibling()) {
    auto element = dynamic_cast<Element *>(node);
    throwIfNotValid(element, NXENTRY_TAG);

    if (matchesAllFilters(element, filters)) {
      auto result = createRunDataForElement(element);
      addValuesForElement(element, valuesToLookup, result);
      results.emplace_back(std::move(result));
    }
  }

  return results;
}

/** Extract an attribute value for all direct children of a given element.
 * @param parentElement : the element containing the child nodes to check
 * @param childElementName : the name that the child elements should have
 * @returns : the attribute values for all of the child elements
 * @throws : if any of the child elements does not have the expected name
 */
std::vector<std::string> getAttributeForAllChildElements(Element *parentElement, const char *childElementName,
                                                         const char *attributeName) {
  auto results = std::vector<std::string>{};

  auto nodeIter = TreeWalker(parentElement, NodeFilter::SHOW_ELEMENT);
  for (auto node = nodeIter.nextNode(); node; node = nodeIter.nextSibling()) {
    auto element = dynamic_cast<Element *>(node);
    throwIfNotValid(element, childElementName);
    results.emplace_back(element->getAttribute(attributeName));
  }

  return results;
}

/** Convert a cycle filename to the cycle name
 * @param filename : a filename e.g. journal_19_4.xml
 * @return the cycle name e.g. 19_4, or an empty string if the name
 * does not match the required pattern
 */
std::string convertFilenameToCycleName(std::string const &filename) {
  boost::regex pattern("[0-9]+_[0-9]+");
  boost::smatch matches;
  boost::regex_search(filename, matches, pattern);

  if (matches.size() == 1)
    return matches[0];

  return std::string();
}

/** Convert a list of cycle filenames to a list of cycle names.
 * @param filenames : the list of filenames e.g. journal_main.xml,
 * journal_19_4.xml
 * @returns : the list of cycle names for all valid journal files e.g. 19_4
 * (note that this excludes files that do not match the journal-file pattern).
 */
std::vector<std::string> convertFilenamesToCycleNames(std::vector<std::string> const &filenames) {
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

/** Construct the journal class for a specific instrument and cycle
 * @param instrument : the ISIS instrument name to request data for e.g. "INTER"
 * (case insensitive)
 * @param cycle : the ISIS cycle the required data is from e.g. "19_4"
 * @param internetHelper : class for sending internet requests
 */
ISISJournal::ISISJournal(std::string const &instrument, std::string const &cycle,
                         std::unique_ptr<InternetHelper> internetHelper)
    : m_internetHelper(std::move(internetHelper)), m_runsFileURL(constructURL(instrument, cycle)),
      m_indexFileURL(constructURL(instrument, INDEX_FILE_NAME)) {}

ISISJournal::~ISISJournal() = default;

ISISJournal::ISISJournal(ISISJournal &&rhs) = default;

ISISJournal &ISISJournal::operator=(ISISJournal &&rhs) = default;

/** Get the cycle names
 *
 * @returns : a list of all ISIS cycle names for the instrument
 * @throws : if there was an error fetching the runs
 */
std::vector<std::string> ISISJournal::getCycleNames() {
  if (!m_indexDocument) {
    auto xmlString = getURLContents(m_indexFileURL);
    m_indexDocument = parse(xmlString);
  }
  auto rootElement = m_indexDocument->documentElement();
  throwIfNotValid(rootElement, JOURNAL_TAG);
  auto filenames = getAttributeForAllChildElements(rootElement, FILE_TAG, "name");
  return convertFilenamesToCycleNames(filenames);
}

/** Get run names and other specified data for all runs that match the given
 * filter criteria.
 *
 * @param valuesToLookup : optional list of additional values to be returned
 * e.g. "run_number", "title"
 * @param filters : optional element names and values to filter the results by
 * @throws : if there was an error fetching the runs
 */
std::vector<ISISJournal::RunData> ISISJournal::getRuns(std::vector<std::string> const &valuesToLookup,
                                                       ISISJournal::RunData const &filters) {
  if (!m_runsDocument) {
    auto xmlString = getURLContents(m_runsFileURL);
    m_runsDocument = parse(xmlString);
  }
  auto rootElement = m_runsDocument->documentElement();
  throwIfNotValid(rootElement, NXROOT_TAG);
  return getValuesForAllElements(rootElement, valuesToLookup, filters);
}

/** Get the contents of a file at a given URL
 * @param url : the URL to fetch
 * @returns : the contents of the file as a string
 * @throws : if there was an error fetching the file
 */
std::string ISISJournal::getURLContents(std::string const &url) {
  std::ostringstream serverReply;
  int statusCode;
  try {
    statusCode = m_internetHelper->sendRequest(url, serverReply);
  } catch (Kernel::Exception::InternetError const &) {
    std::ostringstream msg;
    msg << "Failed to access file " << url << "\nCheck that the cycle name is valid.";
    throw Kernel::Exception::InternetError(msg.str());
  }

  if (statusCode != Poco::Net::HTTPResponse::HTTP_OK) {
    std::ostringstream msg;
    msg << "Failed to access file " << url << "\nHTTP Code: " << statusCode << "\nCheck that the cycle name is valid.";
    throw Kernel::Exception::InternetError(msg.str());
  }
  return serverReply.str();
}
} // namespace DataHandling
} // namespace Mantid
