
#include "MantidKernel/SignalChannel.h"

#include "Poco/LoggingRegistry.h"
#include "Poco/Message.h"
//#include "boost/bind.hpp"

#include <iostream>

namespace Poco {

    /// Connects a function (slot) to a SignalChannel with name chName. 
    /// chName must be defined in Mantid.properies file. A slot is a
    /// function of the type: void slot(const Message& msg).
    /// Returns true if the connection was successful.
    bool DLLExport connectSignal(const std::string& chName, void(*slt)(const Message& msg))
    {
        try
        {
            SignalChannel *pChannel = dynamic_cast<SignalChannel*>(Poco::LoggingRegistry::defaultRegistry().channelForName(chName));
            if (!pChannel) return false;
            pChannel->connect(slt);
        }
        catch(...)
        {
            return false;
        }
        return true;
    }

    SignalChannel::SignalChannel():Channel()
    {
    }

    void SignalChannel::log(const Message& msg)
    {
        _sig(msg);
    }

    /// Connects a slot to the channel.
    void SignalChannel::connect(void(*slt)(const Message& msg))
    {
        _sig.connect(slt);
    }

} // namespace Poco
