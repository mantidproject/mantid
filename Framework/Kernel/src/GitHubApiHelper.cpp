#include "MantidKernel/GitHubApiHelper.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/Logger.h"
#include <Poco/Net/HTTPResponse.h>

namespace Mantid {
namespace Kernel {

using namespace Poco::Net;
using std::map;
using std::string;

namespace {
// anonymous namespace for some utility functions
/// static Logger object
Logger g_log("InternetHelper");
}

//----------------------------------------------------------------------------------------------
/** Constructor
*/
GitHubApiHelper::GitHubApiHelper()
  : InternetHelper()
{
  addAuthenticationToken();
}


//----------------------------------------------------------------------------------------------
/** Constructor
*/
GitHubApiHelper::GitHubApiHelper(const Kernel::ProxyInfo &proxy)
  : InternetHelper(proxy) {
  addAuthenticationToken();
}

//----------------------------------------------------------------------------------------------
/** Destructor
*/
GitHubApiHelper::~GitHubApiHelper() {
}

void GitHubApiHelper::reset() {
  InternetHelper::reset();
  addAuthenticationToken();
}

void GitHubApiHelper::processResponseHeaders(const Poco::Net::HTTPResponse &res) {
  // get github api rate limit information if available;
  int rateLimitRemaining;
  int rateLimitLimit;
  DateAndTime rateLimitReset;
  try {
    rateLimitLimit =
      boost::lexical_cast<int>(res.get("X-RateLimit-Limit", "-1"));
    rateLimitRemaining =
      boost::lexical_cast<int>(res.get("X-RateLimit-Remaining", "-1"));
    rateLimitReset.set_from_time_t(
      boost::lexical_cast<int>(res.get("X-RateLimit-Reset", "0")));
  } catch (boost::bad_lexical_cast const &) {
    rateLimitLimit = -1;
  }
  if (rateLimitLimit > -1) {
    g_log.debug() << "GitHub API " << rateLimitRemaining << " of " << rateLimitLimit <<
      " calls left. Resets at " << rateLimitReset.toSimpleString() << std::endl;
  }
}
} // namespace Kernel
} // namespace Mantid
