
#include "MantidKernel/FilterChannel.h"

#include <Poco/LoggingRegistry.h>
#include <Poco/StringTokenizer.h>
#include <Poco/Message.h>


namespace Poco {

  FilterChannel::FilterChannel():_channel(0),_priority(8)
  {
  }

  FilterChannel::~FilterChannel()
  {
    close();
  }

  void FilterChannel::addChannel(Channel* pChannel)
  {
    poco_check_ptr (pChannel);

    FastMutex::ScopedLock lock(_mutex);

    pChannel->duplicate();
    _channel=pChannel;
  }

  void FilterChannel::setProperty(const std::string& name, const std::string& value)
  {
    if (name.compare(0, 7, "channel") == 0)
    {
      StringTokenizer tokenizer(value, ",;", StringTokenizer::TOK_IGNORE_EMPTY | StringTokenizer::TOK_TRIM);
      for (StringTokenizer::Iterator it = tokenizer.begin(); it != tokenizer.end(); ++it)
      {
        addChannel(LoggingRegistry::defaultRegistry().channelForName(*it));
      }
    }
    else if (name.compare(0, 5, "level") == 0)
    {
      setPriority(value);
    }
    else Channel::setProperty(name, value);
  }


  void FilterChannel::log(const Message& msg)
  {
    FastMutex::ScopedLock lock(_mutex);

    if(msg.getPriority() <= _priority)
    {
      _channel->log(msg);
    }
  }


  void FilterChannel::close()
  {
    FastMutex::ScopedLock lock(_mutex);
    if (_channel!=NULL)
    {
      _channel->release();
    }
  }

  const FilterChannel& FilterChannel::setPriority(const std::string& priority)
  {
    //take a local copy of the input
    std::string workPriority = priority;
    //convert to upper case
    std::transform(workPriority.begin(), workPriority.end(), workPriority.begin(), toupper);

    //if there is a prefix strip it off
    if (workPriority.compare(0, 5, "PRIO_") == 0)
    {
      workPriority = workPriority.substr(5,workPriority.length()-5);
    }

    if (workPriority.compare(0, 2, "FA") == 0) //PRIO_FATAL
      _priority=1;
    else if (workPriority.compare(0, 2, "CR") == 0)//PRIO_CRITICAL
      _priority=2;
    else if (workPriority.compare(0, 2, "ER") == 0)//PRIO_ERROR
      _priority=3;
    else if (workPriority.compare(0, 2, "WA") == 0)//PRIO_WARNING
      _priority=4;
    else if (workPriority.compare(0, 2, "NO") == 0)//PRIO_NOTICE
      _priority=5;
    else if (workPriority.compare(0, 2, "IN") == 0)//PRIO_INFORMATION
      _priority=6;
    else if (workPriority.compare(0, 2, "DE") == 0)//PRIO_DEBUG
      _priority=7;
    else if (workPriority.compare(0, 2, "TR") == 0)//PRIO_TRACE
      _priority=8;

    return *this;
  }


} // namespace Poco
