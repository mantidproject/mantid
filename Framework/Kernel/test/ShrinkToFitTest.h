#ifndef MANTID_SHRINKTOFITTEST_H_
#define MANTID_SHRINKTOFITTEST_H_

#include <cxxtest/TestSuite.h>
#include <string>
#include <vector>

/**
   \class ShrinkToFitTest
   Verify that the non-binding request shrink_to_fit() does reduce the
   container's capacity.
*/

class ShrinkToFitTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ShrinkToFitTest *createSuite() { return new ShrinkToFitTest(); }
  static void destroySuite(ShrinkToFitTest *suite) { delete suite; }

  void test_vector() {

    // verify that std::vector::shrink_to_fit() reduces the vector's capacity.
    std::size_t oldsize{1000};
    std::size_t newsize{100};
    std::vector<double> original(oldsize);
    TS_ASSERT_EQUALS(original.size(), oldsize);
    TS_ASSERT_LESS_THAN_EQUALS(oldsize, original.capacity());
    original.resize(100);
    TS_ASSERT_EQUALS(original.size(), newsize);
    std::size_t capacityAfterResize = original.capacity();
    original.shrink_to_fit();
    TS_ASSERT_EQUALS(original.size(), newsize);
    std::size_t capacityAfterShrinkToFit = original.capacity();
    TS_ASSERT_LESS_THAN(capacityAfterShrinkToFit, capacityAfterResize);

    // Use shrink-to-fit idiom and compare against shrink_to_fit()
    original.resize(1000);
    TS_ASSERT_EQUALS(original.size(), oldsize);
    original.resize(100);
    TS_ASSERT_EQUALS(original.size(), newsize);
    {
      std::vector<double> temp(original);
      std::swap(original, temp);
    }

    TS_ASSERT_LESS_THAN_EQUALS(capacityAfterShrinkToFit, original.capacity());
  }
};

#endif // MANTID_SHRINKTOFITTEST_H_
