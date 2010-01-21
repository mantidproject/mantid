#ifndef MANTID_DATAHANDLING_LOADDETECTORINFO_H_
#define MANTID_DATAHANDLING_LOADDETECTORINFO_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument/ParametrizedComponent.h"
#include "MantidGeometry/Instrument/Component.h"
#include <vector>
#include <string>
#include <climits>

namespace Mantid
{
namespace DataHandling
{
/** Adjusts TOF X-values for offset times and adds or modifies values for "3He(atm)" and
   "wallT(m)" in the workspace's parameter map using values read in from a DAT or RAW file.
   The RAW file or DAT file that is loaded should corrospond to the same run or series of
   experimental runs that created the workspace and no checking of units is done here.

  Depends on the format described in "DETECTOR.DAT format" data specified by Prof G Toby Perring ("detector format.doc")

    Required Properties:
    <UL>
    <LI> Workspace - The name of the Workspace to modify </LI>
    <LI> FileName - Path to the DAT or RAW file </LI>
    </UL>

    @author Steve Williams STFC Rutherford Appleton Laboratory
    @date 27/07/2009

    Copyright &copy; 2008-9 STFC Rutherford Appleton Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport LoadDetectorInfo : public API::Algorithm
{
public:
  LoadDetectorInfo();

  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "LoadDetectorInfo"; }
  /// Algorithm's version for identification overriding a virtual method
  virtual const int version() const { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const{return "DataHandling\\Detectors";}

private:
  // stores the information that is to be saved in the parameter map for a detector
  struct detectorInfo
  {
    int detID;                                      ///< ID number of the detector
    double pressure;                                ///< detectors 3He partial pressure
    double wallThick;                               ///< detector wall thickness
  };
  /// will store a pointer to the user selected workspace
  DataObjects::Workspace2D_sptr m_workspace;
  /// the instrument with in the user selected workspace
  API::IInstrument_sptr m_instrument;
  /// the map that stores additional properties for detectors
  Geometry::ParameterMap *m_paraMap;
  /// number of histograms in the workspace
  int m_numHists;
  /// the detector IDs that are monitors, according to the raw file
  std::set<int> m_monitors;
  /// Xbin boundaries for the monitors, normally monitors have a different time delay and hence a different offset
  DataObjects::Histogram1D::RCtype m_monitorXs;
  /// stores if the bin boundary values, X arrays, we initially common, because if that is we'll work to maximise sharing
  bool m_commonXs;
  /// the delay time for monitors, this algorithm requires all monitors have the same delay. Normally the delay is zero
  float m_monitOffset;
  /// notes if an error was found and the workspace was possibly only partially corrected
  bool m_error;
  /// An estimate of the percentage of the algorithm runtimes that has been completed 
  double m_FracCompl;

  // Implement abstract Algorithm methods
  void init();
  void exec();
  
  void readDAT(const std::string& fName);
  void readRAW(const std::string& fName);

  void setDetectorParams(const detectorInfo &params, detectorInfo &changed);
  void adjDelayTOFs(double lastOffset, bool &differentDelays, const std::vector<int> &detectIDs=std::vector<int>(), const std::vector<float> &delays=std::vector<float>());
  void adjDelayTOFs(double lastOffset, bool &differentDelays, const int * const detectIDs, const float * const delays, int numDetectors);
  void adjustXs(const std::vector<int> &detIDs, const std::vector<float> &offsets);
  void adjustXs(const double detectorOffset);
  void adjustXsCommon(const std::vector<float> &offsets, const std::vector<int> &spectraList, std::map<int,int> &specs2index, std::vector<int> missingDetectors);
  void adjustXsUnCommon(const std::vector<float> &offsets, const std::vector<int> &spectraList, std::map<int,int> &specs2index, std::vector<int> missingDetectors);
  void noteMonitorOffset(const float offSet, const int detID);
  void setUpXArray(DataObjects::Histogram1D::RCtype &theXValuesArray, int specInd, double offset);
  void logErrorsFromRead(const std::vector<int> &missingDetectors);
  void sometimesLogSuccess(const detectorInfo &params, bool &setToFalse);

  /// used to check that all the monitors have the same offset time
  static const float UNSETOFFSET;
  
  /// flag values
  enum {
    USED = 1000-INT_MAX,                                 ///< goes in the unGrouped spectra list to say that a spectrum will be included in a group, any other value and it isn't. Spectra numbers should always be positive so we shouldn't accidientally set a spectrum number to the this
    EMPTY_LINE = 1001-INT_MAX,                           ///< when reading from the input file this value means that we found any empty line
    IGNORE_SPACES = Poco::StringTokenizer::TOK_TRIM      ///< =Poco::StringTokenizer::TOK_TRIM but saves some typing
  };

  /// special numbers in DAT files from "DETECTOR.DAT format" referenced above
  enum {
    PSD_GAS_TUBE = 3,
    MONITOR_DEVICE = 1,
    DUMMY_DECT = 0
  };

  /// Apparently undocumented constants for excitations RAW files (assumed by Steve Williams)
  enum {
    OUR_TOTAL_NUM_TAB = 10,
    OUR_USER_TABLE_FORM = 2,
    USER_TABLE_MONITOR = 1,
    PRESSURE_TAB_NUM = 7,
    WALL_THICK_TAB_NUM = 8,
  };

  static const int INTERVAL = 512;                       ///< update this many detectors before checking for user cancel messages and updating the progress bar

};

} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADDETECTORINFO_H_*/
