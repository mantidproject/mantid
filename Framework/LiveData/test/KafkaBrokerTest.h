#ifndef MANTID_LIVEDATA_KAFKABROKERTEST_H_
#define MANTID_LIVEDATA_KAFKABROKERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidLiveData/Kafka/KafkaBroker.h"

using Mantid::LiveData::KafkaBroker;

class KafkaBrokerTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static KafkaBrokerTest *createSuite() { return new KafkaBrokerTest(); }
  static void destroySuite( KafkaBrokerTest *suite ) { delete suite; }


  void test_Something()
  {
    TS_FAIL( "You forgot to write a test!");
  }


};


#endif /* MANTID_LIVEDATA_KAFKABROKERTEST_H_ */