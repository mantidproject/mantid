#ifndef MANTID_CATALOG_OAUTHTEST_H_
#define MANTID_CATALOG_OAUTHTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCatalog/Exception.h"
#include "MantidCatalog/OAuth.h"
#include "MantidKernel/DateAndTime.h"

using Mantid::Catalog::OAuth::OAuthToken;
using Mantid::Catalog::OAuth::IOAuthTokenStore;
using Mantid::Catalog::OAuth::IOAuthTokenStore_uptr;
using Mantid::Types::Core::DateAndTime;

class OAuthTest : public CxxTest::TestSuite {
public:
  // CxxTest boilerplate.
  static OAuthTest *createSuite() { return new OAuthTest(); }
  static void destroySuite(OAuthTest *suite) { delete suite; }

  void test_oauth_token_from_json_stream() {
    std::stringstream tokenStringSteam;
    tokenStringSteam << std::string(
        "{\"token_type\": \"Bearer\", \"expires_in\": 3600, "
        "\"access_token\": \"2KSL5aEnLvIudMHIjc7LcBWBCfxOHZ\", "
        "\"scope\": \"api:read data:read settings:read\", "
        "\"refresh_token\": \"eZEiz7LbgFrkL5ZHv7R4ck9gOzXexb\"}");
    const auto oauthToken = OAuthToken::fromJSONStream(tokenStringSteam);
    TS_ASSERT_EQUALS(oauthToken.tokenType(), std::string("Bearer"));
    TS_ASSERT_EQUALS(oauthToken.expiresIn(), 3600);
    TS_ASSERT_EQUALS(oauthToken.accessToken(),
                     std::string("2KSL5aEnLvIudMHIjc7LcBWBCfxOHZ"));
    TS_ASSERT_EQUALS(oauthToken.scope(),
                     std::string("api:read data:read settings:read"));
    TS_ASSERT_EQUALS(oauthToken.refreshToken(),
                     std::string("eZEiz7LbgFrkL5ZHv7R4ck9gOzXexb"));

    TS_ASSERT(!oauthToken.isExpired());
    TS_ASSERT(oauthToken.isExpired(DateAndTime::getCurrentTime() + 3601.0));
  }
};

#endif /* MANTID_CATALOG_OAUTHTEST_H_ */
