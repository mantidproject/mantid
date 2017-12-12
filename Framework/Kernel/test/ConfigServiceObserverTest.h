#ifndef MANTID_CONFIGSERVICEOBSERVERTEST_H_
#define MANTID_CONFIGSERVICEOBSERVERTEST_H_

#include "MantidKernel/ConfigService.h"
#include "MantidKernel/ConfigServiceObserver.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid::Kernel;

template <typename Callback> class MockObserver : ConfigServiceObserver {
public:
  MockObserver(Callback callback) : m_callback(callback) {}

protected:
  void onValueChanged(const std::string &name, const std::string &newValue,
                      const std::string &prevValue) override {
    m_callback(name, newValue, prevValue);
  }

private:
  Callback m_callback;
};

template <typename Callback>
MockObserver<Callback> makeMockObserver(Callback callback) {
  return MockObserver<Callback>(callback);
}

class ConfigServiceObserverTest : public CxxTest::TestSuite {
public:
  void testRecievesCallbackForOutputDirectoryChange() {
    auto called = false;
    auto observer = makeMockObserver(
        [&called](const std::string &name, const std::string &newValue,
                  const std::string &prevValue) -> void { called = true; });
    ConfigService::Instance().setString("defaultsave.directory", "/dev/null");
    TS_ASSERT(called);
  }
};
#endif // MANTID_CONFIGSERVICEOBSERVERTEST_H_
