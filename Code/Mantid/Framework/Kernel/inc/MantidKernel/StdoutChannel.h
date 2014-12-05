//
// StdoutChannel.h
//
// Similar to console channel for logging. The output is on std::cout instead of std::clog (which is the same as std::cerr)
// Usage: use in it Mantid.properties or mantid.user.properties instead of ConsoleChannel class
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source
//
// This file is part of Mantid.
//
// Mantid is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// Mantid is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//File change history is stored at: <https://github.com/mantidproject/mantid>
//


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
