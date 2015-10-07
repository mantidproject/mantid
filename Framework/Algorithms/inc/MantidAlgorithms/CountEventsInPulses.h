#ifndef MANTID_ALGORITHMS_COUNTEVENTSINPULSES_H_
#define MANTID_ALGORITHMS_COUNTEVENTSINPULSES_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid {
namespace Algorithms {

/** CountEventsInPulses : TODO: DESCRIPTION

  @date 2012-03-27

  Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

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
class DLLExport CountEventsInPulses : public API::Algorithm {
public:
  CountEventsInPulses();
  virtual ~CountEventsInPulses();

  virtual const std::string name() const { return "CountEventsInPulses"; }
  virtual int version() const { return 1; }
  virtual const std::string category() const { return "Utility"; }
  virtual const std::string summary() const {
    return "Counts the number of events in pulses.";
  }

private:
  /// Properties definition
  void init();

  /// Main executation body
  void exec();

  /// Create an EventWorkspace from input EventWorkspace
  DataObjects::EventWorkspace_sptr
  createEventWorkspace(DataObjects::EventWorkspace_const_sptr parentws,
                       bool sumspectrum);

  /// Count events (main algorithm)
  void convertEvents(DataObjects::EventWorkspace_sptr outWS, bool sumspectra);

  /// Rebin workspace
  void rebin(DataObjects::EventWorkspace_sptr outputWS);

  /// Compress events
  DataObjects::EventWorkspace_sptr
  compressEvents(DataObjects::EventWorkspace_sptr inputws, double tolerance);

  DataObjects::EventWorkspace_const_sptr inpWS;
  std::vector<double> mTimesInSecond;         // Full size time in second
  std::vector<Kernel::DateAndTime> mBinTimes; // Time with pulses binned

  /// Sum spectra or not
  bool mSumSpectra;

  double mUnitFactor;

  /// Average length of pulse in unit of second
  double mPulseLength;

  /// Bin size for future rebinning
  double mBinSize;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_COUNTEVENTSINPULSES_H_ */
