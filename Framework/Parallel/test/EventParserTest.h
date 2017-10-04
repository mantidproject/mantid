#ifndef MANTID_PARALLEL_COLLECTIVESTEST_H_
#define MANTID_PARALLEL_COLLECTIVESTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidParallel/IO/EventParser.h"

#include <numeric>

using namespace Mantid;
using namespace Parallel::IO;
using Mantid::Types::DateAndTime;
using Mantid::Types::TofEvent;

namespace detail {
template <typename IndexType, typename TimeZeroType, typename TimeOffsetType>
class FakeParserDataGenerator {
public:
  FakeParserDataGenerator(size_t numBanks, size_t pixelsPerBank,
                          size_t numPulses) {
    generateTestData(numBanks, pixelsPerBank, numPulses);
  }

  ~FakeParserDataGenerator() {
    for (auto *list : m_event_lists)
      delete list;
  }

  const std::vector<int32_t> &bankOffsets() const { return m_bank_offsets; }

  const std::vector<IndexType> &eventIndex(size_t bank) const {
    return m_event_indices[bank];
  }

  const std::vector<TimeZeroType> &eventTimeZero() { return m_event_time_zero; }

  const std::vector<TimeOffsetType> &eventTimeOffset(size_t bank) const {
    return m_event_time_offsets[bank];
  }

  const std::vector<int32_t> &eventId(size_t bank) const {
    return m_event_ids[bank];
  }

  const std::vector<std::vector<TofEvent> *> &getTestEventLists() const {
    return m_event_lists;
  }

  LoadRange generateBasicRange(size_t bank) {
    LoadRange range;
    range.eventOffset = 0;
    range.eventCount = m_event_ids[bank].size();
    range.bankIndex = bank;

    return range;
  }

  std::vector<LoadRange> generateMPIRanges(size_t bank, int rank,
                                           int numRanks) {
    return std::vector<LoadRange>{};
  }

private:
  void generateTestData(const size_t numBanks, const size_t pixelsPerBank,
                        const size_t numPulses) {
    initOffsetsAndIndices(numBanks, numPulses);
    m_event_time_zero.resize(numPulses);
    auto numPixels = numBanks * pixelsPerBank;

    m_event_ids.resize(numBanks);
    m_event_time_offsets.resize(numBanks);
    std::generate_n(std::back_inserter(m_event_lists), numPixels,
                    []() { return new std::vector<TofEvent>(); });

    for (int pulse = 0; pulse < numPulses; ++pulse) {
      m_event_time_zero[pulse] = static_cast<TimeZeroType>(pulse * 100000);
      size_t pulseEventSize = 0;
      int bank = 0;
      for (size_t pixel = 0; pixel < numPixels; ++pixel) {
        auto eventSize = getRandEventSize(1, 10);
        pulseEventSize += eventSize;
        auto *list = m_event_lists[pixel];
        auto prev_end = list->size();
        std::generate_n(std::back_inserter(*list), eventSize, [this, pulse]() {
          return TofEvent(getRandomTimeOffset(100000),
                          m_event_time_zero[pulse]);
        });
        std::fill_n(std::back_inserter(m_event_ids[bank]), eventSize,
                    static_cast<IndexType>(m_bank_offsets[bank] + pixel));
        std::transform(list->cbegin() + prev_end, list->cend(),
                       std::back_inserter(m_event_time_offsets[bank]),
                       [this](const TofEvent &event) {
                         return static_cast<TimeOffsetType>(event.tof());
                       });
        if (((pixel + 1) % pixelsPerBank) == 0) {
          m_event_indices[bank][pulse] = static_cast<int32_t>(pulseEventSize);
          pulseEventSize = 0;
          bank++;
        }
      }
    }
    calculateEventIndicesPartialSums(numBanks);
  }

  void initOffsetsAndIndices(const size_t numBanks, const size_t numPulses) {
    m_bank_offsets.resize(numBanks);
    m_event_indices.resize(numBanks);
    for (int bank = 0; bank < numBanks; ++bank) {
      m_bank_offsets[bank] = (bank * 1000) + 1000;
      m_event_indices[bank].resize(numPulses);
    }
  }

