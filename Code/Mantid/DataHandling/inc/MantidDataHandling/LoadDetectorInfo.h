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
  using DataObjects::Workspace2D_const_sptr;
  using API::MatrixWorkspace_sptr;
/** Adds or modifies values for "time offset (micro seconds)", "gas pressure (atm)" and
   "wall thickness (m)" in the parameter map of a workspace using values read in from a
   DAT file or RAW file. The RAW file or DAT file that is loaded should corrospond to
   the same run or series of experimental runs that created the workspace.

  Depends on the document "DETECTOR.DAT format" data specified by Prof G Toby Perring ("detector format.doc")

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
  /// An estimate of the percentage of the algorithm runtimes that has been completed 
  double m_FracCompl;

  // Implement abstract Algorithm methods
  void init();
  void exec();
  
  void readDAT(const std::string& fName);
  void readRAW(const std::string& fName);

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
    OUR_USER_TABLE_FORM = 2,
    USER_TABLE_MONITOR = 1,
    PRESSURE_TAB_NUM = 7,
    WALL_THICK_TAB_NUM = 8,
  };

  static const int INTERVAL = 256;                       ///< update this many detectors before checking for user cancel messages and updating the progress bar

};

} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADDETECTORINFO_H_*/
