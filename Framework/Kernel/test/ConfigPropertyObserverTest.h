// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/ConfigPropertyObserver.h"
#include "MantidKernel/ConfigService.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid::Kernel;

template <typename Callback> class MockObserver : ConfigPropertyObserver {
public:
  MockObserver(const std::string &propertyName, Callback callback)
      : ConfigPropertyObserver(std::move(propertyName)), m_callback(callback) {}

protected:
  void onPropertyValueChanged(const std::string &newValue, const std::string &prevValue) override {
    m_callback(newValue, prevValue);
  }

private:
  Callback m_callback;
};

template <typename Callback>
MockObserver<Callback> makeMockObserver(std::string const &propertyName, Callback callback) {
  return MockObserver<Callback>(propertyName, callback);
}

class ConfigPropertyObserverTest : public CxxTest::TestSuite {
public:
  void setUp() override {
    m_searchDirectories = ConfigService::Instance().getString("datasearch.directories");
    m_defaultSaveDirectory = ConfigService::Instance().getString("defaultsave.directory");
  }

  void tearDown() override {
    ConfigService::Instance().setString("datasearch.directories", m_searchDirectories);
    ConfigService::Instance().setString("defaultsave.directory", m_defaultSaveDirectory);
  }

  void testRecievesCallbackForSearchDirectoryChange() {
    auto callCount = 0;
    auto constexpr NUMBER_OF_PROPERTIES_CHANGED = 1;
    auto observer = makeMockObserver("datasearch.directories",
                                     [&callCount](const std::string &newValue, const std::string &prevValue) -> void {
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
    auto observer = makeMockObserver("datasearch.directories",
                                     [&callCount](const std::string &newValue, const std::string &prevValue) -> void {
                                       UNUSED_ARG(newValue);
                                       UNUSED_ARG(prevValue);
                                       callCount++;
                                     });
    ConfigService::Instance().setString("datasearch.directories", "/dev/null");
    TS_ASSERT_EQUALS(NUMBER_OF_PROPERTIES_CHANGED, callCount);
  }

  void testMultipleObserversForDifferentProperties() {
    auto callCountA = 0;
    auto observerA = makeMockObserver("datasearch.directories",
                                      [&callCountA](const std::string &newValue, const std::string &prevValue) -> void {
                                        UNUSED_ARG(newValue);
                                        UNUSED_ARG(prevValue);
                                        callCountA++;
                                      });
    auto callCountB = 0;
    auto observerB = makeMockObserver("projectRecovery.secondsBetween",
                                      [&callCountB](const std::string &newValue, const std::string &prevValue) -> void {
                                        UNUSED_ARG(newValue);
                                        UNUSED_ARG(prevValue);
                                        callCountB++;
                                      });

    ConfigService::Instance().setString("datasearch.directories", "/dev/null");
    ConfigService::Instance().setString("projectRecovery.secondsBetween", "600");

    TS_ASSERT_EQUALS(1, callCountA);
    TS_ASSERT_EQUALS(1, callCountB);
  }

private:
  std::string m_searchDirectories;
  std::string m_defaultSaveDirectory;
  std::string m_retainedAlgorithms;
};
