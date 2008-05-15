//
// FilterChannel.h
//
// $Id: //poco/1.3/Foundation/include/Poco/SplitterChannel.h#1 $
//
// Library: Foundation
// Package: Logging
// Module:  SplitterChannel
//
// Definition of the FilterChannel class. A small extension to the POCO logging.
//
// Copyright &copy; 2007 STFC Rutherford Appleton Laboratories
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


#ifndef Foundation_FilterChannel_INCLUDED
#define Foundation_FilterChannel_INCLUDED

#include "MantidKernel/System.h"

#include "Poco/Foundation.h"
#include "Poco/Channel.h"
#include "Poco/Mutex.h"
#include <vector>


namespace Poco {


class DLLExport FilterChannel: public Channel
	/// This channel sends a message to multiple
	/// channels simultaneously.
{
public:
	FilterChannel();
		/// Creates the SplitterChannel.

	~FilterChannel();

	void addChannel(Channel* pChannel);
		/// Attaches a channel, which may not be null.

  Channel* getChannel() {return _channel;}
	  /// Returns the channel pointer.

  const FilterChannel& setPriority(const std::string& priority);
		/// Attaches a channel, which may not be null.

  const unsigned int getPriority() const { return _priority; }
    /// Returns the integer representation of the priority
		
	void log(const Message& msg);
		/// Sends the given Message to all
		/// attaches channels. 

	void setProperty(const std::string& name, const std::string& value);
		/// Sets or changes a configuration property.
		///
		/// Only the "channel" property is supported, which allows
		/// adding a comma-separated list of channels via the LoggingRegistry.
		/// The "channel" property is set-only.
		/// To simplify file-based configuration, all property
		/// names starting with "channel" are treated as "channel".

	void close();
		/// Removes all channels.
		
protected:

private:
	Channel*        _channel;
  unsigned int _priority;
	mutable FastMutex _mutex;
};


} // namespace Poco


#endif // Foundation_FilterChannel_INCLUDED
