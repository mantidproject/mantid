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

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class DLLExport FilterEvents : public API::Algorithm
  {   

    enum TOFCorrectionType {NoneCorrect, CustomizedCorrect, DirectCorrect, ElasticCorrect, IndirectCorrect};
    enum TOFCorrectionOp {MultiplyOp, ShiftOp};
    enum EVENTFILTERSKIP {EventFilterSkipNoDet, EventFilterSkipNoDetTOFCorr};

  public:
    FilterEvents();
    virtual ~FilterEvents();
    
    /// Algorithm's name for identification overriding a virtual method
    virtual const std::string name() const { return "FilterEvents";}
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Filter events from an EventWorkspace to one or multiple EventWorkspaces according to a series of splitters.";}

    /// Algorithm's version for identification overriding a virtual method
    virtual int version() const { return 1;}

    /// Algorithm's category for identification overriding a virtual method
    virtual const std::string category() const { return "Events\\EventFiltering";}

  private:
    
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

    /// Set up detector calibration parameters
    void setupDetectorTOFCalibration();

    /// Set up detector calibration parameters for elastic scattering instrument
    void setupElasticTOFCorrection(API::MatrixWorkspace_sptr corrws);

    /// Set up detector calibration parmaeters for direct inelastic scattering instrument
    void setupDirectTOFCorrection(API::MatrixWorkspace_sptr corrws);

    /// Set up detector calibration parameters for indirect inelastic scattering instrument
    void setupIndirectTOFCorrection(API::MatrixWorkspace_sptr corrws);

    /// Set up detector calibration parameters from customized values
    void setupCustomizedTOFCorrection();


    /// Filter events by splitters in format of Splitter
    void filterEventsBySplitters(double progressamount);

    /// Filter events by splitters in format of vector
    void filterEventsByVectorSplitters(double progressamount);

    /// Examine workspace
    void examineEventWS();

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
    std::vector<double> m_detTofShifts;

    bool mFilterByPulseTime;

    DataObjects::TableWorkspace_sptr m_informationWS;
    bool m_hasInfoWS;

    double mProgress;

    void getTimeSeriesLogNames(std::vector<std::string>& lognames);

    void generateSplitters(int wsindex, Kernel::TimeSplitterType& splitters);

    void splitLog(DataObjects::EventWorkspace_sptr eventws, std::string logname, Kernel::TimeSplitterType& splitters);

    /// Base of output workspace's name
    std::string m_outputWSNameBase;

    /// Flag to group workspace
    bool m_toGroupWS;

    /// Vector for splitting time
    std::vector<int64_t> m_vecSplitterTime;
    /// Vector for splitting grouip
    std::vector<int> m_vecSplitterGroup;

    /// Flag to split sample logs
    bool m_splitSampleLogs;

    /// Debug
    bool m_useDBSpectrum;
    int m_dbWSIndex;

    /// TOF detector/sample correction type
    TOFCorrectionType m_tofCorrType;

    /// Spectrum skip type
    EVENTFILTERSKIP m_specSkipType;
    /// Vector for skip information
    std::vector<bool> m_vecSkip;

  };


} // namespace Algorithms
} // namespace Mantid

#endif  /* MANTID_ALGORITHMS_FILTEREVENTS_H_ */
