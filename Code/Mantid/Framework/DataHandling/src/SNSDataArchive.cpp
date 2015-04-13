//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <iostream>
#include <sstream>

#include "MantidKernel/Logger.h"
#include "MantidKernel/InternetHelper.h"
#include "MantidKernel/Exception.h"
#include "MantidDataHandling/SNSDataArchive.h"
#include "MantidAPI/ArchiveSearchFactory.h"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <Poco/AutoPtr.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/SAX/InputSource.h>
#include <Poco/DOM/NodeList.h>


namespace Mantid {
namespace DataHandling {
namespace {
// Get a reference to the logger
Kernel::Logger g_log("SNSDataArchive");
/// Base url for restful web survice
const std::string
    BASE_URL("http://icat.sns.gov:2080/icat-rest-ws/datafile/filename/");
}

DECLARE_ARCHIVESEARCH(SNSDataArchive, SNSDataSearch)

/**
 * @param filenames : List of files to search
 * @param exts : List of extensions to check against
 * @return list of archive locations
 */
std::string
SNSDataArchive::getArchivePath(const std::set<std::string> &filenames,
                               const std::vector<std::string> &exts) const {
  std::set<std::string>::const_iterator iter = filenames.begin();
  std::string filename = *iter;

  // ICAT4 web service take upper case filename such as HYSA_2662
  std::transform(filename.begin(), filename.end(), filename.begin(), toupper);

  std::vector<std::string>::const_iterator iter2 = exts.begin();
  for (; iter2 != exts.end(); ++iter2) {
    g_log.debug() << *iter2 << ";";
  }
  g_log.debug() << "\n";

  const std::string URL(BASE_URL + filename);
  g_log.debug() << "URL: " << URL << "\n";


  Kernel::InternetHelper inetHelper;
  std::ostringstream rs;


  int status = inetHelper.sendRequest(URL,rs);


  // Create a DOM document from the response.
  Poco::XML::DOMParser parser;
  std::istringstream istrsource(rs.str());
  Poco::XML::InputSource source(istrsource);
  Poco::AutoPtr<Poco::XML::Document> pDoc = parser.parse(&source);

  std::vector<std::string> locations;

  // Everything went fine, return the XML document.
  // Otherwise look for an error message in the XML document.
  if (status == Kernel::InternetHelper::HTTP_OK) {
    std::string location;
    Poco::AutoPtr<Poco::XML::NodeList> pList =
        pDoc->getElementsByTagName("location");
    for (unsigned long i = 0; i < pList->length(); i++) {
      location = pList->item(i)->innerText();
      g_log.debug() << "location: " << location << "\n";
      locations.push_back(location);
    }
  }

  std::vector<std::string>::const_iterator ext = exts.begin();
  for (; ext != exts.end(); ++ext) {
    std::string datafile = filename + *ext;
    std::vector<std::string>::const_iterator iter = locations.begin();
    for (; iter != locations.end(); ++iter) {
      if (boost::algorithm::ends_with((*iter), datafile)) {
        return *iter;
      } // end if
    }   // end for iter

  } // end for ext
  return "";
}

} // namespace DataHandling
} // namespace Mantid
