#ifndef MANTID_LIVEDATA_ISISKAFKATESTING_H_
#define MANTID_LIVEDATA_ISISKAFKATESTING_H_

#include "MantidLiveData/Kafka/IKafkaBroker.h"
#include "MantidKernel/WarningSuppressions.h"
#include <gmock/gmock.h>

namespace ISISKafkaTesting {

// -----------------------------------------------------------------------------
// Mock broker to inject fake subscribers
// -----------------------------------------------------------------------------
class MockKafkaBroker : public Mantid::LiveData::IKafkaBroker {
public:
  using IKafkaStreamSubscriber_uptr =
      std::unique_ptr<Mantid::LiveData::IKafkaStreamSubscriber>;
  using IKafkaStreamSubscriber_ptr = Mantid::LiveData::IKafkaStreamSubscriber *;

  GCC_DIAG_OFF_SUGGEST_OVERRIDE
  // GMock cannot mock non-copyable return types so we resort to a small
  // adapter method. Users have to use EXPECT_CALL(subscribe_) instead
  MOCK_CONST_METHOD1(subscribe_,
                     IKafkaStreamSubscriber_ptr(const std::string &));
  IKafkaStreamSubscriber_uptr subscribe(const std::string &s) const override {
    return std::unique_ptr<Mantid::LiveData::IKafkaStreamSubscriber>(
        this->subscribe_(s));
  }
  GCC_DIAG_ON_SUGGEST_OVERRIDE
};

// -----------------------------------------------------------------------------
// Fake stream to provide empty data to tests
// -----------------------------------------------------------------------------
class FakeEmptyStreamSubscriber
    : public Mantid::LiveData::IKafkaStreamSubscriber {
public:
  void subscribe() override {}
  bool consumeMessage(std::string *) override { return false; }
};

// -----------------------------------------------------------------------------
// Fake ISIS event stream to provide event data
// -----------------------------------------------------------------------------
class FakeISISSinglePeriodStreamSubscriber
    : public Mantid::LiveData::IKafkaStreamSubscriber {
public:
  void subscribe() override {}
  bool consumeMessage(std::string *) override { return true; }
};

// -----------------------------------------------------------------------------
// Fake ISIS run data stream
// -----------------------------------------------------------------------------
class FakeISISRunInfoStreamSubscriber
    : public Mantid::LiveData::IKafkaStreamSubscriber {
public:
  void subscribe() override {}
  bool consumeMessage(std::string *) override { return true; }
};

// -----------------------------------------------------------------------------
// Fake ISIS spectra-detector stream
// -----------------------------------------------------------------------------
class FakeISISSpDetStreamSubscriber
    : public Mantid::LiveData::IKafkaStreamSubscriber {
public:
  void subscribe() override {}
  bool consumeMessage(std::string *) override { return true; }
};
}

#endif // MANTID_LIVEDATA_ISISKAFKAEVENTSTREAMDECODERTESTMOCKS_H_
