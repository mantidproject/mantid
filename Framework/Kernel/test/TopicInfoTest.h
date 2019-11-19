// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_TOPICINFOTEST_H_
#define MANTID_KERNEL_TOPICINFOTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/InstrumentInfo.h"
#include "MantidKernel/TopicInfo.h"

#include <Poco/AutoPtr.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>

using namespace Mantid::Kernel;

class TopicInfoTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static TopicInfoTest *createSuite() { return new TopicInfoTest(); }
  static void destroySuite(TopicInfoTest *suite) { delete suite; }

  void test_return_correct_topic_type() {
    auto *facility = createFakeFacilityWithTopics();

    auto inst = facility->instruments().front();
    auto topics = inst.topicInfoList();

    TS_ASSERT_EQUALS(topics.size(), 5);
    TS_ASSERT_EQUALS(topics[0].type(), TopicType::Chopper);
    TS_ASSERT_EQUALS(topics[1].type(), TopicType::Sample);
    TS_ASSERT_EQUALS(topics[2].type(), TopicType::Run);
    TS_ASSERT_EQUALS(topics[3].type(), TopicType::Event);
    TS_ASSERT_EQUALS(topics[4].type(), TopicType::Monitor);

    delete facility;
  }

private:
  FacilityInfo *createFakeFacilityWithTopics() {
    const std::string xml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<facilities>"
        "  <facility name=\"MyFacility\" FileExtensions=\".xyz\">"
        "    <instrument name=\"MyInstrument\">"
        "      <zeropadding size=\"8\" />"
        "      <technique>Novel Technique</technique>"
        "      <livedata default=\"event\">"
        "        <connection name=\"event\" address=\"localhost\" "
        "listener=\"KafkaEventListener\" />"
        "        <topic name=\"choppers\" type=\"chopper\" />"
        "        <topic name=\"sample\" type=\"sample\" />"
        "        <topic name=\"run\" type=\"run\" />"
        "        <topic name=\"detector_events\" type=\"event\" />"
        "        <topic name=\"monitor\" type=\"monitor\" />"
        "      </livedata>"
        "    </instrument>"
        "  </facility>"
        "</facilities>";

    Poco::XML::DOMParser parser;

    Poco::AutoPtr<Poco::XML::Document> pDoc = parser.parseString(xml);
    Poco::XML::Element *pRootElem = pDoc->documentElement();
    Poco::XML::Element *elem = pRootElem->getChildElement("facility");
    return new FacilityInfo(elem);
  }
};

#endif /* MANTID_KERNEL_TOPICINFOTEST_H_ */