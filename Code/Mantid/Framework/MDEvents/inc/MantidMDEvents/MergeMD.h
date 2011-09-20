#ifndef MANTID_MDEVENTS_MERGEMD_H_
#define MANTID_MDEVENTS_MERGEMD_H_
/*WIKI* 

This algorithm is meant to merge a large number of large MDEventWorkspaces together into one file-backed MDEventWorkspace, without exceeding available memory.

First, you will need to generate a MDEventWorkspaces NXS file for each run with a fixed box structure:

* This would be a MaxDepth=1 structure but with finer boxes, maybe 50x50x50.
* This can be done immediately after acquiring each run so that less processing has to be done at once.


Then, enter the path to all of the files created previously. The algorithm avoids excessive memory use by only keeping the events from ONE box from ALL the files in memory at once to further process and refine it.
This is why it requires a common box structure.
*WIKI*/
    
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h" 
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidNexusCPP/NeXusFile.hpp"

namespace Mantid
{
namespace MDEvents
{

  /** Algorithm to merge multiple MDEventWorkspaces from files that
   * obey a common box format.
    
    @author Janik Zikovsky
    @date 2011-08-16

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class DLLExport MergeMD  : public API::Algorithm
  {
  public:
    MergeMD();
    ~MergeMD();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "MergeMD";};
    /// Algorithm's version for identification 
    virtual int version() const { return 1;};
    /// Algorithm's category for identification
    virtual const std::string category() const { return "MDEvents";}
    
  private:
    /// Sets documentation strings for this algorithm
    virtual void initDocs();
    /// Initialise the properties
    void init();
    /// Run the algorithm
    void exec();

    template<typename MDE, size_t nd>
    void loadBoxData();

    template<typename MDE, size_t nd>
    typename MDEventWorkspace<MDE, nd>::sptr createOutputWS(typename MDEventWorkspace<MDE, nd>::sptr ws);

    template<typename MDE, size_t nd>
    typename MDEventWorkspace<MDE, nd>::sptr createOutputWSbyCloning(typename MDEventWorkspace<MDE, nd>::sptr ws);

    template<typename MDE, size_t nd>
    void doExec(typename MDEventWorkspace<MDE, nd>::sptr ws);

    template<typename MDE, size_t nd>
    void doExecByCloning(typename MDEventWorkspace<MDE, nd>::sptr ws);

    template<typename MDE, size_t nd>
    void finalizeOutput(typename MDEventWorkspace<MDE, nd>::sptr outWS);

  public:

    /// Files to load
    std::vector<std::string> m_filenames;

    /// Vector of file handles to each input file
    std::vector< ::NeXus::File *> files;

    /// Vector of the box_index vector for each each input file
    std::vector< std::vector<uint64_t> > box_indexes;

    /// Number of events in each box, summed over all input files
    std::vector<uint64_t> eventsPerBox;

    /// # of boxes in the input workspaces.
    size_t numBoxes;

    /// Output IMDEventWorkspace
    Mantid::API::IMDEventWorkspace_sptr outIWS;

    /// # of events from ALL input files
    uint64_t totalEvents;

    /// # of events loaded from all tasks
    uint64_t totalLoaded;

    /// Mutex for file access
    Kernel::Mutex fileMutex;

    /// Mutex for modifying stats
    Kernel::Mutex statsMutex;

    /// Progress reporter
    Mantid::API::Progress * prog;

    /// Set to true if the output is cloned of the first one
    bool clonedFirst;

  };


} // namespace MDEvents
} // namespace Mantid

#endif  /* MANTID_MDEVENTS_MERGEMD_H_ */
