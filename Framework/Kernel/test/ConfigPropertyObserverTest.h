#ifndef MANTID_CONFIGPROPERTYOBSERVERTEST_H_
#define MANTID_CONFIGPROPERTYOBSERVERTEST_H_

#include "MantidKernel/ConfigService.h"
#include "MantidKernel/ConfigPropertyObserver.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid::Kernel;

template <typename Callback> class MockObserver : ConfigPropertyObserver {
public:
  MockObserver(std::string propertyName, Callback callback)
      : ConfigPropertyObserver(std::move(propertyName)), m_callback(callback) {}

protected:
  void onPropertyValueChanged(const std::string &newValue,
                              const std::string &prevValue) override {
    m_callback(newValue, prevValue);
  }

private:
  Callback m_callback;
};

template <typename Callback>
MockObserver<Callback> makeMockObserver(std::string const &propertyName,
                                        Callback callback) {
  return MockObserver<Callback>(propertyName, callback);
}

class ConfigPropertyObserverTest : public CxxTest::TestSuite {
public:
  void setUp() override {
    m_searchDirectories =
        ConfigService::Instance().getString("datasearch.directories");
    m_defaultSaveDirectory =
        ConfigService::Instance().getString("defaultsave.directory");
    m_retainedAlgorithms =
        ConfigService::Instance().getString("algorithms.retained");
  }

  void tearDown() override {
    ConfigService::Instance().setString("datasearch.directories",
                                        m_searchDirectories);
    ConfigService::Instance().setString("defaultsave.directory",
                                        m_defaultSaveDirectory);
    ConfigService::Instance().setString("algorithms.retained",
                                        m_retainedAlgorithms);
  }

  void testRecievesCallbackForSearchDirectoryChange() {
    auto callCount = 0;
    auto constexpr NUMBER_OF_PROPERTIES_CHANGED = 1;
    auto observer =
        makeMockObserver("datasearch.directories",
                         [&callCount](const std::string &newValue,
                                      const std::string &prevValue) -> void {
                           UNUSED_ARG(newValue);
                           UNUSED_ARG(prevValue);
                           callCount++;
                         });
    ConfigService::Instance().setString("datasearch.directories", "/dev/null");
    TS_ASSERT_EQUALS(NUMBER_OF_PROPERTIES_CHANGED, callCount);
  }

  void testRecievesCallbackForOutputDirectoryChangeOnly() {
    auto callCount = 0;
    auto constexpr NUMBER_OF_PROPERTIES_CHANGED = 1;
    auto observer =
        makeMockObserver("datasearch.directories",
                         [&callCount](const std::string &newValue,
                                      const std::string &prevValue) -> void {
                           UNUSED_ARG(newValue);
                           UNUSED_ARG(prevValue);
                           callCount++;
                         });
    ConfigService::Instance().setString("datasearch.directories", "/dev/null");
    TS_ASSERT_EQUALS(NUMBER_OF_PROPERTIES_CHANGED, callCount);
  }

  void testMultipleObserversForDifferentProperties() {
    auto callCountA = 0;
    auto observerA =
        makeMockObserver("datasearch.directories",
                         [&callCountA](const std::string &newValue,
                                       const std::string &prevValue) -> void {
                           UNUSED_ARG(newValue);
                           UNUSED_ARG(prevValue);
                           callCountA++;
                         });
    auto callCountB = 0;
    auto observerB =
        makeMockObserver("algorithms.retained",
                         [&callCountB](const std::string &newValue,
                                       const std::string &prevValue) -> void {
                           UNUSED_ARG(newValue);
                           UNUSED_ARG(prevValue);
                           callCountB++;
                         });

    ConfigService::Instance().setString("datasearch.directories", "/dev/null");
    ConfigService::Instance().setString("algorithms.retained", "40");

    TS_ASSERT_EQUALS(1, callCountA);
    TS_ASSERT_EQUALS(1, callCountB);
  }

private:
  std::string m_searchDirectories;
  std::string m_defaultSaveDirectory;
  std::string m_retainedAlgorithms;
};
#endif // MANTID_CONFIGPROPERTYOBSERVERTEST_H_
