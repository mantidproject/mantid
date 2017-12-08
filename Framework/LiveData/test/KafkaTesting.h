#ifndef MANTID_LIVEDATA_ISISKAFKATESTING_H_
#define MANTID_LIVEDATA_ISISKAFKATESTING_H_

#include "MantidKernel/DateAndTimeHelpers.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidLiveData/Kafka/IKafkaBroker.h"
#include "MantidLiveData/Kafka/IKafkaStreamSubscriber.h"
#include "MantidTypes/Core/DateAndTime.h"
#include <gmock/gmock.h>

GCC_DIAG_OFF(conversion)
#include "Kafka/private/Schema/ba57_run_info_generated.h"
#include "Kafka/private/Schema/df12_det_spec_map_generated.h"
#include "Kafka/private/Schema/ev42_events_generated.h"
#include "Kafka/private/Schema/is84_isis_events_generated.h"
GCC_DIAG_ON(conversion)

#include <ctime>

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
  MOCK_CONST_METHOD2(subscribe_, IKafkaStreamSubscriber_ptr(
                                     std::vector<std::string>,
                                     Mantid::LiveData::SubscribeAtOption));
  IKafkaStreamSubscriber_uptr
  subscribe(std::vector<std::string> s,
            Mantid::LiveData::SubscribeAtOption option) const override {
    return std::unique_ptr<Mantid::LiveData::IKafkaStreamSubscriber>(
        this->subscribe_(s, option));
  }
  MOCK_CONST_METHOD3(subscribe_, IKafkaStreamSubscriber_ptr(
                                     std::vector<std::string>, int64_t,
                                     Mantid::LiveData::SubscribeAtOption));
  IKafkaStreamSubscriber_uptr
  subscribe(std::vector<std::string> s, int64_t offset,
            Mantid::LiveData::SubscribeAtOption option) const override {
    return std::unique_ptr<Mantid::LiveData::IKafkaStreamSubscriber>(
        this->subscribe_(s, offset, option));
  }
  GCC_DIAG_ON_SUGGEST_OVERRIDE
};

// -----------------------------------------------------------------------------
// Fake stream to raise error to tests
// -----------------------------------------------------------------------------
class FakeExceptionThrowingStreamSubscriber
    : public Mantid::LiveData::IKafkaStreamSubscriber {
public:
  void subscribe() override {}
  void subscribe(int64_t offset) override { UNUSED_ARG(offset) }
  void consumeMessage(std::string *buffer, int64_t &offset, int32_t &partition,
                      std::string &topic) override {
    buffer->clear();
    UNUSED_ARG(offset);
    UNUSED_ARG(partition);
    UNUSED_ARG(topic);
    throw std::runtime_error("FakeExceptionThrowingStreamSubscriber");
  }
  std::unordered_map<std::string, std::vector<int64_t>>
  getOffsetsForTimestamp(int64_t timestamp) override {
    UNUSED_ARG(timestamp);
    return {
        std::pair<std::string, std::vector<int64_t>>("topic_name", {1, 2, 3})};
  }
};

// -----------------------------------------------------------------------------
// Fake stream to provide empty stream to client
// -----------------------------------------------------------------------------
class FakeEmptyStreamSubscriber
    : public Mantid::LiveData::IKafkaStreamSubscriber {
public:
  void subscribe() override {}
  void subscribe(int64_t offset) override { UNUSED_ARG(offset) }
  void consumeMessage(std::string *buffer, int64_t &offset, int32_t &partition,
                      std::string &topic) override {
    buffer->clear();
    UNUSED_ARG(offset);
    UNUSED_ARG(partition);
    UNUSED_ARG(topic);
  }
  std::unordered_map<std::string, std::vector<int64_t>>
  getOffsetsForTimestamp(int64_t timestamp) override {
    UNUSED_ARG(timestamp);
    return {
        std::pair<std::string, std::vector<int64_t>>("topic_name", {1, 2, 3})};
  }
};

