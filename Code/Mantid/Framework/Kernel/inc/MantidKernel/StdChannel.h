#ifndef STDCHANNEL_H
#define STDCHANNEL_H

#include <MantidKernel/DllConfig.h>
#include <Poco/ConsoleChannel.h>
namespace Poco{
class MANTID_KERNEL_DLL StdChannel : public ConsoleChannel
{
    public:
    /// Constructor for StdChannel
    StdChannel();
    /// destructor
    ~StdChannel();
};
}
#endif // STDCHANNEL_H
