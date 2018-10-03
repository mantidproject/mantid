#ifndef MANTID_KERNEL_MDAXISVALIDATORTEST_H_
#define MANTID_KERNEL_MDAXISVALIDATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/MDAxisValidator.h"
#include "boost/make_shared.hpp"
#include "boost/shared_ptr.hpp"
#include <map>
#include <string>
#include <vector>

using Mantid::Kernel::MDAxisValidator;
using MDAxisValidator_sptr = boost::shared_ptr<MDAxisValidator>;

class MDAxisValidatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MDAxisValidatorTest *createSuite() {
    return new MDAxisValidatorTest();
  }
  static void destroySuite(MDAxisValidatorTest *suite) { delete suite; }

  /**
   * Tests the class on valid input - output of validate should be empty
   */
  void testMDAxisValidator_Valid() {
    MDAxisValidator_sptr checker = createValidator(4, 5, true);
    auto errors = checker->validate();
    TS_ASSERT_EQUALS(errors.size(), 0);
  }

  /**
   * Tests the error given for an empty axes vector (if check turned on)
   */
  void testMDAxisValidator_Empty() {
    MDAxisValidator_sptr checker = createValidator(0, 3, true);
    auto errors = checker->validate();
    TS_ASSERT_EQUALS(errors.size(), 1);
  }

  /**
   * Tests no error given for an empty axes vector (if check turned off)
   */
  void testMDAxisValidator_EmptyNoCheck() {
    MDAxisValidator_sptr checker = createValidator(0, 3, false);
    auto errors = checker->validate();
    TS_ASSERT_EQUALS(errors.size(), 0);
  }

  /**
   * Tests the error given when number of axes is greater than number of
   * dimensions in the workspace
   */
  void testMDAxisValidator_TooManyAxes() {
    MDAxisValidator_sptr checker = createValidator(5, 4, true);
    auto errors = checker->validate();
    TS_ASSERT_EQUALS(errors.size(), 1);
  }

  /**
   * Tests the error given when one of the axes given is out of the range of
   * dimensions in the workspace
   */
  void testMDAxisValidator_BadDimensionIndexed() {
    int nAxes = 3;
    std::vector<int> axes;
    for (int i = 0; i < nAxes - 1; i++) {
      axes.push_back(i);
    }
    axes.push_back(99); // a dimension out of the real dimension range
    MDAxisValidator checker(axes, nAxes, true);
    auto errors = checker.validate();
    TS_ASSERT_EQUALS(errors.size(), 1);
  }

private:
  /**
   * @brief Utility function to create an MDAxisValidator to test
   *
   * @param nAxes number of MD axes
   * @param nDimensions number of workspace dimensions
   * @param checkIfEmpty Whether the MDAxisValidator should check if the axes
   * vector is empty
   * @returns A shared pointer to an MDAxisValidator with the given properties
   */
  MDAxisValidator_sptr createValidator(int nAxes, int nDimensions,
                                       bool checkIfEmpty) const {
    std::vector<int> axes;
    for (int i = 0; i < nAxes; i++) {
      axes.push_back(i);
    }
    auto checker =
        boost::make_shared<MDAxisValidator>(axes, nDimensions, checkIfEmpty);
    return checker;
  }
};

#endif /* MANTID_KERNEL_MDAXISVALIDATORTEST_H_ */