// -----------------------------------------------------------------------------
// Fake ISIS event stream to provide event data
// -----------------------------------------------------------------------------
class FakeISISEventSubscriber
    : public Mantid::LiveData::IKafkaStreamSubscriber {
public:
  FakeISISEventSubscriber(int32_t nperiods)
      : m_nperiods(nperiods), m_nextPeriod(0) {}
  void subscribe() override {}
  void subscribe(int64_t offset) override { UNUSED_ARG(offset) }
  void consumeMessage(std::string *buffer, int64_t &offset, int32_t &partition,
                      std::string &topic) override {
    assert(buffer);

    flatbuffers::FlatBufferBuilder builder;
    std::vector<uint32_t> spec = {5, 4, 3, 2, 1, 2};
    std::vector<uint32_t> tof = {11000, 10000, 9000, 8000, 7000, 6000};

    uint64_t frameTime = 1;
    float protonCharge(0.5f);

    auto messageFlatbuf = CreateEventMessage(
        builder, builder.CreateString("KafkaTesting"), 0, frameTime,
        builder.CreateVector(tof), builder.CreateVector(spec),
        FacilityData_ISISData,
        CreateISISData(builder, static_cast<uint32_t>(m_nextPeriod),
                       RunState_RUNNING, protonCharge).Union());
    builder.Finish(messageFlatbuf);

    // Copy to provided buffer
    buffer->assign(reinterpret_cast<const char *>(builder.GetBufferPointer()),
                   builder.GetSize());
    m_nextPeriod = ((m_nextPeriod + 1) % m_nperiods);

    UNUSED_ARG(offset);
    UNUSED_ARG(partition);
    UNUSED_ARG(topic);
  }
  std::unordered_map<std::string, std::vector<int64_t>>
  getOffsetsForTimestamp(int64_t timestamp) override {
    UNUSED_ARG(timestamp);
    return {
        std::pair<std::string, std::vector<int64_t>>("topic_name", {1, 2, 3})};
  }

private:
  const int32_t m_nperiods;
  int32_t m_nextPeriod;
};

// -----------------------------------------------------------------------------
// Fake ISIS run data stream
// -----------------------------------------------------------------------------
class FakeISISRunInfoStreamSubscriber
    : public Mantid::LiveData::IKafkaStreamSubscriber {
public:
  FakeISISRunInfoStreamSubscriber(int32_t nperiods) : m_nperiods(nperiods) {}
  void subscribe() override {}
  void subscribe(int64_t offset) override { UNUSED_ARG(offset) }
  void consumeMessage(std::string *buffer, int64_t &offset, int32_t &partition,
                      std::string &topic) override {
    assert(buffer);

    // Convert date to time_t
    auto mantidTime = Mantid::Types::Core::DateAndTime(m_startTime);
    auto tmb = mantidTime.to_tm();
    uint64_t startTime = static_cast<uint64_t>(std::mktime(&tmb));

    // Serialize data with flatbuffers
    flatbuffers::FlatBufferBuilder builder;
    auto runInfo = CreateRunInfo(
        builder, InfoTypes_RunStart,
        CreateRunStart(builder, startTime, m_runNumber,
                       builder.CreateString(m_instName), m_nperiods).Union());
    builder.Finish(runInfo);
    // Copy to provided buffer
    buffer->assign(reinterpret_cast<const char *>(builder.GetBufferPointer()),
                   builder.GetSize());

    UNUSED_ARG(offset);
    UNUSED_ARG(partition);
    UNUSED_ARG(topic);
  }
  std::unordered_map<std::string, std::vector<int64_t>>
  getOffsetsForTimestamp(int64_t timestamp) override {
    UNUSED_ARG(timestamp);
    return {
        std::pair<std::string, std::vector<int64_t>>("topic_name", {1, 2, 3})};
  }

private:
  std::string m_startTime = "2016-08-31T12:07:42";
  int32_t m_runNumber = 1000;
  std::string m_instName = "HRPDTEST";
  int32_t m_nperiods = 1;
};

// -----------------------------------------------------------------------------
// Fake ISIS spectra-detector stream
// -----------------------------------------------------------------------------
class FakeISISSpDetStreamSubscriber
    : public Mantid::LiveData::IKafkaStreamSubscriber {
public:
  void subscribe() override {}
  void subscribe(int64_t offset) override { UNUSED_ARG(offset) }
  void consumeMessage(std::string *buffer, int64_t &offset, int32_t &partition,
                      std::string &topic) override {
    assert(buffer);

    // Serialize data with flatbuffers
    flatbuffers::FlatBufferBuilder builder;
    auto specVector = builder.CreateVector(m_spec);
    auto detIdsVector = builder.CreateVector(m_detid);
    auto spdet = CreateSpectraDetectorMapping(
        builder, specVector, detIdsVector, static_cast<int32_t>(m_spec.size()));
    builder.Finish(spdet);
    // Copy to provided buffer
    buffer->assign(reinterpret_cast<const char *>(builder.GetBufferPointer()),
                   builder.GetSize());

    UNUSED_ARG(offset);
    UNUSED_ARG(partition);
    UNUSED_ARG(topic);
  }
  std::unordered_map<std::string, std::vector<int64_t>>
  getOffsetsForTimestamp(int64_t timestamp) override {
    UNUSED_ARG(timestamp);
    return {
        std::pair<std::string, std::vector<int64_t>>("topic_name", {1, 2, 3})};
  }

private:
  std::vector<int32_t> m_spec = {1, 2, 3, 4, 5};
  // These match the detector numbers in HRPDTEST_Definition.xml
  std::vector<int32_t> m_detid = {1001, 1002, 1100, 901000, 10100};
};
} // namespace ISISKafkaTesting

#endif // MANTID_LIVEDATA_ISISKAFKAEVENTSTREAMDECODERTESTMOCKS_H_
