#ifndef MANTID_ALGORITHMS_READGROUPSFROMFILE_H_
#define MANTID_ALGORITHMS_READGROUPSFROMFILE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

// To be compatible with MSVC++ Express Edition that does not have TR1 headers
#ifndef HAS_UNORDERED_MAP_H
#include <map>
#else
#include <tr1/unordered_map>
#endif
#include "MantidAPI/Algorithm.h"


namespace Mantid
{
namespace Algorithms
{
/** Read a diffraction calibration file (*.cal) and an instrument name, and output a 2D workspace
 * containing on the Y-axis the values of the Group each detector belongs to.
 * This is used to visualise the grouping scheme for powder diffractometers, where a large number of detectors
 * are grouped together. The output 2D workspace can be visualize using the show instrument method.
 * The format of the *.cal file is as follows:
 *
 *   # Format: number  UDET offset  select  group
 *   0        611  0.0000000  1    0
 *   1        612  0.0000000  1    0
 *   2        601  0.0000000  0    0
 *   3        602  0.0000000  0    0
 *   4        621  0.0000000  1    0
 *   The first column is simply an index, the second is a UDET identifier for the detector,
 *   the third column corresponds to an offset in Deltad/d (not applied, usually applied using
 *   the AlignDetectors algorithm). The forth column is a flag to indicate whether the detector
 *   is selected. The fifth column indicates the group this detector belongs to (number >=1),
 *   zero is not considered as a group.
 *   Required Properties:
 *   <UL>
 *   <LI> InstrumentName   - The name of the instrument. Needs to be present in the store </LI>
 *   <LI> GroupingFilename - The name of the output file (*.cal extension) </LI>
 *   <LI> ShowUnselected   - Option (true or false) to consider unselected detectors in the cal file </LI>
 *   <LI> OuputWorkspace   - The name of the output 2D Workspace containing the group information </LI>
 *   </UL>

    @author Laurent Chapon, ISIS Facility, Rutherford Appleton Laboratory
    @date 09/03/2009

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class DLLExport ReadGroupsFromFile : public API::Algorithm
{
public:
  /// (Empty) Constructor
  ReadGroupsFromFile();
  /// Virtual destructor
  virtual ~ReadGroupsFromFile() {}
  /// Algorithm's name
  virtual const std::string name() const { return "ReadGroupsFromFile"; }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "Diagnostics"; }

private:
	/// Map containing the detector entries found in the *.cal file. The key is the udet number, the value of is a pair of <group,selected>.
	#ifndef HAS_UNORDERED_MAP_H
	typedef std::map<int,std::pair<int,int> > calmap;
	#else
	typedef std::tr1::unordered_map<int,std::pair<int,int> > calmap;
	#endif
	/// Initialisation code
  void init();
  /// Execution code
  void exec();
  /// Read a grouping file and construct the calibration map
  /// @param groupingFileName (filename extension .cal, including path)
  void readGroupingFile(const std::string& groupingFileName);
  /// Sub-algorithm to Load the associated empty instrument
  /// @param instrument_xml_name The instrument xml name including extension(.xml or .XML) but no path
  /// this is determine by the mantid instrument.directory
  /// @return Shared pointer to the 2D workspace
  DataObjects::Workspace2D_sptr loadEmptyInstrument(const std::string& instrument_xml_name);
  /// Calibration map containing the detector entries found in the *.cal file. The key is the udet number, the value of is a pair of <group,selected>.
  calmap calibration;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_READGROUPFROMFILES*/
