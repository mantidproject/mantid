// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_PARALLEL_COLLECTIVESTEST_H_
#define MANTID_PARALLEL_COLLECTIVESTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidParallel/IO/EventParser.h"
#include <boost/make_shared.hpp>
#include <numeric>

using namespace Mantid;
using namespace Parallel::IO;
using Mantid::Types::Core::DateAndTime;
using Mantid::Types::Event::TofEvent;

namespace anonymous {
template <typename IndexType, typename TimeZeroType, typename TimeOffsetType>
class FakeParserDataGenerator {
public:
  FakeParserDataGenerator(size_t numBanks, size_t pixelsPerBank,
                          size_t numPulses, size_t maxEventsPerPixel = 100) {
    generateTestData(numBanks, pixelsPerBank, numPulses, maxEventsPerPixel);
  }

  const std::vector<int32_t> &bankOffsets() const { return m_bank_offsets; }

  const std::vector<IndexType> &eventIndex(size_t bank) const {
    return m_event_indices[bank];
  }

  const std::vector<TimeZeroType> &eventTimeZero() const {
    return m_event_time_zero;
  }

  const std::vector<TimeOffsetType> &eventTimeOffset(size_t bank) const {
    return m_event_time_offsets[bank];
  }

  const std::vector<int32_t> &eventId(size_t bank) const {
    return m_event_ids[bank];
  }

  Chunker::LoadRange generateBasicRange(size_t bank) {
    Chunker::LoadRange range;
    range.eventOffset = 0;
    range.eventCount = m_event_ids[bank].size();
    range.bankIndex = bank;

    return range;
  }

  boost::shared_ptr<EventParser<TimeOffsetType>> generateTestParser() {
    test_event_lists.clear();
    test_event_lists.resize(m_referenceEventLists.size());
    std::vector<std::vector<TofEvent> *> eventLists;
    for (auto &eventList : test_event_lists)
      eventLists.emplace_back(&eventList);
    Parallel::Communicator comm;
    return boost::make_shared<EventParser<TimeOffsetType>>(
        comm, std::vector<std::vector<int>>{}, m_bank_offsets, eventLists);
  }

  void checkEventLists() const {
    for (size_t i = 0; i < m_referenceEventLists.size(); ++i)
      TS_ASSERT_EQUALS(m_referenceEventLists[i], test_event_lists[i]);
  }

private:
  void generateTestData(const size_t numBanks, const size_t pixelsPerBank,
                        const size_t numPulses,
                        const size_t maxEventsPerPixel) {
    initOffsetsAndIndices(numBanks, numPulses);
    m_event_time_zero.resize(numPulses);
    auto numPixels = numBanks * pixelsPerBank;

    m_event_ids.resize(numBanks);
    m_event_time_offsets.resize(numBanks);
    m_referenceEventLists.clear();
    m_referenceEventLists.resize(numPixels);

    for (size_t bank = 0; bank < numBanks; ++bank) {
      size_t bankEventSize = 0;
      for (size_t pulse = 0; pulse < numPulses; ++pulse) {
        m_event_indices[bank][pulse] = static_cast<int32_t>(bankEventSize);
        m_event_time_zero[pulse] = static_cast<TimeZeroType>(pulse * 100000);
        for (size_t pixel = 0; pixel < pixelsPerBank; ++pixel) {
          size_t absolutePixel = pixel + bank * pixelsPerBank;
          auto eventSize = getRandEventSize(1, maxEventsPerPixel / numPulses);
          bankEventSize += eventSize;
          auto &list = m_referenceEventLists[absolutePixel];
          auto prev_end = list.size();
          std::generate_n(std::back_inserter(list), eventSize, [this, pulse]() {
            return TofEvent(getRandomTimeOffset(100000),
                            m_event_time_zero[pulse]);
          });
          std::fill_n(
              std::back_inserter(m_event_ids[bank]), eventSize,
              static_cast<IndexType>(m_bank_offsets[bank] + absolutePixel));
          std::transform(list.cbegin() + prev_end, list.cend(),
                         std::back_inserter(m_event_time_offsets[bank]),
                         [](const TofEvent &event) {
                           return static_cast<TimeOffsetType>(event.tof());
                         });
        }
      }
    }
  }