  void calculateEventIndicesPartialSums(size_t numBanks) {
    for (size_t bank = 0; bank < numBanks; bank++) {
      auto &indices = m_event_indices[bank];
      std::partial_sum(indices.cbegin(), indices.cend(), indices.begin(),
                       std::plus<int32_t>());
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
  std::vector<std::vector<TofEvent> *> m_event_lists;
};
} // namespace detail

class EventParserTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static EventParserTest *createSuite() { return new EventParserTest(); }
  static void destroySuite(EventParserTest *suite) { delete suite; }

  void testConstruct() {
    std::vector<std::vector<int>> rankGroups;
    std::vector<int32_t> bankOffsets{1, 2, 3, 4};
    std::vector<std::vector<Mantid::Types::TofEvent> *> eventLists(4);

    TS_ASSERT_THROWS_NOTHING((EventParser<int64_t, int64_t, double>(
        rankGroups, bankOffsets, eventLists)));
  }

  void testConvertEventIDToGlobalSpectrumIndex() {
    std::vector<std::vector<int>> rankGroups;
    std::vector<int32_t> bankOffsets{1000};
    std::vector<std::vector<Mantid::Types::TofEvent> *> eventLists(10);

    EventParser<int64_t, int64_t, double> parser(rankGroups, bankOffsets,
                                                 eventLists);

    std::vector<int32_t> eventId{1001, 1002, 1004, 1004};
    auto eventIdCopy = eventId;
    parser.eventIdToGlobalSpectrumIndex(eventId.data(), eventId.size(), 0);

    TS_ASSERT_EQUALS(eventId[0], eventIdCopy[0] - bankOffsets[0]);
    TS_ASSERT_EQUALS(eventId[1], eventIdCopy[1] - bankOffsets[0]);
    TS_ASSERT_EQUALS(eventId[2], eventIdCopy[2] - bankOffsets[0]);
    TS_ASSERT_EQUALS(eventId[3], eventIdCopy[3] - bankOffsets[0]);
  }

  void testExtractEventsForRanks_1Rank() {
    std::vector<std::vector<Mantid::Types::TofEvent> *> eventLists(10);
    std::vector<int32_t> event_index{10, 20, 35, 60, 100};
    std::vector<int64_t> event_time_zero{0, 100000, 200000, 300000, 400000};
    std::vector<int32_t> event_id(100);
    std::vector<int32_t> event_time_offset(100);

    auto parser = createSimpleParser(eventLists, std::vector<int32_t>{0});
    parser.setPulseInformation(event_index, event_time_zero);

    size_t bankIndex = 0;
    size_t offset = 0;
    size_t numEvents = 100;

    parser.eventIdToGlobalSpectrumIndex(event_id.data() + offset, numEvents,
                                        bankIndex);
    const auto &specIndex = event_id;

    std::vector<std::vector<Event>> rankData(1);
    parser.extractEventsForRanks(rankData, specIndex.data(),
                                 event_time_offset.data() + offset,
                                 LoadRange{bankIndex, offset, numEvents});

    TS_ASSERT_EQUALS(rankData[0].size(), numEvents);

    size_t index = 0;
    auto test = [&event_time_zero, &index](const Event &item) {
      return item.tofEvent.pulseTime() == event_time_zero[index];
    };

    TS_ASSERT(
        std::all_of(rankData[0].cbegin(), rankData[0].cbegin() + 9, test));

    index = 1;
    TS_ASSERT(std::all_of(rankData[0].cbegin() + 10, rankData[0].cbegin() + 19,
                          test));
    index = 2;
    TS_ASSERT(std::all_of(rankData[0].cbegin() + 20, rankData[0].cbegin() + 34,
                          test));
    index = 3;
    TS_ASSERT(std::all_of(rankData[0].cbegin() + 35, rankData[0].cbegin() + 59,
                          test));
    index = 4;
    TS_ASSERT(std::all_of(rankData[0].cbegin() + 60, rankData[0].cbegin() + 100,
                          test));
  }

