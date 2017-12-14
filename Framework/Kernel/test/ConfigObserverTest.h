#ifndef MANTID_CONFIGOBSERVERTEST_H_
#define MANTID_CONFIGOBSERVERTEST_H_

#include "MantidKernel/ConfigService.h"
#include "MantidKernel/ConfigObserver.h"
#include "MantidKernel/System.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid::Kernel;

template <typename Callback> class MockObserver : ConfigObserver {
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

class ConfigObserverTest : public CxxTest::TestSuite {
public:
  void setUp() override {
    m_searchDirectories =
        ConfigService::Instance().getString("datasearch.directories");
    m_defaultSaveDirectory =
        ConfigService::Instance().getString("defaultsave.directory");
  }

  void tearDown() override {
    ConfigService::Instance().setString("datasearch.directories",
                                        m_searchDirectories);
    ConfigService::Instance().setString("defaultsave.directory",
                                        m_defaultSaveDirectory);
  }

  void testRecievesCallbackForOutputDirectoryChange() {
    auto call_count = 0;
    auto constexpr NUMBER_OF_PROPERTIES_CHANGED = 2;
    auto observer = makeMockObserver(
        [&call_count](const std::string &name, const std::string &newValue,
                      const std::string &prevValue) -> void {
          UNUSED_ARG(name);
          UNUSED_ARG(newValue);
          UNUSED_ARG(prevValue);
          call_count++;
        });
    ConfigService::Instance().setString("defaultsave.directory", "/dev/null");
    TS_ASSERT_EQUALS(NUMBER_OF_PROPERTIES_CHANGED, call_count);
  }

  void testCopysObserverOnCopyConstruction() {
    auto call_count = 0;
    auto constexpr NUMBER_OF_PROPERTIES_CHANGED = 2;
    auto observer = makeMockObserver(
        [&call_count](const std::string &name, const std::string &newValue,
                      const std::string &prevValue) -> void {
          UNUSED_ARG(name);
          UNUSED_ARG(newValue);
          UNUSED_ARG(prevValue);
          call_count++;
        });
    auto copyOfObserver = observer;
    ConfigService::Instance().setString("defaultsave.directory", "/dev/null");
    TS_ASSERT_EQUALS(NUMBER_OF_PROPERTIES_CHANGED * 2, call_count);
  }

  void testMovesObserverOnMoveConstruction() {
    auto call_count = 0;
    auto constexpr NUMBER_OF_PROPERTIES_CHANGED = 2;
    auto observer = makeMockObserver(
        [&call_count](const std::string &name, const std::string &newValue,
                      const std::string &prevValue) -> void {
          UNUSED_ARG(name);
          UNUSED_ARG(newValue);
          UNUSED_ARG(prevValue);
          call_count++;
        });
    auto movedObserver = std::move(observer);
    ConfigService::Instance().setString("defaultsave.directory", "/dev/null");
    TS_ASSERT_EQUALS(NUMBER_OF_PROPERTIES_CHANGED, call_count);
  }

private:
  std::string m_searchDirectories;
  std::string m_defaultSaveDirectory;
};
#endif // MANTID_CONFIGSERVICEOBSERVERTEST_H_
