#ifndef MANTID_ALGORITHMS_MOSTLIKELYMEANTEST_H_
#define MANTID_ALGORITHMS_MOSTLIKELYMEANTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/MostLikelyMean.h"

using Mantid::Algorithms::MostLikelyMean;

class MostLikelyMeanTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MostLikelyMeanTest *createSuite() { return new MostLikelyMeanTest(); }
  static void destroySuite(MostLikelyMeanTest *suite) { delete suite; }

  void test_Init() {
    MostLikelyMean alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {
    MostLikelyMean alg;
    std::vector<double> input = {1, 2, 3, 4, 5};
    alg.initialize();
    alg.setProperty("InputArray", input);
    alg.execute();
    const double mean = alg.getProperty("Output");
    TS_ASSERT_EQUALS(mean, 3);
  }
};

class MostLikelyMeanTestPerformance : public CxxTest::TestSuite {
public:
  static MostLikelyMeanTestPerformance *createSuite() {
    return new MostLikelyMeanTestPerformance();
  }
  static void destroySuite(MostLikelyMeanTestPerformance *suite) {
    delete suite;
  }

  void setUp() override {
    std::vector<double> input(10000);
    for (size_t i = 0; i < input.size(); ++i) {
      input[i] = double(sqrt(static_cast<double>(i)));
    }
    m_alg.initialize();
    m_alg.setProperty("InputArray", input);
  }

  void test_performance() { m_alg.execute(); }

private:
  MostLikelyMean m_alg;
};

#endif /* MANTID_ALGORITHMS_MOSTLIKELYMEANTEST_H_ */
