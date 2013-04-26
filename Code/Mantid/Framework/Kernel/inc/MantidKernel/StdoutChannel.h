#ifndef STDOUTCHANNEL_H
#define STDOUTCHANNEL_H

#include <MantidKernel/DllConfig.h>
#include <Poco/ConsoleChannel.h>
namespace Poco{
class MANTID_KERNEL_DLL StdoutChannel : public ConsoleChannel
{
    public:
    /// Constructor for StdChannel
    StdoutChannel();
    /// destructor
    ~StdoutChannel();
};
}
#endif // STDOUTCHANNEL_H