  void testParsingFailsNoEventIndexVector() {
    std::vector<std::vector<int>> rankGroups;
    std::vector<int32_t> bankOffsets(2);
    std::vector<std::vector<Mantid::Types::TofEvent> *> eventLists(4);

    EventParser<int32_t, int64_t, int32_t> parser(rankGroups, bankOffsets,
                                                  eventLists);

    TS_ASSERT_THROWS(parser.startParsing(nullptr, nullptr, LoadRange{0, 0, 0}),
                     std::runtime_error);
  }

  void testParsingFailNoEventTimeZeroVector() {
    std::vector<std::vector<int>> rankGroups;
    std::vector<int32_t> bankOffsets(2);
    std::vector<std::vector<Mantid::Types::TofEvent> *> eventLists(4);

    EventParser<int32_t, int64_t, int32_t> parser(rankGroups, bankOffsets,
                                                  eventLists);

    parser.setPulseInformation({10, 4, 4}, {});

    TS_ASSERT_THROWS(parser.startParsing(nullptr, nullptr, LoadRange{0, 0, 0}),
                     std::runtime_error);
  }

  void testParsing_1Rank_1Bank() {
    detail::FakeParserDataGenerator<int32_t, int64_t, int32_t> gen(1, 10, 2);
    const auto &testEventLists = gen.getTestEventLists();
    auto eventLists = prepareEventLists(testEventLists.size());
    auto parser = createSimpleParser(eventLists, gen.bankOffsets());

    parser.setPulseInformation(gen.eventIndex(0), gen.eventTimeZero());
    auto event_id = gen.eventId(0);
    auto event_time_offset = gen.eventTimeOffset(0);

    parser.startParsing(event_id.data(), event_time_offset.data(),
                        gen.generateBasicRange(0));

    parser.finalize();
    for (int i = 0; i < testEventLists.size(); i++)
      TS_ASSERT_EQUALS(*eventLists[i], *testEventLists[i]);

    cleanupEventLists(eventLists);
  }

  void testParsing_1Rank_2Banks() {
    int numBanks = 2;
    detail::FakeParserDataGenerator<int32_t, int64_t, int32_t> gen(numBanks, 10,
                                                                   7);
    const auto &testEventLists = gen.getTestEventLists();
    auto eventLists = prepareEventLists(testEventLists.size());
    auto parser = createSimpleParser(eventLists, gen.bankOffsets());

    for (int i = 0; i < numBanks; i++) {
      parser.setPulseInformation(gen.eventIndex(i), gen.eventTimeZero());
      auto event_id = gen.eventId(i);
      auto event_time_offset = gen.eventTimeOffset(i);

      parser.startParsing(event_id.data(), event_time_offset.data(),
                          gen.generateBasicRange(i));
      parser.wait();
    }
    parser.finalize();
    for (int i = 0; i < testEventLists.size(); i++) {
      TS_ASSERT_EQUALS(eventLists[i]->size(), testEventLists[i]->size())
      TS_ASSERT_EQUALS(*eventLists[i], *testEventLists[i]);
    }

    cleanupEventLists(eventLists);
  }

private:
  EventParser<int32_t, int64_t, int32_t>
  createSimpleParser(std::vector<std::vector<Types::TofEvent> *> &eventLists,
                     std::vector<int32_t> bankOffsets) {
    std::vector<std::vector<int>> rankGroups;

    return EventParser<int32_t, int64_t, int32_t>(rankGroups, bankOffsets,
                                                  eventLists);
  }

  std::vector<std::vector<Types::TofEvent> *>
  prepareEventLists(size_t numLists) {
    std::vector<std::vector<Mantid::Types::TofEvent> *> eventLists(numLists);
    for (int i = 0; i < numLists; i++)
      eventLists[i] = new std::vector<Types::TofEvent>();

    return eventLists;
  }

  void
  cleanupEventLists(std::vector<std::vector<Types::TofEvent> *> &eventLists) {
    for (auto *list : eventLists) {
      if (list != nullptr)
        delete list;
    }
  }
};

// TODO Add performance Tests
#endif /* MANTID_PARALLEL_COLLECTIVESTEST_H_ */
