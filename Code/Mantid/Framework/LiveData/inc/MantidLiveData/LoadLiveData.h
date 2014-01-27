#ifndef MANTID_LIVEDATA_LOADLIVEDATA_H_
#define MANTID_LIVEDATA_LOADLIVEDATA_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidLiveData/LiveDataAlgorithm.h"
#include "MantidAPI/Workspace.h"

namespace Mantid
{
namespace LiveData
{

  /** Algorithm to load a chunk of live data.
   * Called by StartLiveData and MonitorLiveData
    
    @date 2012-02-16

    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class DLLExport LoadLiveData  : public LiveDataAlgorithm
  {
  public:
    LoadLiveData();
    virtual ~LoadLiveData();
    
    virtual const std::string name() const;
    virtual const std::string category() const;
    virtual int version() const;

    int runNumber() const;

    void exec();

  private:
    virtual void initDocs();
    void init();

    Mantid::API::Workspace_sptr runProcessing(Mantid::API::Workspace_sptr inputWS, bool PostProcess);
    Mantid::API::Workspace_sptr processChunk(Mantid::API::Workspace_sptr chunkWS);
    void runPostProcessing();

    void replaceChunk(Mantid::API::Workspace_sptr chunkWS);
    void addChunk(Mantid::API::Workspace_sptr chunkWS);
    void addMatrixWSChunk(const std::string &algoName, API::Workspace_sptr accumWS, API::Workspace_sptr chunkWS);
    void appendChunk(Mantid::API::Workspace_sptr chunkWS);

    void doSortEvents(Mantid::API::Workspace_sptr ws);

    /// The "accumulation" workspace = after adding, but before post-processing
    Mantid::API::Workspace_sptr m_accumWS;

    /// The final output = the post-processed accumulation workspace
    Mantid::API::Workspace_sptr m_outputWS;

    int m_runNumber; ///< The run number stored in the extracted workspace 'chunk', if any.
  };


} // namespace LiveData
} // namespace Mantid

#endif  /* MANTID_LIVEDATA_LOADLIVEDATA_H_ */
