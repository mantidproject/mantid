// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/ICatalogInfo.h"
#include "MantidKernel/UserCatalogInfo.h"
#include "MantidKernel/WarningSuppressions.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

using namespace testing;

namespace boost {
template <class CharType, class CharTrait>
std::basic_ostream<CharType, CharTrait> &operator<<(std::basic_ostream<CharType, CharTrait> &out,
                                                    std::optional<std::string> const &maybe) {
  if (maybe)
    out << maybe;
  return out;
}
} // namespace boost
GNU_DIAG_OFF_SUGGEST_OVERRIDE
using namespace Mantid::Kernel;
class MockCatalogConfigService : public CatalogConfigService {
public:
  MOCK_CONST_METHOD0(preferredMountPoint, OptionalPath());
};

class MockICatalogInfo : public ICatalogInfo {
public:
  MOCK_CONST_METHOD0(catalogName, const std::string());
  MOCK_CONST_METHOD0(soapEndPoint, const std::string());
  MOCK_CONST_METHOD0(externalDownloadURL, const std::string());
  MOCK_CONST_METHOD0(catalogPrefix, const std::string());
  MOCK_CONST_METHOD0(windowsPrefix, const std::string());
  MOCK_CONST_METHOD0(macPrefix, const std::string());
  MOCK_CONST_METHOD0(linuxPrefix, const std::string());
  MOCK_CONST_METHOD0(clone, ICatalogInfo *());
};
GNU_DIAG_ON_SUGGEST_OVERRIDE
// Adaptee
struct UserType {
  std::string getString(const std::string &) const { return "my_value"; }
};

class UserCatalogInfoTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static UserCatalogInfoTest *createSuite() { return new UserCatalogInfoTest(); }
  static void destroySuite(UserCatalogInfoTest *suite) { delete suite; }

  void test_pass_through_adaptee() {

    // Adaptee
    MockICatalogInfo host;
    auto mockCatInfo = new MockICatalogInfo;

    // because the input gets cloned, we need to set our expectations upon the
    // clone product
    EXPECT_CALL(host, clone()).WillOnce(Return(mockCatInfo));
    EXPECT_CALL(*mockCatInfo, catalogName()).Times(Exactly(1));
    EXPECT_CALL(*mockCatInfo, catalogPrefix()).Times(Exactly(1));
    EXPECT_CALL(*mockCatInfo, externalDownloadURL()).Times(Exactly(1));
    EXPECT_CALL(*mockCatInfo, linuxPrefix()).Times(Exactly(1));
    EXPECT_CALL(*mockCatInfo, macPrefix()).Times(Exactly(1));
    EXPECT_CALL(*mockCatInfo, windowsPrefix()).Times(Exactly(1));
    EXPECT_CALL(*mockCatInfo, soapEndPoint()).Times(Exactly(1));

    // The user service will return to the effect that there are no user
    // overrides
    MockCatalogConfigService mockConfigService;
    EXPECT_CALL(mockConfigService, preferredMountPoint()).WillRepeatedly(Return(OptionalPath())); // No user override

    // Setup the UserCatalogInfo
    UserCatalogInfo userAdapter(host, mockConfigService);

    userAdapter.catalogName();
    userAdapter.catalogPrefix();
    userAdapter.externalDownloadURL();
    userAdapter.linuxPrefix();
    userAdapter.macPrefix();
    userAdapter.windowsPrefix();
    userAdapter.soapEndPoint();

    TS_ASSERT(Mock::VerifyAndClear(&mockConfigService));
    TS_ASSERT(Mock::VerifyAndClear(&mockCatInfo));
  }

  void test_mac_path_customizeable() {

    auto mockCatInfo = new MockICatalogInfo;
    MockICatalogInfo host;
    MockCatalogConfigService mockConfigService;

    const std::string expectedPath = "/custom_windows_mountpoint";

    // Set the User override to be used to return the expected path.
    EXPECT_CALL(mockConfigService, preferredMountPoint())
        .WillRepeatedly(Return(OptionalPath(expectedPath))); // overriden path.

    EXPECT_CALL(host, clone()).WillOnce(Return(mockCatInfo)); // because the input gets cloned, we
                                                              // need to set our expectations upon the
                                                              // clone product
    // Expect that the facility default is not used.
    EXPECT_CALL(*mockCatInfo, macPrefix()).Times(Exactly(0));

    UserCatalogInfo userCatInfo(host, mockConfigService);
    std::string actualPath = userCatInfo.macPrefix();

    TSM_ASSERT_EQUALS("Mac mount point should have come from user override", expectedPath, actualPath);
    TS_ASSERT(Mock::VerifyAndClear(mockCatInfo));
    TS_ASSERT(Mock::VerifyAndClear(&host));
    TS_ASSERT(Mock::VerifyAndClear(&mockConfigService));
  }

  void test_linux_path_customizeable() {

    auto mockCatInfo = new MockICatalogInfo;
    MockICatalogInfo host;
    MockCatalogConfigService mockConfigService;

    const std::string expectedPath = "/custom_windows_mountpoint";

    // Set the User override to be used to return the expected path.
    EXPECT_CALL(mockConfigService, preferredMountPoint())
        .WillRepeatedly(Return(OptionalPath(expectedPath))); // overriden path.

    EXPECT_CALL(host, clone()).WillOnce(Return(mockCatInfo)); // because the input gets cloned, we
                                                              // need to set our expectations upon the
                                                              // clone product
    // Expect that the facility default is not used.
    EXPECT_CALL(*mockCatInfo, linuxPrefix()).Times(Exactly(0));

    UserCatalogInfo userCatInfo(host, mockConfigService);
    std::string actualPath = userCatInfo.linuxPrefix();

    TSM_ASSERT_EQUALS("Linux mount point should have come from user override", expectedPath, actualPath);
    TS_ASSERT(Mock::VerifyAndClear(mockCatInfo));
    TS_ASSERT(Mock::VerifyAndClear(&host));
    TS_ASSERT(Mock::VerifyAndClear(&mockConfigService));
  }

  void test_windows_path_customizeable() {

    auto mockCatInfo = new MockICatalogInfo;
    MockICatalogInfo host;
    MockCatalogConfigService mockConfigService;

    const std::string expectedPath = "/custom_windows_mountpoint";

    // Set the User override to be used to return the expected path.
    EXPECT_CALL(mockConfigService, preferredMountPoint())
        .WillRepeatedly(Return(OptionalPath(expectedPath))); // overriden path.

    EXPECT_CALL(host, clone()).WillOnce(Return(mockCatInfo)); // because the input gets cloned, we
                                                              // need to set our expectations upon the
                                                              // clone product
    // Expect that the facility default is not used.
    EXPECT_CALL(*mockCatInfo, windowsPrefix()).Times(Exactly(0));

    UserCatalogInfo userCatInfo(host, mockConfigService);
    std::string actualPath = userCatInfo.windowsPrefix();

    TSM_ASSERT_EQUALS("Windows mount point should have come from user override", expectedPath, actualPath);
    TS_ASSERT(Mock::VerifyAndClear(mockCatInfo));
    TS_ASSERT(Mock::VerifyAndClear(&host));
    TS_ASSERT(Mock::VerifyAndClear(&mockConfigService));
  }

  void test_auto_adapter() {
    UserType usertype;
    CatalogConfigService *service = makeCatalogConfigServiceAdapter(usertype, std::string("my_key"));
    OptionalPath mountPoint = service->preferredMountPoint();
    TS_ASSERT(mountPoint);
    TS_ASSERT_EQUALS("my_value", mountPoint.value());
    delete service;
  }
};
