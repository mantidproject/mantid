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


#ifndef Foundation_FilterChannel_INCLUDED
#define Foundation_FilterChannel_INCLUDED

#include "MantidKernel/System.h"

#include "Poco/Foundation.h"
#include "Poco/Channel.h"
#include "Poco/Mutex.h"
#include <vector>


namespace Poco {

/// This channel sends a message to multiple
/// channels simultaneously.
class DLLExport FilterChannel: public Channel

{
public:
	/// Creates the SplitterChannel.
	FilterChannel();

  ///destructor
	~FilterChannel();

	/// Attaches a channel, which may not be null.
	void addChannel(Channel* pChannel);

	/// Returns the channel pointer.
  Channel* getChannel() {return _channel;}

	/// Attaches a channel, which may not be null.
  const FilterChannel& setPriority(const std::string& priority);

  /// Returns the integer representation of the priority
  unsigned int getPriority() const { return _priority; }
		
	/// Sends the given Message to the attached channel. 
	void log(const Message& msg);

  /// Sets or changes a configuration property.
	void setProperty(const std::string& name, const std::string& value);

  /// Removes all channels.
	void close();
		
protected:

private:
  ///private pointer to the channel to pass messages onto
	Channel*        _channel;
  ///The priority used to filter messages
  int _priority;
  ///A mutex lock to prevent race conditions
	mutable FastMutex _mutex;
};


} // namespace Poco


#endif // Foundation_FilterChannel_INCLUDED
