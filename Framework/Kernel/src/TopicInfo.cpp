// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/TopicInfo.h"
#include "MantidKernel/InstrumentInfo.h"
#include "MantidKernel/Logger.h"
#include <Poco/DOM/Element.h>

#include <utility>

namespace Mantid::Kernel {

namespace {
// static logger object
Logger g_log("TopicInfo");

std::string typeToString(TopicType type) {
  switch (type) {
  case TopicType::Event:
    return "Event";
  case TopicType::Monitor:
    return "Monitor";
  case TopicType::Sample:
    return "Sample";
  case TopicType::Chopper:
    return "Chopper";
  case TopicType::Run:
    return "Run";
  default:
    return "unspecified";
  }
}
} // namespace

TopicInfo::TopicInfo(InstrumentInfo *inst, const Poco::XML::Element *elem) : m_name(elem->getAttribute("name")) {
  if (m_name.empty())
    g_log.warning() << "Kafka topic provided without a suitable name for instrument " << inst->name()
                    << ". No attempts will be made to connect to this topic." << std::endl;

  std::string topicType = elem->getAttribute("type");

  if (topicType == "event")
    m_type = TopicType::Event;
  else if (topicType == "chopper")
    m_type = TopicType::Chopper;
  else if (topicType == "sample")
    m_type = TopicType::Sample;
  else if (topicType == "run")
    m_type = TopicType::Run;
  else if (topicType == "monitor")
    m_type = TopicType::Monitor;
  else
    g_log.warning() << "Kafka topic provided without a suitable type for instrument " << inst->name()
                    << ". No attempts will be made to connect to this topic." << std::endl;
}

TopicInfo::TopicInfo(std::string name, TopicType type) : m_name(std::move(name)), m_type(type) {}

/**
 * Prints the listener to the stream.
 *
 * @param buffer :: A reference to an output stream
 * @param topic :: A reference to a TopicInfo object
 * @return A reference to the stream written to
 */
std::ostream &operator<<(std::ostream &buffer, const TopicInfo &topic) {
  buffer << topic.name() << "(" << typeToString(topic.type()) << ", "
         << ")";
  return buffer;
}
} // namespace Mantid::Kernel
