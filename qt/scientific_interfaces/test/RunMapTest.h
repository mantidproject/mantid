#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_RUNMAPTEST_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_RUNMAPTEST_H_

#include "../EnggDiffraction/RunMap.h"

#include <cxxtest/TestSuite.h>

using namespace MantidQt::CustomInterfaces;

class RunMapTest: public CxxTest::TestSuite{

public :
  void test_addedItemsExistInMap(){
    RunMap<3, std::string> runMap;
    TS_ASSERT_THROWS_NOTHING(runMap.add(123, 1, "Polly"));
    TS_ASSERT_THROWS_NOTHING(runMap.add(456, 2, "Morphism"));
    TS_ASSERT_THROWS(runMap.add(789, 4, "Al"), std::invalid_argument);

    TS_ASSERT(runMap.contains(123, 1));
    TS_ASSERT(runMap.contains(456, 2));
    TS_ASSERT(!runMap.contains(789, 4));
  }
  
  void test_addedItemsAreCorrect(){
    RunMap<3, std::string> runMap;
    TS_ASSERT_THROWS_NOTHING(runMap.add(123, 1, "Polly"));
    TS_ASSERT_THROWS_NOTHING(runMap.add(456, 2, "Morphism"));

    TS_ASSERT_EQUALS(runMap.get(123, 1), "Polly");
    TS_ASSERT_EQUALS(runMap.get(456, 2), "Morphism");
    
  }
  
};

#endif // MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_RUNMAPTEST_H_
