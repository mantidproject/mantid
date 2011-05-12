#ifndef MANTID_DATAHANDLING_LOADDETECTORINFO_H_
#define MANTID_DATAHANDLING_LOADDETECTORINFO_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidKernel/System.h"
#include <vector>
#include <string>
#include <climits>
#include <cfloat>

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

    @author Steve Williams ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory
    @date 27/07/2009

    Copyright &copy; 2008-9 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  virtual int version() const { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const{return "DataHandling\\Detectors";}

private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  /// stores the information that is to be saved in the parameter map for a detector
  struct detectorInfo
  {
    int detID;  ///< ID number of the detector
    double pressure;  ///< detectors 3He partial pressure
    double wallThick;  ///< detector wall thickness
    double l2;  ///< l2
    double theta; ///< theta
    double phi; ///<phi
    /// Constructor
    detectorInfo(): detID(-1), pressure(-1.0), wallThick(DBL_MAX), l2(DBL_MAX),
		    theta(DBL_MAX), phi(DBL_MAX) {}
  };
  /// will store a pointer to the user selected workspace
  API::MatrixWorkspace_sptr m_workspace;
  /// number of histograms in the workspace
  std::size_t m_numHists;
  /// the detector IDs that are monitors, according to the raw file
  std::set<int64_t> m_monitors;
  /// Xbin boundaries for the monitors, normally monitors have a different time delay and hence a different offset
  MantidVecPtr m_monitorXs;
  /// stores if the bin boundary values, X arrays, we initially common, because if that is we'll work to maximise sharing
  bool m_commonXs;
  /// the delay time for monitors, this algorithm requires all monitors have the same delay. Normally the delay is zero
  float m_monitOffset;
  /// notes if an error was found and the workspace was possibly only partially corrected
  bool m_error;
  /// An estimate of the percentage of the algorithm runtimes that has been completed 
  double m_FracCompl;
  /// If set to true then update the detector positions base on the information in the given file
  bool m_moveDets;
  /// Store the sample position as we may need it repeatedly
  Geometry::V3D m_samplePos;

  // Implement abstract Algorithm methods
  void init();
  void exec();
  
  void readDAT(const std::string& fName);
  void readRAW(const std::string& fName);

  void setDetectorParams(const detectorInfo &params, detectorInfo &changed);
  void adjDelayTOFs(double lastOffset, bool &differentDelays, const std::vector<int64_t> &detectIDs=std::vector<int64_t>(), const std::vector<float> &delays=std::vector<float>());
  void adjDelayTOFs(double lastOffset, bool &differentDelays, const int * const detectIDs, const float * const delays, std::size_t numDetectors);
  void adjustXs(const std::vector<int64_t> &detIDs, const std::vector<float> &offsets);
  void adjustXs(const double detectorOffset);
  void adjustXsCommon(const std::vector<float> &offsets, const std::vector<int64_t> &spectraList, std::map<int64_t,int64_t> &specs2index, std::vector<int64_t> missingDetectors);
  void adjustXsUnCommon(const std::vector<float> &offsets, const std::vector<int64_t> &spectraList, std::map<int64_t,int64_t> &specs2index, std::vector<int64_t> missingDetectors);
  void noteMonitorOffset(const float offSet, const int64_t detID);
  void setUpXArray(MantidVecPtr &theXValuesArray, int64_t specInd, double offset);
  void logErrorsFromRead(const std::vector<int64_t> &missingDetectors);
  void sometimesLogSuccess(const detectorInfo &params, bool &setToFalse);

  /// used to check that all the monitors have the same offset time
  static const float UNSETOFFSET;
  
  /// flag values
  enum {
    USED = 1000-INT_MAX,                                 ///< goes in the unGrouped spectra list to say that a spectrum will be included in a group, any other value and it isn't. Spectra numbers should always be positive so we shouldn't accidientally set a spectrum number to the this
    EMPTY_LINE = 1001-INT_MAX,                           ///< when reading from the input file this value means that we found any empty line
    IGNORE_SPACES = Poco::StringTokenizer::TOK_TRIM      ///< equals Poco::StringTokenizer::TOK_TRIM but saves some typing
  };

  /// special numbers in DAT files from "DETECTOR.DAT format" referenced above
  enum {
    PSD_GAS_TUBE = 3,
    NON_PSD_GAS_TUBE = 2,
    MONITOR_DEVICE = 1,
    DUMMY_DECT = 0
  };

  /// Holds data about the where detector information is stored in the user tables of raw files
  struct detectDatForm {
    /** default constructor
    *  @param total :: value for totalNumTabs
    *  @param pressure :: value pressureTabNum will be set to
    *  @param wall :: value to set wallThickTabNum to
    */    
    detectDatForm(unsigned int total=-1, unsigned int pressure=-1, unsigned int wall=-1) :
      totalNumTabs(total), pressureTabNum(pressure), wallThickTabNum(wall) {}
    unsigned int totalNumTabs;                      ///< total number of tables in the user area of the raw file
    unsigned int pressureTabNum;                    ///< user table that contains detector pressures (set this > totalNumTabs to crash the app!)
    unsigned int wallThickTabNum;                   ///< user table that contains detector pressures (set this > totalNumTabs to crash the app!)
  };
  static const detectDatForm MARI_TYPE;             ///< the data format that I've seen in MARI raw files (Steve Williams)
  static const detectDatForm MAPS_MER_TYPE;         ///< the data format that I've seen in MAPS and MERLIN files (Steve Williams)

  static const int INTERVAL = 512;                  ///< update this many spectra before checking for user cancel messages and updating the progress bar

};

} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADDETECTORINFO_H_*/
