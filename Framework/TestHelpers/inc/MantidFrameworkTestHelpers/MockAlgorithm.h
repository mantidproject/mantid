// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "MantidAPI/IAlgorithm.h"
#include "MantidJson/Json.h"
#include "MantidKernel/Property.h"

#include "MantidKernel/WarningSuppressions.h"

using namespace Mantid::API;
using namespace testing;

GNU_DIAG_OFF_SUGGEST_OVERRIDE

class MockAlgorithm : public IAlgorithm {
public:
  MOCK_CONST_METHOD0(name, const std::string());
  MOCK_CONST_METHOD0(version, int());
  MOCK_CONST_METHOD0(summary, const std::string());
  MOCK_CONST_METHOD0(category, const std::string());
  MOCK_CONST_METHOD0(categories, const std::vector<std::string>());
  MOCK_CONST_METHOD0(categorySeparator, const std::string());
  MOCK_CONST_METHOD0(seeAlso, const std::vector<std::string>());
  MOCK_CONST_METHOD0(aliasDeprecated, const std::string());
  MOCK_CONST_METHOD0(alias, const std::string());
  MOCK_CONST_METHOD0(helpURL, const std::string());

  MOCK_CONST_METHOD0(workspaceMethodName, const std::string());
  MOCK_CONST_METHOD0(workspaceMethodOn, const std::vector<std::string>());
  MOCK_CONST_METHOD0(workspaceMethodInputProperty, const std::string());

  MOCK_CONST_METHOD0(getAlgorithmID, AlgorithmID());

  MOCK_METHOD0(initialize, void());
  MOCK_METHOD0(validateInputs, std::map<std::string, std::string>());
  MOCK_METHOD0(execute, bool());
  MOCK_METHOD0(executeAsync, Poco::ActiveResult<bool>());
  MOCK_METHOD0(executeAsChildAlg, void());

  MOCK_CONST_METHOD0(executionState, ExecutionState());
  MOCK_CONST_METHOD0(resultState, ResultState());

  MOCK_CONST_METHOD0(isInitialized, bool());
  MOCK_CONST_METHOD0(isExecuted, bool());

  MOCK_METHOD0(cancel, void());
  MOCK_CONST_METHOD0(isRunning, bool());
  MOCK_CONST_METHOD0(isReadyForGarbageCollection, bool());
  MOCK_CONST_METHOD0(isChild, bool());
  MOCK_CONST_METHOD0(getAlwaysStoreInADS, bool());

  MOCK_METHOD1(setChild, void(const bool));
  MOCK_METHOD1(enableHistoryRecordingForChild, void(const bool));
  MOCK_METHOD1(setAlwaysStoreInADS, void(const bool));
  MOCK_METHOD1(setRethrows, void(const bool));

  MOCK_CONST_METHOD1(addObserver, void(const Poco::AbstractObserver &));
  MOCK_CONST_METHOD1(removeObserver, void(const Poco::AbstractObserver &));

  MOCK_METHOD1(setLogging, void(const bool));
  MOCK_CONST_METHOD0(isLogging, bool());
  MOCK_METHOD1(setLoggingOffset, void(const int));
  MOCK_CONST_METHOD0(getLoggingOffset, int());
  MOCK_METHOD1(setAlgStartupLogging, void(const bool));
  MOCK_CONST_METHOD0(getAlgStartupLogging, bool());

  MOCK_CONST_METHOD1(setChildStartProgress, void(const double));
  MOCK_CONST_METHOD1(setChildEndProgress, void(const double));

  MOCK_CONST_METHOD0(toString, std::string());
  MOCK_CONST_METHOD0(toJson, ::Json::Value());

  // IPropertyManager methods
  MOCK_CONST_METHOD1(getProperty, TypedValue(const std::string &));
  MOCK_CONST_METHOD1(getPointerToProperty, Mantid::Kernel::Property *(const std::string &));
  MOCK_METHOD0(clear, void());
  MOCK_CONST_METHOD1(getPointerToPropertyOrdinal, Mantid::Kernel::Property *(const int &));
  MOCK_METHOD2(removeProperty, void(const std::string &, const bool));
  MOCK_METHOD1(takeProperty, std::unique_ptr<Mantid::Kernel::Property>(const size_t));
  MOCK_METHOD0(resetProperties, void());

  MOCK_METHOD2(setPropertiesWithString, void(const std::string &, const std::unordered_set<std::string> &));
  MOCK_METHOD3(setProperties, void(const std::string &, const std::unordered_set<std::string> &, bool));
  MOCK_METHOD3(setProperties, void(const ::Json::Value &, const std::unordered_set<std::string> &, bool));

  MOCK_METHOD2(setPropertyValue, void(const std::string &, const std::string &));
  MOCK_METHOD2(setPropertyValueFromJson, void(const std::string &, const Json::Value &));
  MOCK_METHOD2(setPropertyOrdinal, void(const int &, const std::string &));

  MOCK_CONST_METHOD1(existsProperty, bool(const std::string &));
  MOCK_CONST_METHOD0(validateProperties, bool());
  MOCK_CONST_METHOD0(propertyCount, size_t());
  MOCK_CONST_METHOD1(getPropertyValue, std::string(const std::string &));
  MOCK_CONST_METHOD0(getProperties, const std::vector<Mantid::Kernel::Property *> &());
  MOCK_METHOD((std::vector<std::string>), getDeclaredPropertyNames, (), (const, noexcept));
  MOCK_CONST_METHOD1(asString, std::string(bool));
  MOCK_CONST_METHOD1(asJson, ::Json::Value(bool));

  MOCK_METHOD2(declareProperty, void(std::unique_ptr<Mantid::Kernel::Property>, const std::string &));
  MOCK_METHOD2(declareOrReplaceProperty, void(std::unique_ptr<Mantid::Kernel::Property>, const std::string &));

  // Methods to help when testing
  template <typename T> void expectGetProperty(std::string const &propertyName, PropertyWithValue<T> *prop) {
    EXPECT_CALL(*this, getPointerToProperty(propertyName)).WillOnce(Return(prop));
    EXPECT_CALL(*this, getProperty(propertyName)).WillOnce(Return(TypedValue(*this, propertyName)));
  }
};

GNU_DIAG_ON_SUGGEST_OVERRIDE