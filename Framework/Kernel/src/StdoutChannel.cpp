#include <MantidKernel/StdoutChannel.h>
#include <iostream>

namespace Poco {
StdoutChannel::StdoutChannel() : ConsoleChannel(std::cout) {}
} // namespace Poco
