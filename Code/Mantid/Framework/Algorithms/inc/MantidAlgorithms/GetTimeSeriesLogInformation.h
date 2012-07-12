#ifndef MANTID_ALGORITHMS_GETTIMESERIESLOGINFORMATION_H_
#define MANTID_ALGORITHMS_GETTIMESERIESLOGINFORMATION_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"

namespace Mantid
{
namespace Algorithms
{

  /** GetTimeSeriesLogInformation : Read a TimeSeries log and return some information required by users.
    
    @date 2011-12-22

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
  class DLLExport GetTimeSeriesLogInformation : public API::Algorithm
  {
  public:
    GetTimeSeriesLogInformation();
    virtual ~GetTimeSeriesLogInformation();
    
    virtual const std::string name() const {return "GetTimeSeriesLogInformation"; };
    virtual int version() const {return 1; };
    virtual const std::string category() const {return "Diffraction;Events\\EventFiltering"; };

  private:
    DataObjects::EventWorkspace_sptr eventWS;
     DataObjects::Workspace2D_const_sptr seWS;
     DataObjects::EventWorkspace_sptr outputWS;

     std::vector<int64_t> mSETimes;
     std::vector<double> mSEValues;

     Kernel::DateAndTime mRunStartTime;
     Kernel::DateAndTime mFilterT0;
     Kernel::DateAndTime mFilterTf;

     virtual void initDocs();

     void init();

     void exec();

     void examLog(std::string logname, std::string outputdir);

     void generateCalibrationFile();

     void doTimeRangeInformation();

     void calDistributions(std::vector<Kernel::DateAndTime> timevec, double dts);

     void exportLog();

     void doStatistic();

     void exportErrorLog(API::MatrixWorkspace_sptr ws, std::vector<Kernel::DateAndTime> abstimevec, double dts);

     void checkLogAlternating(std::vector<Kernel::DateAndTime>timevec, std::vector<double> values,  double delta);

     void checkLogBasicInforamtion(API::MatrixWorkspace_sptr ws, std::string logname);
  };


} // namespace Algorithms
} // namespace Mantid

#endif  /* MANTID_ALGORITHMS_GETTIMESERIESLOGINFORMATION_H_ */
