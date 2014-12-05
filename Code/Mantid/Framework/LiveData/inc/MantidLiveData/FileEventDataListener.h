#ifndef MANTID_LIVEDATA_FILEEVENTDATALISTENER_H_
#define MANTID_LIVEDATA_FILEEVENTDATALISTENER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/ILiveListener.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventWorkspace.h"
#include <Poco/ActiveMethod.h>
#include <Poco/Void.h>

namespace Mantid
{
  namespace LiveData
  {
    /** An implementation of ILiveListener for testing purposes that reads from a
        file and serves up 'chunks' when extractBuffer() is called.

        To avoid polluting the interface the file to use and the number of chunks to
        divide it into need to be set via configuration properties (i.e. programmatically
        via the ConfigService or included in Mantud.user.properties):
         - fileeventdatalistener.filename
         - fileeventdatalistener.chunks

        Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

        This file is part of Mantid.

        Mantid is free software; you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation; either version 3 of the License, or
        (at your option) any later version.

        Mantid is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
        GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
        along with this program.  If not, see <http://www.gnu.org/licenses/>.
     */
    class FileEventDataListener : public API::ILiveListener
    {
    public:
      FileEventDataListener();
      ~FileEventDataListener();

      std::string name() const { return "FileEventDataListener"; }
      bool supportsHistory() const { return false; } // For the time being at least
      bool buffersEvents() const { return true; }

      bool connect(const Poco::Net::SocketAddress& address);
      void start(Kernel::DateAndTime startTime = Kernel::DateAndTime());
      boost::shared_ptr<API::Workspace> extractData();

      bool isConnected();
      ILiveListener::RunStatus runStatus();
      int runNumber() const;

    private:
      std::string m_filename;         ///< The file to read
      int m_runNumber;                ///< The number of the run in the file
      std::string m_tempWSname;       ///< The name of the hidden workspace that holds the next chunk
      int m_numChunks;                ///< The number of pieces to divide the file into
      int m_nextChunk;                ///< The number of the next chunk to be loaded
      std::string m_filePropName;     ///< The file property name for the loader
      std::string m_loaderName;       ///< The loader that will do the work
      bool m_canLoadMonitors;         ///< A flag to turn off monitor loading for loaders that can

      /// Future that holds the result of the latest call to LoadEventPreNexus
      Poco::ActiveResult<bool> * m_chunkload;
      void loadChunk();
      /// Shared pointer to the correct file loader instance - it needs to be kept alive.
      API::Algorithm_sptr m_loader;
    };

  } // namespace LiveData
} // namespace Mantid

#endif  /* MANTID_LIVEDATA_FILEEVENTDATALISTENER_H_ */
