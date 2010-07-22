//
// SignalChannel.h
//
//
// Definition of the SignalChannel class. A small extension to the POCO logging.
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory
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
//File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
//


#ifndef Foundation_SignalChannel_INCLUDED
#define Foundation_SignalChannel_INCLUDED

#include "MantidKernel/System.h"

#include "Poco/Foundation.h"
#include "Poco/Channel.h"
//#include "Poco/Mutex.h"
#include "boost/signal.hpp"

#include <vector>


namespace Poco {

/**
   class SignalChannel passes log messages to slots, connected to it. 

**/
class DLLExport SignalChannel: public Channel
    /// This channel sends a message through boost::signal.
{
public:

    /// Signal type
    typedef boost::signal<void (const Message& msg)> signal_t;

    /// Creates the SignalChannel.
    SignalChannel();

    /// Connects a slot to the channel.
    void connect(void(*slt)(const Message& msg));

    /// Sends the given Message. 
	  void log(const Message& msg);

    /// Reference to the signal
    signal_t& sig(){return _sig;}

private:
	//mutable FastMutex _mutex;
    signal_t _sig;///< boost::signal used to send the message
};


} // namespace Poco


#endif // Foundation_SignalChannel_INCLUDED
