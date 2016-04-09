#ifndef MANTID_DATAHANDLING_IMGGAGGREGATEWAVELENGTHSTEST_H_
#define MANTID_DATAHANDLING_IMGGAGGREGATEWAVELENGTHSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/ImggAggregateWavelengths.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/Exception.h"

using Mantid::DataHandling::ImggAggregateWavelengths;

// This algorithm is all about I/O. No effective functional testing is
// done here, but in system tests
class ImggAggregateWavelengthsTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ImggAggregateWavelengthsTest *createSuite() {
    return new ImggAggregateWavelengthsTest();
  }

  static void destroySuite(ImggAggregateWavelengthsTest *suite) {
    delete suite;
  }

  void test_init() {
    ImggAggregateWavelengths alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    int numProjs, numBands;
    TS_ASSERT_THROWS_NOTHING(numProjs = alg.getProperty("NumProjections"));
    TS_ASSERT_THROWS_NOTHING(numBands = alg.getProperty("NumBands"));
    TS_ASSERT_EQUALS(numProjs, 0);
    TS_ASSERT_EQUALS(numBands, 0);
  }

  void test_exec_fail() {
    ImggAggregateWavelengths alg;
    // Don't put output in ADS by default
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputPath", "."));
    TS_ASSERT_THROWS(alg.setPropertyValue("OutputPath", "inexistent_fail"),
                     std::invalid_argument);

    TS_ASSERT_THROWS(alg.execute(), std::runtime_error);
    TS_ASSERT(!alg.isExecuted());
  }

  void test_errors_options() {
    auto alg = Mantid::API::AlgorithmManager::Instance().create(
        "ImggAggregateWavelengths");

    TS_ASSERT_THROWS(
        alg->setPropertyValue("OutputWorkspace", "_unused_for_child"),
        Mantid::Kernel::Exception::NotFoundError);

    TS_ASSERT_THROWS(alg->setPropertyValue("UniformBands", "fail"),
                     std::invalid_argument);
  }

  void test_too_many_options() {
    auto alg = Mantid::API::AlgorithmManager::Instance().create(
        "ImggAggregateWavelengths");

    alg->setPropertyValue("IndexRanges", "1-10");
    alg->setPropertyValue("ToFRanges", "5000-7000");
    TS_ASSERT_THROWS(alg->execute(), std::runtime_error);
    TS_ASSERT(!alg->isExecuted());

    auto alg2nd = Mantid::API::AlgorithmManager::Instance().create(
        "ImggAggregateWavelengths");
    alg2nd->setPropertyValue("ToFRanges", "5000-7000");
    alg2nd->setPropertyValue("IndexRanges", "1-10");
    TS_ASSERT_THROWS(alg2nd->execute(), std::runtime_error);
    TS_ASSERT(!alg2nd->isExecuted());

    auto alg3rd = Mantid::API::AlgorithmManager::Instance().create(
        "ImggAggregateWavelengths");
    alg3rd->setPropertyValue("UniformBands", "3");
    alg3rd->setPropertyValue("IndexRanges", "1-10");
    TS_ASSERT_THROWS(alg3rd->execute(), std::runtime_error);
    TS_ASSERT(!alg3rd->isExecuted());
  }

  void test_formats() {
    auto alg = Mantid::API::AlgorithmManager::Instance().create(
        "ImggAggregateWavelengths");

    alg->setPropertyValue("InputImageFormat", "FITS");
    TS_ASSERT_THROWS(alg->setPropertyValue("InputImageFormat", "Bla");
                     , std::invalid_argument);
    TS_ASSERT_THROWS(alg->setPropertyValue("InputImageFormat", "TIFF");
                     , std::invalid_argument);

    auto alg2nd = Mantid::API::AlgorithmManager::Instance().create(
        "ImggAggregateWavelengths");
    alg2nd->setPropertyValue("OutputImageFormat", "FITS");
    TS_ASSERT_THROWS(alg2nd->setPropertyValue("OutputImageFormat", "Wrong");
                     , std::invalid_argument);
    TS_ASSERT_THROWS(alg2nd->setPropertyValue("OutputImageFormat", "TIFF");
                     , std::invalid_argument);
  }
};

#endif /* MANTID_DATAHANDLING_IMGGAGGREGATEWAVELENGTHSTEST_H_ */