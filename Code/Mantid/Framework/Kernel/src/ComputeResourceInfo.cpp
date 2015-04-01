#include "MantidKernel/ComputeResourceInfo.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/Logger.h"

#include <Poco/DOM/AutoPtr.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/Text.h>

namespace Mantid {
namespace Kernel {
namespace {
// static logger object
Logger g_log("ComputeResourceInfo");
}

/**
 * Construct a compute resource from information found in a facilities
 * definition file.
 *
 * @param fac Facility where this (remote) compute resource is available
 * @param elem A (Poco::XML::) Element to read the data from
 *
 * @throw std::runtime_error if name or required attributes are not
 * found
 */
ComputeResourceInfo::ComputeResourceInfo(const FacilityInfo *fac,
                                         const Poco::XML::Element *elem)
    : m_facility(fac) {

  m_name = elem->getAttribute("name");
  if (m_name.empty()) {
    std::string elemStr = "";
    if (elem)
      elemStr = elem->innerText();
    throw std::runtime_error(
        "The compute resource name is not defined, at element: " + elemStr);
  }

  // default: Mantid web service API:
  // http://www.mantidproject.org/Remote_Job_Submission_API
  m_managerType = "MantidWebServiceAPIJobManager";
  std::string type = elem->getAttribute("JobManagerType");
  if (!type.empty()) {
    m_managerType = type;
  }

  const std::string baseTag = "baseURL";
  Poco::AutoPtr<Poco::XML::NodeList> nl = elem->getElementsByTagName(baseTag);
  if (!nl || nl->length() != 1 || !nl->item(0) ||
      !nl->item(0)->hasChildNodes()) {
    g_log.error("Failed to get base URL for remote compute resource '" +
                m_name + "'");
    throw std::runtime_error("Remote compute resources must have exactly one "
                             "baseURL tag. It was not found for the resource "
                             "'" +
                             m_name + "'");
  } else {
    nl = nl->item(0)->childNodes();
    if (nl->length() > 0) {
      Poco::XML::Text *txt = dynamic_cast<Poco::XML::Text *>(nl->item(0));
      if (txt) {
        m_baseURL = txt->getData();
      } else {
        g_log.error("Failed to get base URL for remote compute resource '" +
                    m_name + "'. The " + baseTag + " tag seems empty!");
        throw std::runtime_error(
            "Remote compute resources must have exactly one "
            "baseURL tag containing a URL string. A tag was found for the "
            "resource "
            "'" +
            m_name + "', but it seems empty!");
      }
    }
  }
}

/**
* Equality operator. Two different resources cannot have the same name
*
* @param rhs object to compare this with
*
* @return True if the objects (names) are equal
*/
bool ComputeResourceInfo::operator==(const ComputeResourceInfo &rhs) const {
  return (this->name() == rhs.name());
}

std::string ComputeResourceInfo::name() const { return m_name; }

std::string ComputeResourceInfo::baseURL() const { return m_baseURL; }

std::string ComputeResourceInfo::remoteJobManagerType() const {
  return m_managerType;
}

const FacilityInfo &ComputeResourceInfo::facility() const {
  return *m_facility;
}

/**
 * Prints the instrument name into an output stream
 *
 * @param buffer an output stream being written to
 * @param cr a ComputeResourceInfo object to print
 *
 * @return reference to the output stream being written to
 */
std::ostream &operator<<(std::ostream &buffer, const ComputeResourceInfo &cr) {
  buffer << "'" + cr.name() + "', at '" + cr.baseURL() + "', of type '" +
                cr.remoteJobManagerType() + "'";
  return buffer;
}

} // namespace Kernel
} // namespace Mantid
