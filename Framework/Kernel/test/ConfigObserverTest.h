// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/ConfigObserver.h"
#include "MantidKernel/ConfigService.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid::Kernel;

template <typename Callback> class MockObserver : public ConfigObserver {
public:
  MockObserver(Callback callback) : m_callback(callback) {}

protected:
  void onValueChanged(const std::string &name, const std::string &newValue, const std::string &prevValue) override {
    m_callback(name, newValue, prevValue, static_cast<ConfigObserver *>(this));
  }

private:
  Callback m_callback;
};

template <typename Callback> MockObserver<Callback> makeMockObserver(Callback callback) {
  return MockObserver<Callback>(callback);
}

class ConfigObserverTest : public CxxTest::TestSuite {
public:
  void setUp() override {
    m_searchDirectories = ConfigService::Instance().getString("datasearch.directories");
    m_defaultSaveDirectory = ConfigService::Instance().getString("defaultsave.directory");
  }

  void tearDown() override {
    ConfigService::Instance().setString("datasearch.directories", m_searchDirectories);
    ConfigService::Instance().setString("defaultsave.directory", m_defaultSaveDirectory);
  }

  void testRecievesCallbackForOutputDirectoryChange() {
    auto callCount = 0;
    auto constexpr NUMBER_OF_PROPERTIES_CHANGED = 2;
    auto observer = makeMockObserver([&callCount](const std::string &name, const std::string &newValue,
                                                  const std::string &prevValue, ConfigObserver *self) -> void {
      UNUSED_ARG(name);
      UNUSED_ARG(newValue);
      UNUSED_ARG(prevValue);
      UNUSED_ARG(self);
      callCount++;
    });
    ConfigService::Instance().setString("defaultsave.directory", "/dev/null");
    TS_ASSERT_EQUALS(NUMBER_OF_PROPERTIES_CHANGED, callCount);
  }

  void testCreatesNewObserverOnCopyConstruction() {
    ConfigObserver *lastCaller = nullptr;
    auto callCount = 0;
    auto constexpr NUMBER_OF_PROPERTIES_CHANGED = 1;
    auto observer =
        makeMockObserver([&callCount, &lastCaller](const std::string &name, const std::string &newValue,
                                                   const std::string &prevValue, ConfigObserver *self) -> void {
          UNUSED_ARG(name);
          UNUSED_ARG(newValue);
          UNUSED_ARG(prevValue);
          callCount++;
          if (lastCaller == nullptr)
            lastCaller = self;
          else
            TSM_ASSERT("The same observer was notified twice", lastCaller != self);
        });
    auto copyOfObserver = observer;
    ConfigService::Instance().setString("datasearch.directories", "/dev/null");
    TS_ASSERT_EQUALS(NUMBER_OF_PROPERTIES_CHANGED * 2, callCount);
  }

private:
  std::string m_searchDirectories;
  std::string m_defaultSaveDirectory;
};
