// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_LIVEDATA_ISISKAFKATESTING_H_
#define MANTID_LIVEDATA_ISISKAFKATESTING_H_

#include "MantidKernel/DateAndTimeHelpers.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidLiveData/Kafka/IKafkaBroker.h"
#include "MantidLiveData/Kafka/IKafkaStreamSubscriber.h"
#include "MantidTypes/Core/DateAndTime.h"
#include <gmock/gmock.h>

GNU_DIAG_OFF("conversion")
#include "Kafka/private/Schema/ba57_run_info_generated.h"
#include "Kafka/private/Schema/df12_det_spec_map_generated.h"
#include "Kafka/private/Schema/ev42_events_generated.h"
#include "Kafka/private/Schema/f142_logdata_generated.h"
#include "Kafka/private/Schema/hs00_event_histogram_generated.h"
#include "Kafka/private/Schema/is84_isis_events_generated.h"
GNU_DIAG_ON("conversion")

#include <ctime>

namespace KafkaTesting {

// -----------------------------------------------------------------------------
// Mock broker to inject fake subscribers
// -----------------------------------------------------------------------------
class MockKafkaBroker : public Mantid::LiveData::IKafkaBroker {
public:
  using IKafkaStreamSubscriber_uptr =
      std::unique_ptr<Mantid::LiveData::IKafkaStreamSubscriber>;
  using IKafkaStreamSubscriber_ptr = Mantid::LiveData::IKafkaStreamSubscriber *;

  GNU_DIAG_OFF_SUGGEST_OVERRIDE
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
  GNU_DIAG_ON_SUGGEST_OVERRIDE
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

  std::unordered_map<std::string, std::vector<int64_t>>
  getCurrentOffsets() override {
    std::unordered_map<std::string, std::vector<int64_t>> offsets;
    return offsets;
  }

