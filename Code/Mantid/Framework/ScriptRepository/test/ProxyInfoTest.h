#ifndef MANTID_SCRIPTREPOSITORY_PROXYINFOTEST_H_
#define MANTID_SCRIPTREPOSITORY_PROXYINFOTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidScriptRepository/ProxyInfo.h"

using Mantid::ScriptRepository::ProxyInfo;

class ProxyInfoTest: public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ProxyInfoTest *createSuite()
  {
    return new ProxyInfoTest();
  }
  static void destroySuite(ProxyInfoTest *suite)
  {
    delete suite;
  }

  void test_construction_no_proxy()
  {
    ProxyInfo proxyInfo;
    TSM_ASSERT("Is not a valid proxy object", proxyInfo.emptyProxy());
    TS_ASSERT_THROWS(proxyInfo.host(), std::logic_error&);
    TS_ASSERT_THROWS(proxyInfo.port(), std::logic_error&);
    TSM_ASSERT("Cannot be a http proxy if not a proxy at all.", !proxyInfo.isHttpProxy());
  }

  void test_construction_proxy()
  {
    const std::string url = "some_url";
    const int port = 1;
    const bool isHttpProxy = true;
    ProxyInfo proxyInfo(url, port, isHttpProxy);
    TSM_ASSERT("This is a valid proxy object", !proxyInfo.emptyProxy());
    TS_ASSERT_EQUALS(url, proxyInfo.host());
    TS_ASSERT_EQUALS(port, proxyInfo.port());
    TS_ASSERT_EQUALS(isHttpProxy, proxyInfo.isHttpProxy());
  }

  void test_is_http_proxy()
  {
    const std::string url = "some_url";
    const int port = 1;
    const bool isHttpProxy = false;
    ProxyInfo proxyInfo(url, port, isHttpProxy);
    TS_ASSERT_EQUALS(isHttpProxy, proxyInfo.isHttpProxy());
  }

  void test_copy()
  {
    const std::string url = "some_url";
    const int port = 1;
    const bool isHttpProxy = true;
    ProxyInfo a(url, port, isHttpProxy);
    ProxyInfo b = a;

    TS_ASSERT_EQUALS(a.host(), b.host());
    TS_ASSERT_EQUALS(a.port(), b.port());
    TS_ASSERT_EQUALS(a.isHttpProxy(), b.isHttpProxy());
  }

  void test_assign()
  {
    ProxyInfo a("a", 1, false);
    ProxyInfo b("b", 2, true);

    a = b;
    TS_ASSERT_EQUALS(a.host(), b.host());
    TS_ASSERT_EQUALS(a.port(), b.port());
    TS_ASSERT_EQUALS(a.isHttpProxy(), b.isHttpProxy());
  }

};

#endif /* MANTID_SCRIPTREPOSITORY_PROXYINFOTEST_H_ */
