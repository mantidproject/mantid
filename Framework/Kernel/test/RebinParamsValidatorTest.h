// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/Memory.h"
#include "MantidKernel/RebinParamsValidator.h"
#include <cxxtest/TestSuite.h>
#include <memory>

using namespace Mantid::Kernel;

// this is a simple mock of the availMem function to keep consistent and small values from this function
std::size_t Mantid::Kernel::MemoryStats::availMem() const { return 12; }

class RebinParamsValidatorTest : public CxxTest::TestSuite {
public:
  void testClone() {
    IValidator_sptr v = std::make_shared<RebinParamsValidator>();
    IValidator_sptr vv = v->clone();
    TS_ASSERT_DIFFERS(v, vv);
    TS_ASSERT(std::dynamic_pointer_cast<RebinParamsValidator>(vv));
  }

  void testCast() {
    RebinParamsValidator *d = new RebinParamsValidator;
    TS_ASSERT(dynamic_cast<IValidator *>(d));
    delete d;
  }

  void testFailEmpty() {
    TS_ASSERT_EQUALS(standardValidator.isValid(std::vector<double>()), "Enter values for this property");
  }

  void testSucceedEmpty() {
    RebinParamsValidator allowEmptyValidator = RebinParamsValidator(true);
    TS_ASSERT(allowEmptyValidator.isValid(std::vector<double>()).empty());
  }

  void testFailWrongLength() {
    const std::vector<double> vec(6, 1.0);
    TS_ASSERT(!standardValidator.isValid(vec).empty());
  }

  void testFailOutOfOrder() {
    std::vector<double> vec(5);
    vec[0] = 1.0;
    vec[1] = 0.1;
    vec[2] = 2.0;
    vec[3] = 0.2;
    vec[4] = 1.5;
    TS_ASSERT_EQUALS(standardValidator.isValid(vec), "Bin boundary values must be given in order of increasing value");
  }

  void testFailZeroBin_only() {
    // Don't give a 0 bin
    std::vector<double> vec{0.0};
    TS_ASSERT(!standardValidator.isValid(vec).empty());
  }

  void testFailZeroBin_or_bad_log() {
    // Don't give a 0 bin
    std::vector<double> vec(3);
    vec[0] = 1.0;
    vec[1] = 0.0;
    vec[2] = 2.0;
    TS_ASSERT(!standardValidator.isValid(vec).empty());
    // Logarithmic bin starts at 0
    vec[0] = 0.0;
    vec[1] = -1.0;
    vec[2] = 200.0;
    TS_ASSERT(!standardValidator.isValid(vec).empty());
    // Logarithmic bin starts at -ve number
    vec[0] = -5.0;
    vec[1] = -1.0;
    vec[2] = 10.0;
    TS_ASSERT(!standardValidator.isValid(vec).empty());
  }

  void testCorrect() {
    std::vector<double> vec(5);
    vec[0] = 1.0;
    vec[1] = 0.1;
    vec[2] = 2.0;
    vec[3] = 0.2;
    vec[4] = 2.5;
    TS_ASSERT(standardValidator.isValid(vec).empty());
  }

  void testRange() {
    auto allowRangeValidator = RebinParamsValidator(true, true);
    std::vector<double> vec(2);
    vec[0] = 0.0;
    vec[1] = 1.0;
    TS_ASSERT(allowRangeValidator.isValid(vec).empty());
  }

  void testRangeWrongOrder() {
    auto allowRangeValidator = RebinParamsValidator(true, true);
    std::vector<double> vec(2);
    vec[0] = 1.0;
    vec[1] = 0.0;
    TS_ASSERT_EQUALS(allowRangeValidator.isValid(vec),
                     "When giving a range the second value must be larger than the first");
  }

  void testTooManyBins() {
    size_t mem = Mantid::Kernel::MemoryStats().availMem();
    size_t numBins = mem * 1024 / sizeof(double) + 1; // one more than can fit in memory
    std::vector<double> vec(3);
    vec[0] = 1.0;
    vec[1] = 1.0;
    vec[2] = 1.0 + static_cast<double>(numBins); // make sure we have more than numBins bins
    TS_ASSERT(!standardValidator.isValid(vec).empty());
  }

  void testTooManyBinsLog() {
    size_t mem = Mantid::Kernel::MemoryStats().availMem();
    size_t numBins = mem * 1024 / sizeof(double) + 1; // one more than can fit in memory
    std::vector<double> vec(3);
    vec[0] = 1.0;
    vec[1] = 1.0 - std::pow(10.0, 1. / static_cast<double>(numBins)); // make sure we have more than numBins bins
    vec[2] = 10.0;
    TS_ASSERT_LESS_THAN(vec[1], 0.0); // make sure it is negative, to trigger log binning
    TS_ASSERT(!standardValidator.isValid(vec).empty());
  }

  void testBinsInKiB() {
    // make sure we are calculating the number of bins in KiB correctly
    // 1. find the number of KiB remaining in memory.
    // 2. calculate the number of bins that would equal that value in *bytes* (i.e. numBins * sizeof(double))
    // 3. make sure that the validator passes, if the bytes of the bins are less than the available memory
    size_t memInKiB = Mantid::Kernel::MemoryStats().availMem();
    size_t numBins =
        memInKiB / sizeof(double); // the number bins (doubles) that would fit in memory, IF memory were in bytes
    std::vector<double> vec{1., 1., 1. + static_cast<double>(numBins)}; // make sure we have numBins bins
    TS_ASSERT(standardValidator.isValid(vec).empty());
  }

private:
  RebinParamsValidator standardValidator;
};
