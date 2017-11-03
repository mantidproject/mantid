
#include "MantidKernel/FilterChannel.h"
#include <MantidKernel/StringTokenizer.h>

#include <Poco/LoggingRegistry.h>
#include <Poco/Message.h>

#include <algorithm>

namespace Poco {

FilterChannel::FilterChannel() : _channel(nullptr), _priority(8) {}

FilterChannel::~FilterChannel() { close(); }

void FilterChannel::addChannel(Channel *pChannel) {
  poco_check_ptr(pChannel);
  std::lock_guard<std::mutex> lock(_mutex);

  pChannel->duplicate();
  _channel = pChannel;
}

void FilterChannel::setProperty(const std::string &name,
                                const std::string &value) {
  if (name.compare(0, 7, "channel") == 0) {
    Mantid::Kernel::StringTokenizer tokenizer(
        value, ",;", Mantid::Kernel::StringTokenizer::TOK_IGNORE_EMPTY |
                         Mantid::Kernel::StringTokenizer::TOK_TRIM);
    for (const auto &piece : tokenizer) {
      addChannel(LoggingRegistry::defaultRegistry().channelForName(piece));
    }
  } else if (name.compare(0, 5, "level") == 0) {
    setPriority(value);
  } else
    Channel::setProperty(name, value);
}

void FilterChannel::log(const Message &msg) {
  std::lock_guard<std::mutex> lock(_mutex);

  if ((_channel) && (msg.getPriority() <= _priority)) {
    _channel->log(msg);
  }
}

void FilterChannel::close() {
  std::lock_guard<std::mutex> lock(_mutex);
  if (_channel != nullptr) {
    _channel->release();
  }
}

const FilterChannel &FilterChannel::setPriority(const int &priority) {
  _priority = priority;

  return *this;
}

const FilterChannel &FilterChannel::setPriority(const std::string &priority) {
  // take a local copy of the input
  std::string workPriority = priority;
  // convert to upper case
  std::transform(workPriority.begin(), workPriority.end(), workPriority.begin(),
                 toupper);

  // if there is a prefix strip it off
  if (workPriority.compare(0, 5, "PRIO_") == 0) {
    workPriority = workPriority.substr(5, workPriority.length() - 5);
  }

  if (workPriority.compare(0, 2, "FA") == 0) // PRIO_FATAL
    _priority = 1;
  else if (workPriority.compare(0, 2, "CR") == 0) // PRIO_CRITICAL
    _priority = 2;
  else if (workPriority.compare(0, 2, "ER") == 0) // PRIO_ERROR
    _priority = 3;
  else if (workPriority.compare(0, 2, "WA") == 0) // PRIO_WARNING
    _priority = 4;
  else if (workPriority.compare(0, 2, "NO") == 0) // PRIO_NOTICE
    _priority = 5;
  else if (workPriority.compare(0, 2, "IN") == 0) // PRIO_INFORMATION
    _priority = 6;
  else if (workPriority.compare(0, 2, "DE") == 0) // PRIO_DEBUG
    _priority = 7;
  else if (workPriority.compare(0, 2, "TR") == 0) // PRIO_TRACE
    _priority = 8;

  return *this;
}

} // namespace Poco
