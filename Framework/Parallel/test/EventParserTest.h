#ifndef MANTID_PARALLEL_COLLECTIVESTEST_H_
#define MANTID_PARALLEL_COLLECTIVESTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidParallel/IO/EventParser.h"

#include <numeric>

using namespace Mantid;
using namespace Parallel::IO;
using Mantid::Types::Core::DateAndTime;
using Mantid::Types::Event::TofEvent;

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

  const std::vector<std::vector<TofEvent> *> &eventLists() const {
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

  static EventParser<IndexType, TimeZeroType, TimeOffsetType>
  createSimpleParser(std::vector<std::vector<Types::TofEvent> *> &eventLists,
                     std::vector<int32_t> bankOffsets) {
    std::vector<std::vector<int>> rankGroups;

    return EventParser<IndexType, TimeZeroType, TimeOffsetType>(
        rankGroups, bankOffsets, eventLists);
  }

  static std::vector<std::vector<Types::TofEvent> *>
  prepareEventLists(size_t numLists) {
    std::vector<std::vector<Mantid::Types::TofEvent> *> eventLists(numLists);
    for (int i = 0; i < numLists; i++)
      eventLists[i] = new std::vector<Types::TofEvent>();

    return eventLists;
  }

  static void
  cleanupEventLists(std::vector<std::vector<Types::TofEvent> *> &eventLists) {
    for (auto *list : eventLists) {
      if (list != nullptr)
        delete list;
    }
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
    std::vector<std::vector<TofEvent> *> eventLists(4);

    TS_ASSERT_THROWS_NOTHING((EventParser<int64_t, int64_t, double>(
        rankGroups, bankOffsets, eventLists)));
  }

  void testConvertEventIDToGlobalSpectrumIndex() {
    std::vector<std::vector<int>> rankGroups;
    std::vector<int32_t> bankOffsets{1000};
    std::vector<std::vector<TofEvent> *> eventLists(10);

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

  void testFindFirstAndLastPulses() {
    std::vector<std::vector<int>> rankGroups;
    std::vector<int32_t> bankOffsets{1000};
    std::vector<std::vector<TofEvent> *> eventLists(10);

    EventParser<int64_t, int64_t, double> parser(rankGroups, bankOffsets,
                                                 eventLists);

    std::vector<int64_t> event_index{10, 20, 40, 60, 100, 150, 210};
    size_t curr = 0;
    auto res = parser.findStartAndEndPulses(event_index, 0, 50, curr);
    TS_ASSERT_EQUALS(res.first, 0);
    TS_ASSERT_EQUALS(res.second, 3);
    TS_ASSERT_EQUALS(curr, 3);

    curr = 0; // reset "current position" for new set of indices
    res = parser.findStartAndEndPulses(event_index, 30, 50, curr);
    TS_ASSERT_EQUALS(res.first, 1);
    TS_ASSERT_EQUALS(res.second, 4);
    TS_ASSERT_EQUALS(curr, 4);

    // instead of resetting curr allow search to start from this position
    res = parser.findStartAndEndPulses(event_index, 105, 98, curr);
    TS_ASSERT_EQUALS(res.first, 4);
    TS_ASSERT_EQUALS(res.second, 6);
    TS_ASSERT_EQUALS(curr, 6);

    // starting from an offset which may be lower than curr
    res = parser.findStartAndEndPulses(event_index, 0, 100, curr);
    TS_ASSERT_EQUALS(res.first, 0);
    TS_ASSERT_EQUALS(res.second, 4);
    TS_ASSERT_EQUALS(curr, 4);
  }

  void testExtractEventsFull() {
    detail::FakeParserDataGenerator<int32_t, int64_t, int64_t> gen(1, 10, 5);
    auto eventLists = gen.prepareEventLists(gen.eventLists().size());
    auto parser = gen.createSimpleParser(eventLists, gen.bankOffsets());
    parser.setPulseInformation(gen.eventIndex(0), gen.eventTimeZero());
    auto event_id = gen.eventId(0);
    auto event_time_offset = gen.eventTimeOffset(0);
    auto range = gen.generateBasicRange(0);

    parser.eventIdToGlobalSpectrumIndex(event_id.data() + range.eventOffset,
                                        range.eventCount, range.bankIndex);
    std::vector<std::vector<Event>> rankData(1);
    // event_id now contains spectrum indices
    parser.extractEventsForRanks(rankData, event_id.data(),
                                 event_time_offset.data() + range.eventOffset,
                                 range);
    TS_ASSERT(std::equal(rankData[0].cbegin(), rankData[0].cend(),
                         event_time_offset.cbegin(),
                         [](const Event &e, const int64_t tof) {
                           return static_cast<double>(tof) == e.tofEvent.tof();
                         }));
    doTestRankData(rankData, parser, gen, range);
    gen.cleanupEventLists(eventLists);
  }

  void testExtractEventsPartial() {
    detail::FakeParserDataGenerator<int32_t, int64_t, int64_t> gen(1, 10, 5);
    auto eventLists = gen.prepareEventLists(gen.eventLists().size());
    auto parser = gen.createSimpleParser(eventLists, gen.bankOffsets());
    parser.setPulseInformation(gen.eventIndex(0), gen.eventTimeZero());
    auto event_id = gen.eventId(0);
    auto event_time_offset = gen.eventTimeOffset(0);
    auto range = LoadRange{0, 5, 100};

    parser.eventIdToGlobalSpectrumIndex(event_id.data() + range.eventOffset,
                                        range.eventCount, range.bankIndex);
    std::vector<std::vector<Event>> rankData(1);
    // event_id now contains spectrum indices
    parser.extractEventsForRanks(rankData, event_id.data(),
                                 event_time_offset.data() + range.eventOffset,
                                 range);
    TS_ASSERT(std::equal(rankData[0].cbegin(), rankData[0].cend(),
                         event_time_offset.cbegin() + range.eventOffset,
                         [](const Event &e, const int64_t tof) {
                           return static_cast<double>(tof) == e.tofEvent.tof();
                         }));
    doTestRankData(rankData, parser, gen, range);
    gen.cleanupEventLists(eventLists);
  }

  void testParsingFailsNoEventIndexVector() {
    std::vector<std::vector<int>> rankGroups;
    std::vector<int32_t> bankOffsets(2);
    std::vector<std::vector<TofEvent> *> eventLists(4);

    EventParser<int32_t, int64_t, int32_t> parser(rankGroups, bankOffsets,
                                                  eventLists);

    TS_ASSERT_THROWS(parser.startParsing(nullptr, nullptr, LoadRange{0, 0, 0}),
                     std::runtime_error);
  }

  void testParsingFailNoEventTimeZeroVector() {
    std::vector<std::vector<int>> rankGroups;
    std::vector<int32_t> bankOffsets(2);
    std::vector<std::vector<TofEvent> *> eventLists(4);

    EventParser<int32_t, int64_t, int32_t> parser(rankGroups, bankOffsets,
                                                  eventLists);

    parser.setPulseInformation({10, 4, 4}, {});

    TS_ASSERT_THROWS(parser.startParsing(nullptr, nullptr, LoadRange{0, 0, 0}),
                     std::runtime_error);
  }

  void testParsingFull_1Pulse_1Bank() {
    detail::FakeParserDataGenerator<int32_t, int32_t, double> gen(1, 10, 1);
    auto eventLists = gen.prepareEventLists(gen.eventLists().size());
    auto parser = gen.createSimpleParser(eventLists, gen.bankOffsets());
    parser.setPulseInformation(gen.eventIndex(0), gen.eventTimeZero());
    auto event_id = gen.eventId(0);
    auto event_time_offset = gen.eventTimeOffset(0);

    parser.startParsing(event_id.data(), event_time_offset.data(),
                        gen.generateBasicRange(0));

    parser.finalize();
    for (int i = 0; i < gen.eventLists().size(); ++i)
      TS_ASSERT_EQUALS(*eventLists[i], *gen.eventLists()[i]);

    gen.cleanupEventLists(eventLists);
  }

  void testParsingFull_1Rank_1Bank() {
    detail::FakeParserDataGenerator<int32_t, int64_t, int32_t> gen(1, 10, 2);
    auto eventLists = gen.prepareEventLists(gen.eventLists().size());
    auto parser = gen.createSimpleParser(eventLists, gen.bankOffsets());
    parser.setPulseInformation(gen.eventIndex(0), gen.eventTimeZero());
    auto event_id = gen.eventId(0);
    auto event_time_offset = gen.eventTimeOffset(0);

    parser.startParsing(event_id.data(), event_time_offset.data(),
                        gen.generateBasicRange(0));

    parser.finalize();
    for (int i = 0; i < gen.eventLists().size(); ++i)
      TS_ASSERT_EQUALS(*eventLists[i], *gen.eventLists()[i]);

    gen.cleanupEventLists(eventLists);
  }

  void testParsingFull_1Rank_2Banks() {
    int numBanks = 2;
    detail::FakeParserDataGenerator<int32_t, int64_t, double> gen(numBanks, 10,
                                                                  7);
    auto eventLists = gen.prepareEventLists(gen.eventLists().size());
    auto parser = gen.createSimpleParser(eventLists, gen.bankOffsets());

    for (int i = 0; i < numBanks; i++) {
      parser.setPulseInformation(gen.eventIndex(i), gen.eventTimeZero());
      auto event_id = gen.eventId(i);
      auto event_time_offset = gen.eventTimeOffset(i);

      parser.startParsing(event_id.data(), event_time_offset.data(),
                          gen.generateBasicRange(i));
      parser.wait();
    }
    parser.finalize();

    for (int i = 0; i < gen.eventLists().size(); ++i)
      TS_ASSERT_EQUALS(*eventLists[i], *gen.eventLists()[i]);
    gen.cleanupEventLists(eventLists);
  }

  void testParsingFull_InParts_1Rank_1Bank() {
    detail::FakeParserDataGenerator<int32_t, int64_t, double> gen(1, 11, 7);
    auto eventLists = gen.prepareEventLists(gen.eventLists().size());
    auto parser = gen.createSimpleParser(eventLists, gen.bankOffsets());

    parser.setPulseInformation(gen.eventIndex(0), gen.eventTimeZero());
    auto event_id = gen.eventId(0);
    auto event_time_offset = gen.eventTimeOffset(0);

    auto parts = 5;
    auto portion = event_id.size() / parts;

    for (int i = 0; i < parts; ++i) {
      auto offset = portion * i;

      // Needed so that no data is missed.
      if (i == (parts - 1))
        portion = event_id.size() - offset;

      LoadRange range{0, offset, portion};
      parser.startParsing(event_id.data() + offset,
                          event_time_offset.data() + offset, range);
      parser.wait();
    }
    parser.finalize();
    for (int i = 0; i < gen.eventLists().size(); ++i)
      TS_ASSERT_EQUALS(*eventLists[i], *gen.eventLists()[i]);
    gen.cleanupEventLists(eventLists);
  }

  void testParsingFull_InParts_1Rank_3Banks() {
    size_t numBanks = 3;
    detail::FakeParserDataGenerator<int32_t, int64_t, double> gen(3, 20, 7);
    auto eventLists = gen.prepareEventLists(gen.eventLists().size());
    auto parser = gen.createSimpleParser(eventLists, gen.bankOffsets());

    for (int bank = 0; bank < numBanks; bank++) {
      parser.setPulseInformation(gen.eventIndex(bank), gen.eventTimeZero());
      auto event_id = gen.eventId(bank);
      auto event_time_offset = gen.eventTimeOffset(bank);

      auto parts = 11;
      auto portion = event_id.size() / parts;

      for (int i = 0; i < parts; ++i) {
        auto offset = portion * i;

        // Needed so that no data is missed.
        if (i == (parts - 1))
          portion = event_id.size() - offset;

        LoadRange range{bank, offset, portion};
        parser.startParsing(event_id.data() + offset,
                            event_time_offset.data() + offset, range);
        parser.wait();
      }
    }
    parser.finalize();
    for (int i = 0; i < gen.eventLists().size(); ++i)
      TS_ASSERT_EQUALS(*eventLists[i], *gen.eventLists()[i]);

    gen.cleanupEventLists(eventLists);
  }

private:
  template <typename IndexType, typename TimeZeroType, typename TimeOffsetType>
  void
  doTestRankData(const std::vector<std::vector<Event>> &rankData,
                 EventParser<IndexType, TimeZeroType, TimeOffsetType> &parser,
                 detail::FakeParserDataGenerator<IndexType, TimeZeroType,
                                                 TimeOffsetType> &gen,
                 const LoadRange &range) {
    size_t cur = 0;
    auto res = parser.findStartAndEndPulses(
        gen.eventIndex(0), range.eventOffset, range.eventCount, cur);

    for (size_t pulse = res.first; pulse <= res.second; ++pulse) {
      auto start =
          std::max(pulse == 0
                       ? 0
                       : static_cast<size_t>(gen.eventIndex(0)[pulse - 1]),
                   range.eventOffset) -
          range.eventOffset;
      auto end = std::min(static_cast<size_t>(gen.eventIndex(0)[pulse] - 1),
                          range.eventOffset + range.eventCount) -
                 range.eventOffset;
      auto &pulses = gen.eventTimeZero();
      TS_ASSERT(std::all_of(rankData[0].cbegin() + start,
                            rankData[0].cbegin() + end,
                            [pulses, pulse](const Event &e) {
                              return e.tofEvent.pulseTime() ==
                                     static_cast<int64_t>(pulses[pulse]);
                            }));
    }
  }
};

// TODO Add performance Tests
#endif /* MANTID_PARALLEL_COLLECTIVESTEST_H_ */
