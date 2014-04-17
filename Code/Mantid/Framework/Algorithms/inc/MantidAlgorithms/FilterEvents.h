#ifndef MANTID_ALGORITHMS_FILTEREVENTS_H_
#define MANTID_ALGORITHMS_FILTEREVENTS_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/SplittersWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/ISplittersWorkspace.h"
#include "MantidKernel/TimeSplitter.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidKernel/TimeSeriesProperty.h"

namespace Mantid
{
namespace Algorithms
{

  /** FilterEvents : Filter Events in EventWorkspace to multiple EventsWorkspace by Splitters
    
    @date 2012-04-04

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
  class DLLExport FilterEvents : public API::Algorithm
  {
  public:
    FilterEvents();
    virtual ~FilterEvents();
    
    /// Algorithm's name for identification overriding a virtual method
    virtual const std::string name() const { return "FilterEvents";};
    /// Algorithm's version for identification overriding a virtual method
    virtual int version() const { return 1;};
    /// Algorithm's category for identification overriding a virtual method
    virtual const std::string category() const { return "Events\\EventFiltering";}

  private:
    /// Sets documentation strings for this algorithm
    virtual void initDocs();
    // Implement abstract Algorithm methods
    void init();
    // Implement abstract Algorithm methods
    void exec();

    /// Process user input properties
    void processProperties();

    void processSplittersWorkspace();

    ///
    void processMatrixSplitterWorkspace();

    void createOutputWorkspaces();

    void setupDetectorTOFCalibration();

    void filterEventsBySplitters(double progressamount);

    ///
    void filterEventsByVectorSplitters(double progressamount);

    DataObjects::EventWorkspace_sptr m_eventWS;
    DataObjects::SplittersWorkspace_sptr m_splittersWorkspace;
    API::MatrixWorkspace_const_sptr m_matrixSplitterWS;
    DataObjects::TableWorkspace_sptr m_detCorrectWorkspace;

    /// Flag to use matrix splitters or table splitters
    bool m_useTableSplitters;

    std::set<int> m_workGroupIndexes;
    Kernel::TimeSplitterType m_splitters;
    std::map<int, DataObjects::EventWorkspace_sptr> m_outputWS;
    std::vector<std::string> m_wsNames;

    std::vector<double> m_detTofOffsets;

    bool mFilterByPulseTime;

    DataObjects::TableWorkspace_sptr m_informationWS;
    bool mWithInfo;

    double mProgress;

    void getTimeSeriesLogNames(std::vector<std::string>& lognames);

    void generateSplitters(int wsindex, Kernel::TimeSplitterType& splitters);

    void splitLog(DataObjects::EventWorkspace_sptr eventws, std::string logname, Kernel::TimeSplitterType& splitters);

    /// Flag to do TOF correction
    bool m_doTOFCorrection;
    /// Flag to generate TOF correction
    bool m_genTOFCorrection;

    /// Base of output workspace's name
    std::string m_outputWSNameBase;

    ///
    bool m_toGroupWS;

    ///
    std::vector<int64_t> m_vecSplitterTime;
    ///
    std::vector<int> m_vecSplitterGroup;

    /// Flag to split sample logs
    bool m_splitSampleLogs;

    /// Debug
    bool m_useDBSpectrum;
    int m_dbWSIndex;
  };


} // namespace Algorithms
} // namespace Mantid

#endif  /* MANTID_ALGORITHMS_FILTEREVENTS_H_ */
