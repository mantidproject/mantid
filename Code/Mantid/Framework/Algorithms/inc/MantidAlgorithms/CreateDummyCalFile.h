#ifndef MANTID_ALGORITHMS_CREATEDUMMYCALFILE_H_
#define MANTID_ALGORITHMS_CREATEDUMMYCALFILE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

// To be compatible with MSVC++ Express Edition that does not have TR1 headers
#include <map>
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace Algorithms
{
/** Create a calibration file for diffraction focussing (*.cal old Ariel format)
 *  based on list of names of the instrument tree.
 *  The offsets are all sets to zero and all detectors are selected. Detectors not assigned
 *  to any group will appear as group 0, i.e. not included when using AlignDetector or
 *  DiffractionFocussing algorithms.
 *  The group number is assigned based on a descent in the instrument tree assembly.
 *  If two assemblies are parented, say Bank1 and module1, and both assembly names
 *  are given in the GroupNames, they will get assigned different grouping numbers.
 *  This allows to isolate a particular sub-assembly of a particular leaf of the tree

    Required Properties:
    <UL>
    <LI> InstrumentName   - The name of the instrument. Needs to be present in the store</LI>
    <LI> GroupingFilename - The name of the output file (*.cal extension) .</LI>
    <LI> GroupNames       - Name of assemblies to consider (names separated by "/" or "," or "*"</LI>
    </UL>

    @author Vickie Lynch, SNS, ORNL
    @date 12/01/2010

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class DLLExport CreateDummyCalFile : public API::Algorithm
{
public:
  /// (Empty) Constructor
  CreateDummyCalFile();
  /// Virtual destructor
  virtual ~CreateDummyCalFile() {}
  /// Algorithm's name
  virtual const std::string name() const { return "CreateDummyCalFile"; }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "Diffraction"; }

private:
	/// Calibration entries map
	typedef std::map<int,std::pair<int,int> > instrcalmap;
  /// Initialisation code
  void init();
  /// Execution code
  void exec();
  /// The name and path of the input file
  std::string m_filename;
  /// Determine whether the grouping file already exists.
  /// @param filename :: GroupingFilename (extension .cal)
  /// @return true if the grouping file exists
  bool groupingFileDoesExist(const std::string& filename) const;
  void saveGroupingFile(const std::string&,bool overwrite) const;
  static void writeCalEntry(std::ostream& os, int number, int udet, double offset, int select, int group);
  void writeHeaders(std::ostream& os,const std::string& filename,bool overwrite) const;
  /// The names of the groups
  std::string groups;
  /// Calibration map used if the *.cal file exist. All entries in the *.cal file are registered with the udet number as the key and the <Number,Offset,Select,Group> as the tuple value.
  instrcalmap instrcalib;
  /// Number of groups
  int group_no;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_CREATEDUMMYCALFILE_H_*/