  void seek(const std::string &topic, uint32_t partition,
            int64_t offset) override {
    UNUSED_ARG(topic);
    UNUSED_ARG(partition);
    UNUSED_ARG(offset);
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

  std::unordered_map<std::string, std::vector<int64_t>>
  getCurrentOffsets() override {
    std::unordered_map<std::string, std::vector<int64_t>> offsets;
    return offsets;
  }

  void seek(const std::string &topic, uint32_t partition,
            int64_t offset) override {
    UNUSED_ARG(topic);
    UNUSED_ARG(partition);
    UNUSED_ARG(offset);
  }
};

namespace {
void fakeReceiveAnISISEventMessage(std::string *buffer, int32_t nextPeriod) {
  flatbuffers::FlatBufferBuilder builder;
  std::vector<uint32_t> spec = {5, 4, 3, 2, 1, 2};
  std::vector<uint32_t> tof = {11000, 10000, 9000, 8000, 7000, 6000};

  uint64_t frameTime = 1;
  float protonCharge(0.5f);

  auto messageFlatbuf = CreateEventMessage(
      builder, builder.CreateString("KafkaTesting"), 0, frameTime,
      builder.CreateVector(tof), builder.CreateVector(spec),
      FacilityData_ISISData,
      CreateISISData(builder, static_cast<uint32_t>(nextPeriod),
                     RunState_RUNNING, protonCharge)
          .Union());
  FinishEventMessageBuffer(builder, messageFlatbuf);

  // Copy to provided buffer
  buffer->assign(reinterpret_cast<const char *>(builder.GetBufferPointer()),
                 builder.GetSize());
}

void fakeReceiveAnEventMessage(std::string *buffer) {
  flatbuffers::FlatBufferBuilder builder;
  std::vector<uint32_t> spec = {5, 4, 3};
  std::vector<uint32_t> tof = {11000, 10000, 9000};
  uint64_t frameTime = 1;

  auto messageFlatbuf = CreateEventMessage(
      builder, builder.CreateString("KafkaTesting"), 0, frameTime,
      builder.CreateVector(tof), builder.CreateVector(spec));
  FinishEventMessageBuffer(builder, messageFlatbuf);

  // Copy to provided buffer
  buffer->assign(reinterpret_cast<const char *>(builder.GetBufferPointer()),
                 builder.GetSize());
}

void fakeReceiveHistoMessage(std::string *buffer) {
  flatbuffers::FlatBufferBuilder builder;
  // shape is binedges=2 nspectra=5
  std::vector<uint32_t> current_shape{3, 5};
  auto bin_edges = builder.CreateVector(std::vector<double>{0, 1, 2});
  auto xbins = HistoSchema::CreateArrayDouble(builder, bin_edges);
  auto bin_metadata = HistoSchema::CreateDimensionMetaData(
      builder, 3, builder.CreateString("TOF"), builder.CreateString("TOF"),
      HistoSchema::Array_ArrayDouble, xbins.Union());
  auto unit_metadata = HistoSchema::CreateDimensionMetaData(
      builder, 1, builder.CreateString("Counts"));

  auto dim_metadata = builder.CreateVector(
      std::vector<flatbuffers::Offset<HistoSchema::DimensionMetaData>>{
          bin_metadata, unit_metadata});

  // Data values are nspectra*nbins
  auto data_values = builder.CreateVector(
      std::vector<double>{100, 140, 210, 100, 110, 70, 5, 3, 20, 4});
  auto data = HistoSchema::CreateArrayDouble(builder, data_values);

  auto messageFlatBuf = HistoSchema::CreateEventHistogram(
      builder, builder.CreateString("KafkaTesting"), 0, dim_metadata, 0,
      builder.CreateVector(current_shape), 0, HistoSchema::Array_ArrayDouble,
      data.Union());

  FinishEventHistogramBuffer(builder, messageFlatBuf);
  buffer->assign(reinterpret_cast<const char *>(builder.GetBufferPointer()),
                 builder.GetSize());
}

void fakeReceiveASampleEnvMessage(std::string *buffer) {
  flatbuffers::FlatBufferBuilder builder;
  // Sample environment log
  auto logDataMessage = LogSchema::CreateLogData(
      builder, builder.CreateString("fake source"), LogSchema::Value_Int,
      LogSchema::CreateInt(builder, 42).Union(), 1495618188000000000L);
  FinishLogDataBuffer(builder, logDataMessage);

  // Copy to provided buffer
  buffer->assign(reinterpret_cast<const char *>(builder.GetBufferPointer()),
                 builder.GetSize());
}

void fakeReceiveARunStartMessage(std::string *buffer, int32_t runNumber,
                                 const std::string &startTime,
                                 const std::string &instName,
                                 int32_t nPeriods) {
  // Convert date to time_t
  auto mantidTime = Mantid::Types::Core::DateAndTime(startTime);
  auto startTimestamp =
      static_cast<uint64_t>(mantidTime.to_time_t() * 1000000000);

  flatbuffers::FlatBufferBuilder builder;
  auto runInfo =
      CreateRunInfo(builder, InfoTypes_RunStart,
                    CreateRunStart(builder, startTimestamp, runNumber,
                                   builder.CreateString(instName), nPeriods)
                        .Union());
  FinishRunInfoBuffer(builder, runInfo);
  // Copy to provided buffer
  buffer->assign(reinterpret_cast<const char *>(builder.GetBufferPointer()),
                 builder.GetSize());
}

void fakeReceiveARunStopMessage(std::string *buffer,
                                const std::string &stopTime) {
  // Convert date to time_t
  auto mantidTime = Mantid::Types::Core::DateAndTime(stopTime);
  auto stopTimestamp =
      static_cast<uint64_t>(mantidTime.to_time_t() * 1000000000);

  flatbuffers::FlatBufferBuilder builder;
  auto runInfo = CreateRunInfo(builder, InfoTypes_RunStop,
                               CreateRunStop(builder, stopTimestamp).Union());
  FinishRunInfoBuffer(builder, runInfo);
  // Copy to provided buffer
  buffer->assign(reinterpret_cast<const char *>(builder.GetBufferPointer()),
                 builder.GetSize());
}
} // namespace
// -----------------------------------------------------------------------------
// Fake ISIS event stream to provide event and sample environment data
// -----------------------------------------------------------------------------
class FakeISISEventSubscriber
    : public Mantid::LiveData::IKafkaStreamSubscriber {
public:
  explicit FakeISISEventSubscriber(int32_t nperiods)
      : m_nperiods(nperiods), m_nextPeriod(0) {}
  void subscribe() override {}
  void subscribe(int64_t offset) override { UNUSED_ARG(offset) }
  void consumeMessage(std::string *message, int64_t &offset, int32_t &partition,
                      std::string &topic) override {
    assert(message);

    fakeReceiveAnISISEventMessage(message, m_nextPeriod);
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

  std::unordered_map<std::string, std::vector<int64_t>>
  getCurrentOffsets() override {
    std::unordered_map<std::string, std::vector<int64_t>> offsets;
    return offsets;
  }

  void seek(const std::string &topic, uint32_t partition,
            int64_t offset) override {
    UNUSED_ARG(topic);
    UNUSED_ARG(partition);
    UNUSED_ARG(offset);
  }

private:
  const int32_t m_nperiods;
  int32_t m_nextPeriod;
};

// ---------------------------------------------------------------------------------------
// Fake non-institution-specific event stream to provide event and sample
// environment data
// ---------------------------------------------------------------------------------------
class FakeEventSubscriber : public Mantid::LiveData::IKafkaStreamSubscriber {
public:
  void subscribe() override {}
  void subscribe(int64_t offset) override { UNUSED_ARG(offset) }
  void consumeMessage(std::string *message, int64_t &offset, int32_t &partition,
                      std::string &topic) override {
    assert(message);

    switch (m_nextOffset) {
    case 0:
      fakeReceiveARunStartMessage(message, 1000, "2016-08-31T12:07:42",
                                  "HRPDTEST", 1);
      break;
    case 2:
      fakeReceiveARunStopMessage(message, m_stopTime);
      break;
    default:
      fakeReceiveAnEventMessage(message);
    }
    m_nextOffset++;

    UNUSED_ARG(offset);
    UNUSED_ARG(partition);
    UNUSED_ARG(topic);
  }

  std::unordered_map<std::string, std::vector<int64_t>>
  getOffsetsForTimestamp(int64_t timestamp) override {
    UNUSED_ARG(timestamp);
    return {std::pair<std::string, std::vector<int64_t>>(m_topicName, {1})};
  }

  std::unordered_map<std::string, std::vector<int64_t>>
  getCurrentOffsets() override {
    std::unordered_map<std::string, std::vector<int64_t>> offsets;
    return {std::pair<std::string, std::vector<int64_t>>(m_topicName, {1})};
  }

  void seek(const std::string &topic, uint32_t partition,
            int64_t offset) override {
    UNUSED_ARG(topic);
    UNUSED_ARG(partition);
    UNUSED_ARG(offset);
  }

private:
  std::string m_topicName = "topic_name";
  int m_nextOffset = 0;
  std::string m_stopTime = "2016-08-31T12:07:52";
};

// ---------------------------------------------------------------------------------------
// Fake non-institution-specific histo stream to provide histogram and sample
// environment data
// ---------------------------------------------------------------------------------------
class FakeHistoSubscriber : public Mantid::LiveData::IKafkaStreamSubscriber {
public:
  void subscribe() override {}
  void subscribe(int64_t offset) override { UNUSED_ARG(offset) }
  void consumeMessage(std::string *message, int64_t &offset, int32_t &partition,
                      std::string &topic) override {
    assert(message);

    switch (m_nextOffset) {
    case 0:
      fakeReceiveARunStartMessage(message, 1000, "2016-08-31T12:07:42",
                                  "HRPDTEST", 1);
      break;
    case 2:
      fakeReceiveARunStopMessage(message, m_stopTime);
      break;
    default:
      fakeReceiveHistoMessage(message);
    }
    m_nextOffset++;

    UNUSED_ARG(offset);
    UNUSED_ARG(partition);
    UNUSED_ARG(topic);
  }

  std::unordered_map<std::string, std::vector<int64_t>>
  getOffsetsForTimestamp(int64_t timestamp) override {
    UNUSED_ARG(timestamp);
    return {std::pair<std::string, std::vector<int64_t>>(m_topicName, {1})};
  }

  std::unordered_map<std::string, std::vector<int64_t>>
  getCurrentOffsets() override {
    std::unordered_map<std::string, std::vector<int64_t>> offsets;
    return {std::pair<std::string, std::vector<int64_t>>(m_topicName, {1})};
  }

  void seek(const std::string &topic, uint32_t partition,
            int64_t offset) override {
    UNUSED_ARG(topic);
    UNUSED_ARG(partition);
    UNUSED_ARG(offset);
  }

private:
  std::string m_topicName = "topic_name";
  int m_nextOffset = 0;
  std::string m_stopTime = "2016-08-31T12:07:52";
};

// -----------------------------------------------------------------------------
// Fake event stream to provide sample environment data
// -----------------------------------------------------------------------------
class FakeSampleEnvironmentSubscriber
    : public Mantid::LiveData::IKafkaStreamSubscriber {
public:
  void subscribe() override {}
  void subscribe(int64_t offset) override { UNUSED_ARG(offset) }
  void consumeMessage(std::string *message, int64_t &offset, int32_t &partition,
                      std::string &topic) override {
    assert(message);

    fakeReceiveASampleEnvMessage(message);

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

  std::unordered_map<std::string, std::vector<int64_t>>
  getCurrentOffsets() override {
    std::unordered_map<std::string, std::vector<int64_t>> offsets;
    return offsets;
  }

  void seek(const std::string &topic, uint32_t partition,
            int64_t offset) override {
    UNUSED_ARG(topic);
    UNUSED_ARG(partition);
    UNUSED_ARG(offset);
  }
};

// -----------------------------------------------------------------------------
// Fake run data stream
// -----------------------------------------------------------------------------
class FakeRunInfoStreamSubscriber
    : public Mantid::LiveData::IKafkaStreamSubscriber {
public:
  explicit FakeRunInfoStreamSubscriber(int32_t nperiods)
      : m_nperiods(nperiods) {}
  void subscribe() override {}
  void subscribe(int64_t offset) override { UNUSED_ARG(offset) }
  void consumeMessage(std::string *buffer, int64_t &offset, int32_t &partition,
                      std::string &topic) override {
    assert(buffer);

    fakeReceiveARunStartMessage(buffer, m_runNumber, m_startTime, m_instName,
                                m_nperiods);

    m_runNumber++;
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

  std::unordered_map<std::string, std::vector<int64_t>>
  getCurrentOffsets() override {
    std::unordered_map<std::string, std::vector<int64_t>> offsets;
    return offsets;
  }

  void seek(const std::string &topic, uint32_t partition,
            int64_t offset) override {
    UNUSED_ARG(topic);
    UNUSED_ARG(partition);
    UNUSED_ARG(offset);
  }

private:
  std::string m_startTime = "2016-08-31T12:07:42";
  int32_t m_runNumber = 1000;
  std::string m_instName = "HRPDTEST";
  int32_t m_nperiods = 1;
};

// -----------------------------------------------------------------------------
// Fake run data stream with incrementing number of periods
// -----------------------------------------------------------------------------
class FakeRunInfoStreamSubscriberVaryingNPeriods
    : public Mantid::LiveData::IKafkaStreamSubscriber {
public:
  void subscribe() override {}
  void subscribe(int64_t offset) override { UNUSED_ARG(offset) }
  void consumeMessage(std::string *buffer, int64_t &offset, int32_t &partition,
                      std::string &topic) override {
    assert(buffer);

    fakeReceiveARunStartMessage(buffer, m_runNumber, m_startTime, m_instName,
                                m_nperiods);

    m_nperiods++;
    m_runNumber++;
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

  std::unordered_map<std::string, std::vector<int64_t>>
  getCurrentOffsets() override {
    std::unordered_map<std::string, std::vector<int64_t>> offsets;
    return offsets;
  }

  void seek(const std::string &topic, uint32_t partition,
            int64_t offset) override {
    UNUSED_ARG(topic);
    UNUSED_ARG(partition);
    UNUSED_ARG(offset);
  }

private:
  std::string m_startTime = "2016-08-31T12:07:42";
  int32_t m_runNumber = 1000;
  std::string m_instName = "HRPDTEST";
  int32_t m_nperiods = 1;
};

// -----------------------------------------------------------------------------
// Varing period data stream with run and event messages
// -----------------------------------------------------------------------------
class FakeVariablePeriodSubscriber
    : public Mantid::LiveData::IKafkaStreamSubscriber {
public:
  explicit FakeVariablePeriodSubscriber(uint32_t startOffset)
      : m_nextOffset(startOffset) {}
  void subscribe() override {}
  void subscribe(int64_t offset) override { UNUSED_ARG(offset) }
  void consumeMessage(std::string *buffer, int64_t &offset, int32_t &partition,
                      std::string &topic) override {
    assert(buffer);

    // Return messages in this order:
    // Run start (with 1 period)
    // Event data
    // Run start (with 2 periods)
    // Run stop
    // Event data
    // Event data (data for 2nd period)
    // Run stop

    switch (m_nextOffset) {
    case 0:
      fakeReceiveARunStartMessage(buffer, 1000, m_startTime, m_instName,
                                  m_nperiods);
      break;
    case 2:
      fakeReceiveARunStartMessage(buffer, 1001, m_startTime, m_instName, 2);
      break;
    case 3:
      fakeReceiveARunStopMessage(buffer, m_stopTime);
      break;
    case 5:
      fakeReceiveAnISISEventMessage(buffer, 1);
      break;
    case 6:
      fakeReceiveARunStopMessage(buffer, m_stopTime);
      break;
    default:
      fakeReceiveAnISISEventMessage(buffer, 0);
    }
    topic = "topic_name";
    offset = m_nextOffset;
    partition = 0;
    m_nextOffset++;
  }

  std::unordered_map<std::string, std::vector<int64_t>>
  getOffsetsForTimestamp(int64_t timestamp) override {
    UNUSED_ARG(timestamp);
    return {std::pair<std::string, std::vector<int64_t>>(m_topicName, {2})};
  }

  std::unordered_map<std::string, std::vector<int64_t>>
  getCurrentOffsets() override {
    return {std::pair<std::string, std::vector<int64_t>>(m_topicName, {2})};
  }

  void seek(const std::string &topic, uint32_t partition,
            int64_t offset) override {
    UNUSED_ARG(topic);
    UNUSED_ARG(partition);
    UNUSED_ARG(offset);
  }

private:
  const std::string m_topicName = "topic_name";
  uint32_t m_nextOffset;
  std::string m_startTime = "2016-08-31T12:07:42";
  std::string m_stopTime = "2016-08-31T12:07:52";
  const std::string m_instName = "HRPDTEST";
  int32_t m_nperiods = 1;
};

// -----------------------------------------------------------------------------
// Fake data stream with run and event messages
// -----------------------------------------------------------------------------
class FakeDataStreamSubscriber
    : public Mantid::LiveData::IKafkaStreamSubscriber {
public:
  explicit FakeDataStreamSubscriber(int64_t stopOffset)
      : m_stopOffset(stopOffset) {}
  void subscribe() override {}
  void subscribe(int64_t offset) override { UNUSED_ARG(offset) }
  void consumeMessage(std::string *buffer, int64_t &offset, int32_t &partition,
                      std::string &topic) override {
    assert(buffer);

    // Return messages in this order:
    // Run start
    // Event data
    // Run stop
    // Event data
    // Run start
    // Event data... ad infinitum

    switch (m_nextOffset) {
    case 0:
      fakeReceiveARunStartMessage(buffer, 1000, m_startTime, m_instName,
                                  m_nperiods);
      break;
    case 2:
      fakeReceiveARunStopMessage(buffer, m_stopTime);
      break;
    case 4:
      fakeReceiveARunStartMessage(buffer, 1000, m_startTime, m_instName,
                                  m_nperiods);
      break;
    default:
      fakeReceiveAnISISEventMessage(buffer, 0);
    }
    topic = "topic_name";
    offset = m_nextOffset;
    partition = 0;
    m_nextOffset++;
  }

  std::unordered_map<std::string, std::vector<int64_t>>
  getOffsetsForTimestamp(int64_t timestamp) override {
    UNUSED_ARG(timestamp);
    return {std::pair<std::string, std::vector<int64_t>>(m_topicName,
                                                         {m_stopOffset})};
  }

  std::unordered_map<std::string, std::vector<int64_t>>
  getCurrentOffsets() override {
    return {std::pair<std::string, std::vector<int64_t>>(m_topicName,
                                                         {m_nextOffset - 1})};
  }

  void seek(const std::string &topic, uint32_t partition,
            int64_t offset) override {
    UNUSED_ARG(topic);
    UNUSED_ARG(partition);
    UNUSED_ARG(offset);
  }

private:
  const std::string m_topicName = "topic_name";
  uint32_t m_nextOffset = 0;
  std::string m_startTime = "2016-08-31T12:07:42";
  std::string m_stopTime = "2016-08-31T12:07:52";
  const std::string m_instName = "HRPDTEST";
  int32_t m_nperiods = 1;
  int64_t m_stopOffset;
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
    FinishSpectraDetectorMappingBuffer(builder, spdet);
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

  std::unordered_map<std::string, std::vector<int64_t>>
  getCurrentOffsets() override {
    std::unordered_map<std::string, std::vector<int64_t>> offsets;
    return offsets;
  }

  void seek(const std::string &topic, uint32_t partition,
            int64_t offset) override {
    UNUSED_ARG(topic);
    UNUSED_ARG(partition);
    UNUSED_ARG(offset);
  }

private:
  std::vector<int32_t> m_spec = {1, 2, 3, 4, 5};
  // These match the detector numbers in HRPDTEST_Definition.xml
  std::vector<int32_t> m_detid = {1001, 1002, 1100, 901000, 10100};
};
} // namespace KafkaTesting

#endif // MANTID_LIVEDATA_ISISKAFKAEVENTSTREAMDECODERTESTMOCKS_H_
