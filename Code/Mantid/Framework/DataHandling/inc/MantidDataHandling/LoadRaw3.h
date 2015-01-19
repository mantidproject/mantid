#ifndef MANTID_DATAHANDLING_LOADRAW3_H_
#define MANTID_DATAHANDLING_LOADRAW3_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadRawHelper.h"
#include "MantidDataObjects/Workspace2D.h"
#include <climits>

//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
class ISISRAW2;

namespace Mantid {
namespace DataHandling {
/** @class LoadRaw3 LoadRaw3.h DataHandling/LoadRaw3.h

Loads an file in ISIS RAW format and stores it in a 2D workspace
(Workspace2D class). LoadRaw is an algorithm and LoadRawHelper class and
overrides the init() & exec() methods.
LoadRaw3 uses less memory by only loading up the datablocks as required.

Copyright &copy; 2007-9 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class DLLExport LoadRaw3 : public LoadRawHelper {

public:
  /// Default constructor
  LoadRaw3();
  /// Destructor
  ~LoadRaw3();
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "LoadRaw"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Loads a data file in ISIS  RAW format and stores it in a 2D "
           "workspace (Workspace2D class).";
  }

  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 3; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "DataHandling\\Raw"; }

private:
  /// Overwrites Algorithm method.
  void init();
  /// Overwrites Algorithm method
  void exec();

  /// returns true if the given spectrum is a monitor
  bool isMonitor(const std::vector<specid_t> &monitorIndexes,
                 specid_t spectrumNum);

  /// validate workspace sizes
  void validateWorkspaceSizes(bool bexcludeMonitors, bool bseparateMonitors,
                              const int64_t normalwsSpecs,
                              const int64_t monitorwsSpecs);

  /// creates output workspace, monitors excluded from this workspace
  void excludeMonitors(FILE *file, const int &period,
                       const std::vector<specid_t> &monitorList,
                       DataObjects::Workspace2D_sptr ws_sptr);

  /// creates output workspace whcih includes monitors
  void includeMonitors(FILE *file, const int64_t &period,
                       DataObjects::Workspace2D_sptr ws_sptr);

  /// creates two output workspaces none normal workspace and separate one for
  /// monitors
  void separateMonitors(FILE *file, const int64_t &period,
                        const std::vector<specid_t> &monitorList,
                        DataObjects::Workspace2D_sptr ws_sptr,
                        DataObjects::Workspace2D_sptr mws_sptr);

  /// skip all spectra in a period
  void skipPeriod(FILE *file, const int64_t &period);
  /// return true if loading a selection of periods
  bool isSelectedPeriods() const { return !m_periodList.empty(); }
  /// check if a period should be loaded
  bool isPeriodIncluded(int period) const;
  /// get the previous period number
  int getPreviousPeriod(int period) const;

  /// sets optional properties
  void setOptionalProperties();

  /// sets progress taking account of progress time taken up by ChildAlgorithms
  void setProg(double);

  /// The name and path of the input file
  std::string m_filename;

  /// The number of spectra in the raw file
  specid_t m_numberOfSpectra;
  /// Allowed values for the cache property
  std::vector<std::string> m_cache_options;
  /// A map for storing the time regime for each spectrum
  std::map<int64_t, int64_t> m_specTimeRegimes;
  /// number of time regime
  int64_t m_noTimeRegimes;
  /// The current value of the progress counter
  double m_prog;
  /// Start and ends values of progress counter
  double m_prog_start;
  double m_prog_end;

  /// Read in the time bin boundaries
  int64_t m_lengthIn;
  /// time channels vector
  std::vector<boost::shared_ptr<MantidVec>> m_timeChannelsVec;
  /// total number of specs
  int64_t m_total_specs;
  /// A list of periods to read. Each value is between 1 and m_numberOfPeriods
  std::vector<int> m_periodList;
};

} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LoadRaw3_H_*/