  void initOffsetsAndIndices(const size_t numBanks, const size_t numPulses) {
    m_bank_offsets.resize(numBanks);
    m_event_indices.resize(numBanks);
    for (size_t bank = 0; bank < numBanks; ++bank) {
      m_bank_offsets[bank] = static_cast<int32_t>(bank * 1000) + 1000;
      m_event_indices[bank].resize(numPulses);
    }
  }

  size_t getRandEventSize(size_t min = 1, size_t max = 1000) {
    return static_cast<size_t>(rand()) % (max - min) + min;
  }

  double getRandomTimeOffset(size_t pulseWidth) {
    return static_cast<double>(rand() % pulseWidth);
  }

  std::vector<int32_t> m_bank_offsets;
  std::vector<std::vector<int32_t>> m_event_ids;
  std::vector<std::vector<TimeOffsetType>> m_event_time_offsets;
  std::vector<std::vector<IndexType>> m_event_indices;
  std::vector<TimeZeroType> m_event_time_zero;
  std::vector<std::vector<TofEvent>> m_referenceEventLists;
  std::vector<std::vector<TofEvent>> test_event_lists;
};
} // namespace anonymous

class EventParserTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static EventParserTest *createSuite() { return new EventParserTest(); }
  static void destroySuite(EventParserTest *suite) { delete suite; }

  void testConstruct() {
    std::vector<std::vector<int>> rankGroups;
    std::vector<int32_t> bankOffsets{1, 2, 3, 4};
    std::vector<std::vector<TofEvent> *> eventLists(4);

    Parallel::Communicator comm;
    TS_ASSERT_THROWS_NOTHING(
        (EventParser<double>(comm, rankGroups, bankOffsets, eventLists)));
  }

  void testConvertEventIDToGlobalSpectrumIndex() {
    std::vector<int32_t> bankOffsets{1000};
    std::vector<int32_t> eventId{1001, 1002, 1004, 1004};
    auto eventIdCopy = eventId;
    detail::eventIdToGlobalSpectrumIndex(eventId.data(), eventId.size(),
                                         bankOffsets[0]);

    TS_ASSERT_EQUALS(eventId[0], eventIdCopy[0] - bankOffsets[0]);
    TS_ASSERT_EQUALS(eventId[1], eventIdCopy[1] - bankOffsets[0]);
    TS_ASSERT_EQUALS(eventId[2], eventIdCopy[2] - bankOffsets[0]);
    TS_ASSERT_EQUALS(eventId[3], eventIdCopy[3] - bankOffsets[0]);
  }

  void testExtractEventsFull() {
    anonymous::FakeParserDataGenerator<int32_t, int64_t, double> gen(1, 10, 5);
    auto event_id = gen.eventId(0);
    auto event_time_offset = gen.eventTimeOffset(0);
    auto range = gen.generateBasicRange(0);

    detail::eventIdToGlobalSpectrumIndex(event_id.data() + range.eventOffset,
                                         range.eventCount, 1000);
    std::vector<std::vector<EventParser<double>::Event>> rankData;
    // event_id now contains spectrum indices
    EventDataPartitioner<int32_t, int64_t, double> partitioner(
        1, {gen.eventIndex(0), gen.eventTimeZero(), "nanosecond", 0});
    partitioner.partition(rankData, event_id.data(),
                          event_time_offset.data() + range.eventOffset, range);

    TS_ASSERT(std::equal(rankData[0].cbegin(), rankData[0].cend(),
                         event_time_offset.cbegin(),
                         [](const EventParser<double>::Event &e,
                            const double tof) { return tof == e.tof; }));
    doTestRankData(rankData, gen, range);
  }

  void testExtractEventsPartial() {
    anonymous::FakeParserDataGenerator<int32_t, int64_t, double> gen(1, 10, 5);
    auto event_id = gen.eventId(0);
    auto event_time_offset = gen.eventTimeOffset(0);
    auto range = Chunker::LoadRange{0, 5, 100};

    detail::eventIdToGlobalSpectrumIndex(event_id.data() + range.eventOffset,
                                         range.eventCount, 1000);
    std::vector<std::vector<EventParser<double>::Event>> rankData;
    // event_id now contains spectrum indices
    EventDataPartitioner<int32_t, int64_t, double> partitioner(
        1, {gen.eventIndex(0), gen.eventTimeZero(), "nanosecond", 0});
    partitioner.partition(rankData, event_id.data(),
                          event_time_offset.data() + range.eventOffset, range);

    TS_ASSERT(
        std::equal(rankData[0].cbegin(), rankData[0].cend(),
                   event_time_offset.cbegin() + range.eventOffset,
                   [](const EventParser<double>::Event &e, const double tof) {
                     return static_cast<double>(tof) == e.tof;
                   }));
    doTestRankData(rankData, gen, range);
  }

  void testParsingFull_1Pulse_1Bank() {
    anonymous::FakeParserDataGenerator<int32_t, int32_t, double> gen(1, 10, 1);
    auto parser = gen.generateTestParser();
    parser->setEventDataPartitioner(
        std::make_unique<EventDataPartitioner<int32_t, int32_t, double>>(
            1, PulseTimeGenerator<int32_t, int32_t>{
                   gen.eventIndex(0), gen.eventTimeZero(), "nanosecond", 0}));
    parser->setEventTimeOffsetUnit("microsecond");
    auto event_id = gen.eventId(0);
    auto event_time_offset = gen.eventTimeOffset(0);

    parser->startAsync(event_id.data(), event_time_offset.data(),
                       gen.generateBasicRange(0));

    parser->wait();
    gen.checkEventLists();
  }

  void testParsingFull_1Rank_1Bank() {
    anonymous::FakeParserDataGenerator<int32_t, int64_t, float> gen(1, 10, 2);
    auto parser = gen.generateTestParser();
    parser->setEventDataPartitioner(
        std::make_unique<EventDataPartitioner<int32_t, int64_t, float>>(
            1, PulseTimeGenerator<int32_t, int64_t>{
                   gen.eventIndex(0), gen.eventTimeZero(), "nanosecond", 0}));
    parser->setEventTimeOffsetUnit("microsecond");
    auto event_id = gen.eventId(0);
    auto event_time_offset = gen.eventTimeOffset(0);

    parser->startAsync(event_id.data(), event_time_offset.data(),
                       gen.generateBasicRange(0));

    parser->wait();
    gen.checkEventLists();
  }

  void testParsingFull_1Rank_2Banks() {
    int numBanks = 2;
    anonymous::FakeParserDataGenerator<int32_t, int64_t, double> gen(numBanks,
                                                                     10, 7);
    auto parser = gen.generateTestParser();

    for (int i = 0; i < numBanks; i++) {
      parser->setEventDataPartitioner(
          std::make_unique<EventDataPartitioner<int32_t, int64_t, double>>(
              1, PulseTimeGenerator<int32_t, int64_t>{
                     gen.eventIndex(i), gen.eventTimeZero(), "nanosecond", 0}));
      parser->setEventTimeOffsetUnit("microsecond");
      auto event_id = gen.eventId(i);
      auto event_time_offset = gen.eventTimeOffset(i);

      parser->startAsync(event_id.data(), event_time_offset.data(),
                         gen.generateBasicRange(i));
      parser->wait();
    }
    gen.checkEventLists();
  }

  void testParsingFull_InParts_1Rank_1Bank() {
    anonymous::FakeParserDataGenerator<int32_t, int64_t, double> gen(1, 11, 7);
    auto parser = gen.generateTestParser();
    parser->setEventDataPartitioner(
        std::make_unique<EventDataPartitioner<int32_t, int64_t, double>>(
            1, PulseTimeGenerator<int32_t, int64_t>{
                   gen.eventIndex(0), gen.eventTimeZero(), "nanosecond", 0}));
    parser->setEventTimeOffsetUnit("microsecond");
    auto event_id = gen.eventId(0);
    auto event_time_offset = gen.eventTimeOffset(0);

    auto parts = 5;
    auto portion = event_id.size() / parts;

    for (int i = 0; i < parts; ++i) {
      auto offset = portion * i;

      // Needed so that no data is missed.
      if (i == (parts - 1))
        portion = event_id.size() - offset;

      Chunker::LoadRange range{0, offset, portion};
      parser->startAsync(event_id.data() + offset,
                         event_time_offset.data() + offset, range);
      parser->wait();
    }
    gen.checkEventLists();
  }

  void testParsingFull_InParts_1Rank_3Banks() {
    size_t numBanks = 3;
    anonymous::FakeParserDataGenerator<int32_t, int64_t, double> gen(3, 20, 7);
    auto parser = gen.generateTestParser();

    for (size_t bank = 0; bank < numBanks; bank++) {
      parser->setEventDataPartitioner(
          std::make_unique<EventDataPartitioner<int32_t, int64_t, double>>(
              1, PulseTimeGenerator<int32_t, int64_t>{gen.eventIndex(bank),
                                                      gen.eventTimeZero(),
                                                      "nanosecond", 0}));
      parser->setEventTimeOffsetUnit("microsecond");
      auto event_id = gen.eventId(bank);
      auto event_time_offset = gen.eventTimeOffset(bank);

      auto parts = 11;
      auto portion = event_id.size() / parts;

      for (int i = 0; i < parts; ++i) {
        auto offset = portion * i;

        // Needed so that no data is missed.
        if (i == (parts - 1))
          portion = event_id.size() - offset;

        Chunker::LoadRange range{bank, offset, portion};
        parser->startAsync(event_id.data() + offset,
                           event_time_offset.data() + offset, range);
        parser->wait();
      }
    }
    gen.checkEventLists();
  }

  void test_setEventTimeOffsetUnit() {
    std::vector<std::vector<int>> rankGroups;
    std::vector<int32_t> bankOffsets{0};
    std::vector<TofEvent> eventList;
    std::vector<std::vector<TofEvent> *> eventLists{&eventList};
    Parallel::Communicator comm;
    EventParser<double> parser(comm, rankGroups, bankOffsets, eventLists);
    PulseTimeGenerator<int32_t, int32_t> pulseTimes({0}, {0}, "nanosecond", 0);

    parser.setEventDataPartitioner(
        std::make_unique<EventDataPartitioner<int32_t, int32_t, double>>(
            1, std::move(pulseTimes)));

    int32_t event_id{0};
    const double event_time_offset{1.5};
    const Chunker::LoadRange range{0, 0, 1};

    parser.startAsync(&event_id, &event_time_offset, range);
    parser.wait();
    TS_ASSERT_EQUALS(eventList.size(), 1);
    TS_ASSERT_EQUALS(eventList[0].tof(), 0.0);

    parser.setEventTimeOffsetUnit("second");
    parser.startAsync(&event_id, &event_time_offset, range);
    parser.wait();
    TS_ASSERT_EQUALS(eventList.size(), 2);
    TS_ASSERT_EQUALS(eventList[1].tof(), 1.5e6);

    parser.setEventTimeOffsetUnit("microsecond");
    parser.startAsync(&event_id, &event_time_offset, range);
    parser.wait();
    TS_ASSERT_EQUALS(eventList.size(), 3);
    TS_ASSERT_EQUALS(eventList[2].tof(), 1.5);

    parser.setEventTimeOffsetUnit("nanosecond");
    parser.startAsync(&event_id, &event_time_offset, range);
    parser.wait();
    TS_ASSERT_EQUALS(eventList.size(), 4);
    TS_ASSERT_EQUALS(eventList[3].tof(), 1.5e-3);

    TS_ASSERT_THROWS_EQUALS(
        parser.setEventTimeOffsetUnit("millisecond"),
        const std::runtime_error &e, std::string(e.what()),
        "EventParser: unsupported unit `millisecond` for event_time_offset");
  }

