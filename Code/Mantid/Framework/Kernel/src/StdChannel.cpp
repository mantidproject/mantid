#include <MantidKernel/StdChannel.h>
#include <iostream>

namespace Poco {
StdChannel::StdChannel():ConsoleChannel(std::cout)
{
}
StdChannel::~StdChannel()
{
}

}
