#ifndef MANTID_PARALLEL_COLLECTIVESTEST_H_
#define MANTID_PARALLEL_COLLECTIVESTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidParallel/IO/EventParser.h"

using namespace Mantid;
using namespace Parallel::IO;

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

  void testConstructorFailZeroLengthBankOffsets() {
    std::vector<std::vector<int>> rankGroups;
    std::vector<int32_t> bankOffsets;
    std::vector<std::vector<Mantid::Types::TofEvent> *> eventLists(4);

    TS_ASSERT_THROWS((EventParser<int64_t, int64_t, double>(
                         rankGroups, bankOffsets, eventLists)),
                     std::invalid_argument);
  }

  void testConstructorFailZeroLengthEventLists() {
    std::vector<std::vector<int>> rankGroups;
    std::vector<int32_t> bankOffsets{1, 2, 3, 4};
    std::vector<std::vector<Mantid::Types::TofEvent> *> eventLists;

    TS_ASSERT_THROWS((EventParser<int64_t, int64_t, double>(
                         rankGroups, bankOffsets, eventLists)),
                     std::invalid_argument);
  }

  void testConvertEventIDToGlobalSpectrumIndex() {
    std::vector<std::vector<int>> rankGroups;
    std::vector<int32_t> bankOffsets{1000};
    std::vector<std::vector<Mantid::Types::TofEvent> *> eventLists(10);

    EventParser<int64_t, int64_t, double> parser(rankGroups, bankOffsets,
                                                 eventLists);

    std::vector<int32_t> eventId{1001, 1002, 1004, 1004};

    parser.eventIdToGlobalSpectrumIndex(eventId.data(), eventId.size(), 0);
    const auto &specIndex = parser.globalSpectrumIndex();

    TS_ASSERT_EQUALS(specIndex.size(), eventId.size());
    TS_ASSERT_EQUALS(specIndex[0], eventId[0] - bankOffsets[0]);
    TS_ASSERT_EQUALS(specIndex[1], eventId[1] - bankOffsets[0]);
    TS_ASSERT_EQUALS(specIndex[2], eventId[2] - bankOffsets[0]);
    TS_ASSERT_EQUALS(specIndex[3], eventId[3] - bankOffsets[0]);
  }

  void testExtractEventsForRanks_1Rank() {
    std::vector<std::vector<Mantid::Types::TofEvent> *> eventLists(10);
    std::vector<int32_t> event_index{10, 20, 35, 60, 90};
    std::vector<int64_t> event_time_zero{0, 100000, 200000, 300000, 400000};
    std::vector<int32_t> event_id(100);
    std::vector<int32_t> event_time_offset(100);

    auto parser = createSimpleParser(eventLists);
    parser.setPulseInformation(event_index, event_time_zero);

    size_t offset = 20;
    size_t numEvents = 30;
    size_t bankIndex = 0;
    parser.eventIdToGlobalSpectrumIndex(event_id.data() + offset, numEvents,
                                        bankIndex);
    const auto &specIndex = parser.globalSpectrumIndex();

    std::vector<std::vector<Event>> rankData(1);
    parser.extractEventsForRanks(rankData, specIndex,
                                 event_time_offset.data() + offset, offset);

    TS_ASSERT_EQUALS(rankData[0].size(), numEvents);

    size_t index = 1;
    auto test = [&event_time_zero, &index](const Event &item) {
      return item.tofEvent.pulseTime() == event_time_zero[index];
    };

    TS_ASSERT(
        std::all_of(rankData[0].cbegin(), rankData[0].cbegin() + 15, test));

    index = 2;
    TS_ASSERT(std::all_of(rankData[0].cbegin() + 15, rankData[0].cend(), test));
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

  void testParsing_1Rank() {
    auto eventLists = prepareEventLists(6);
    std::vector<int32_t> event_index{2, 5, 7};
    std::vector<int64_t> event_time_zero{0, 10, 20};
    // Also SpectrumIndex since the bankOffset is 0
    std::vector<int32_t> event_id{2, 5, 0, 4, 3, 5, 1, 4, 2, 1, 1};
    std::vector<int32_t> event_time_offset{1, 2, 4, 1, 7, 8, 3, 2, 4, 9, 1};

    auto parser = createSimpleParser(eventLists);

    parser.setPulseInformation(event_index, event_time_zero);

    size_t bank = 0;
    size_t offset = 3;
    size_t count = 6;
    TS_ASSERT_THROWS_NOTHING(parser.startParsing(
        event_id.data() + offset, event_time_offset.data() + offset,
        LoadRange{bank, offset, count}));

    std::vector<int32_t> testOutput1{3}, testOutput2{4}, testOutput3{7},
        testOutput4{1, 2}, testOutput5{8};

    auto testFunc = [](const Types::TofEvent &item, int32_t time_offset) {
      return item.tof() == static_cast<double>(time_offset);
    };

    TS_ASSERT(eventLists[0]->empty());
    TS_ASSERT(std::equal(eventLists[1]->cbegin(), eventLists[1]->cend(),
                         testOutput1.cbegin(), testFunc));
    TS_ASSERT(std::equal(eventLists[2]->cbegin(), eventLists[2]->cend(),
                         testOutput2.cbegin(), testFunc));
    TS_ASSERT(std::equal(eventLists[3]->cbegin(), eventLists[3]->cend(),
                         testOutput3.cbegin(), testFunc));
    TS_ASSERT(std::equal(eventLists[4]->cbegin(), eventLists[4]->cend(),
                         testOutput4.cbegin(), testFunc));
    TS_ASSERT(std::equal(eventLists[5]->cbegin(), eventLists[5]->cend(),
                         testOutput5.cbegin(), testFunc));
  }

private:
  EventParser<int32_t, int64_t, int32_t>
  createSimpleParser(std::vector<std::vector<Types::TofEvent> *> &eventLists) {
    std::vector<std::vector<int>> rankGroups;
    std::vector<int32_t> bankOffsets{0};

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
};

// TODO Add performance Tests
#endif /* MANTID_PARALLEL_COLLECTIVESTEST_H_ */
