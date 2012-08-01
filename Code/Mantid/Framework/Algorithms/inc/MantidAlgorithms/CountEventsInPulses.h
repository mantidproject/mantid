#ifndef MANTID_ALGORITHMS_COUNTEVENTSINPULSES_H_
#define MANTID_ALGORITHMS_COUNTEVENTSINPULSES_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid
{
namespace Algorithms
{

  /** CountEventsInPulses : TODO: DESCRIPTION
    
    @date 2012-03-27

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class DLLExport CountEventsInPulses : public API::Algorithm
  {
  public:
    CountEventsInPulses();
    virtual ~CountEventsInPulses();
    
    virtual const std::string name() const {return "CountEventsInPulses"; }
    virtual int version() const {return 1; }
    virtual const std::string category() const {return "Utility"; }

  private:
    virtual void initDocs();

    /// Properties definition
    void init();

    /// Main executation body
    void exec();

    /// Count
    DataObjects::EventWorkspace_sptr countInEventWorkspace();

    /// Create an EventWorkspace from input EventWorkspace
    DataObjects::EventWorkspace_sptr createEventWorkspace(DataObjects::EventWorkspace_const_sptr parentws);

    /// Count events (main algorithm)
    void convertEvents(DataObjects::EventWorkspace_sptr outWS);

    DataObjects::EventWorkspace_const_sptr inpWS;
    std::vector<Kernel::DateAndTime> mTimes; // Full size time
    std::vector<double> mTimesInSecond; // Full size time in second
    std::vector<Kernel::DateAndTime> mBinTimes; // Time with pulses binned
    size_t mBinSize;
    bool mSumSpectra;
    double mUnitFactor;
    double mPulseLength;

    double mTolerance;

  };


} // namespace Algorithms
} // namespace Mantid

#endif  /* MANTID_ALGORITHMS_COUNTEVENTSINPULSES_H_ */
