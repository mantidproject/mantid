// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_REMOTEJOBMANAGERS_LSFJOBMANAGERTEST_H_
#define MANTID_REMOTEJOBMANAGERS_LSFJOBMANAGERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/RemoteJobManagerFactory.h"
#include "MantidRemoteJobManagers/LSFJobManager.h"

#include <boost/make_shared.hpp>

// TODO: use gmock
class MockedLSFJobManager : public Mantid::RemoteJobManagers::LSFJobManager {
  /// needs to define this pure virtual method
  void authenticate(const std::string & /*username*/,
                    const std::string & /*password*/) override {}

  void logout(const std::string & /* username */) override {}
};

/// This just checks basic cast/interface properties of a virtual
/// class (LSFJobManager::authenticate() = 0).
/// What can be tested is mostly in the test of SCARFLSFJobManager
/// which derives from LSFJobManager.
///
/// The factory cannot be tested with these LSFJobManager-derived classes (like
/// MockedLSFJobManager) because there is no computeResource in the facilities
/// file that uses them as remote job manager. To do a test of the remote job
/// factory with for example 'MockedLSFJobManager' we'd need a
/// ComputeResourceInfo constructor that does not require a facility /
/// facilities file element. This is not supported at the moment and might never
/// make sense, unless you want to create and use compute resources without
/// adding them in the facilities file.
class LSFJobManagerTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LSFJobManagerTest *createSuite() { return new LSFJobManagerTest(); }
  static void destroySuite(LSFJobManagerTest *suite) { delete suite; }

  void test_constructCasts() {
    // can create
    boost::shared_ptr<MockedLSFJobManager> jm;
    TS_ASSERT(jm = boost::make_shared<MockedLSFJobManager>());
    // can cast to inherited interfaces and base classes

    MockedLSFJobManager lsf;
    TSM_ASSERT(
        "Job manager constructed dynamically should cast to LSFJobManager",
        dynamic_cast<Mantid::RemoteJobManagers::LSFJobManager *>(jm.get()));
    TSM_ASSERT(
        "Job manager constructed statically should cast to LSFJobManager",
        dynamic_cast<Mantid::RemoteJobManagers::LSFJobManager *>(&lsf));
    TSM_ASSERT(
        "Job manager constructed dynamically should cast to IRemoteJobManager",
        dynamic_cast<Mantid::API::IRemoteJobManager *>(jm.get()));
    TSM_ASSERT(
        "Job manager constructed statically should cast to IRemoteJobManger",
        dynamic_cast<Mantid::API::IRemoteJobManager *>(&lsf));
  }
};

#endif // MANTID_REMOTEJOGMANAGERS_LSFJOBMANAGERTEST_H_
