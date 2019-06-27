// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_LIVEDATA_FILEEVENTDATALISTENER_H_
#define MANTID_LIVEDATA_FILEEVENTDATALISTENER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/LiveListener.h"
#include "MantidDataObjects/EventWorkspace.h"
#include <Poco/ActiveMethod.h>
#include <Poco/Void.h>

namespace Mantid {
namespace LiveData {
/** An implementation of ILiveListener for testing purposes that reads from a
    file and serves up 'chunks' when extractBuffer() is called.

    To avoid polluting the interface the file to use and the number of chunks to
    divide it into need to be set via configuration properties (i.e.
   programmatically
    via the ConfigService or included in Mantud.user.properties):
     - fileeventdatalistener.filename
     - fileeventdatalistener.chunks
 */
class FileEventDataListener : public API::LiveListener {
public:
  FileEventDataListener();
  ~FileEventDataListener() override;

  std::string name() const override { return "FileEventDataListener"; }
  bool supportsHistory() const override {
    return false;
  } // For the time being at least
  bool buffersEvents() const override { return true; }

  bool connect(const Poco::Net::SocketAddress &address) override;
  void start(
      Types::Core::DateAndTime startTime = Types::Core::DateAndTime()) override;
  boost::shared_ptr<API::Workspace> extractData() override;

  bool isConnected() override;
  ILiveListener::RunStatus runStatus() override;
  int runNumber() const override;

private:
  std::string m_filename;   ///< The file to read
  int m_runNumber;          ///< The number of the run in the file
  std::string m_tempWSname; ///< The name of the hidden workspace that holds the
  /// next chunk
  int m_numChunks;            ///< The number of pieces to divide the file into
  int m_nextChunk;            ///< The number of the next chunk to be loaded
  std::string m_filePropName; ///< The file property name for the loader
  std::string m_loaderName;   ///< The loader that will do the work
  bool m_canLoadMonitors; ///< A flag to turn off monitor loading for loaders
  /// that can

  /// Future that holds the result of the latest call to LoadEventPreNexus
  std::unique_ptr<Poco::ActiveResult<bool>> m_chunkload;
  void loadChunk();
  /// Shared pointer to the correct file loader instance - it needs to be kept
  /// alive.
  API::Algorithm_sptr m_loader;
};

} // namespace LiveData
} // namespace Mantid

#endif /* MANTID_LIVEDATA_FILEEVENTDATALISTENER_H_ */