private:
  template <typename T, typename IndexType, typename TimeZeroType,
            typename TimeOffsetType>
  void
  doTestRankData(const T &rankData,
                 anonymous::FakeParserDataGenerator<IndexType, TimeZeroType,
                                                    TimeOffsetType> &gen,
                 const Chunker::LoadRange &range) {
    PulseTimeGenerator<IndexType, TimeZeroType> pulseTimes(
        gen.eventIndex(0), gen.eventTimeZero(), "nanosecond", 0);
    pulseTimes.seek(range.eventOffset);
    TS_ASSERT_EQUALS(rankData[0].size(), range.eventCount);
    for (const auto &item : rankData[0]) {
      TS_ASSERT_EQUALS(item.pulseTime, pulseTimes.next());
    }
  }
};

class EventParserTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static EventParserTestPerformance *createSuite() {
    return new EventParserTestPerformance();
  }
  static void destroySuite(EventParserTestPerformance *suite) { delete suite; }

  EventParserTestPerformance() : gen(NUM_BANKS, 1000, 7, 100) {
    event_ids.resize(NUM_BANKS);
    event_time_offsets.resize(NUM_BANKS);
    // Copy here so this does not have to happen
    // in performance test
    for (size_t i = 0; i < NUM_BANKS; ++i) {
      event_time_offsets[i] = gen.eventTimeOffset(i);
      event_ids[i] = gen.eventId(i);
    }

    parser = gen.generateTestParser();
    for (auto &eventList : m_eventLists)
      m_eventListPtrs.emplace_back(&eventList);
  }

  void testCompletePerformance() {
    for (size_t i = 0; i < NUM_BANKS; ++i) {
      parser->setEventDataPartitioner(
          std::make_unique<EventDataPartitioner<int32_t, int64_t, double>>(
              1, PulseTimeGenerator<int32_t, int64_t>{
                     gen.eventIndex(i), gen.eventTimeZero(), "nanosecond", 0}));
      parser->setEventTimeOffsetUnit("microsecond");
      parser->startAsync(event_ids[i].data(), event_time_offsets[i].data(),
                         gen.generateBasicRange(i));
      parser->wait();
    }
  }

  void testExtractEventsPerformance() {
    for (size_t bank = 0; bank < NUM_BANKS; bank++) {
      EventDataPartitioner<int32_t, int64_t, double> partitioner(
          1, {gen.eventIndex(bank), gen.eventTimeZero(), "nanosecond", 0});
      partitioner.partition(rankData, event_ids[bank].data(),
                            event_time_offsets[bank].data(),
                            gen.generateBasicRange(bank));
    }
  }

private:
  const size_t NUM_BANKS = 7;
  std::vector<std::vector<int32_t>> event_ids;
  std::vector<std::vector<double>> event_time_offsets;
  anonymous::FakeParserDataGenerator<int32_t, int64_t, double> gen;
  boost::shared_ptr<EventParser<double>> parser;
  std::vector<std::vector<EventParser<double>::Event>> rankData;
  std::vector<std::vector<TofEvent>> m_eventLists{NUM_BANKS * 1000};
  std::vector<std::vector<TofEvent> *> m_eventListPtrs;
};
#endif /* MANTID_PARALLEL_COLLECTIVESTEST_H_ */
