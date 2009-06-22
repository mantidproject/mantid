#ifndef MANTID_ALGORITHMS_GETDETECTOROFFSETS_H_
#define MANTID_ALGORITHMS_GETDETECTOROFFSETS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"
namespace Mantid
{
namespace Algorithms
{
/**
 Algorithm to focus powder diffraction data into a number of histograms according to a
 grouping scheme defined in a file.
 The structure of the grouping file is as follows:
 # Format: number  UDET offset  select  group
 0        611  0.0000000  1    0
 1        612  0.0000000  1    0
 2        601  0.0000000  0    0
 3        602  0.0000000  0    0
 4        621  0.0000000  1    0
 The first column is simply an index, the second is a UDET identifier for the detector,
 the third column corresponds to an offset in Deltad/d (not applied, usually applied using
 the AlignDetectors algorithm). The forth column is a flag to indicate whether the detector
 is selected. The fifth column indicates the group this detector belongs to (number >=1),
 zero is not considered as a group.

 Given an InputWorkspace and a Grouping filename, the algorithm follows:
 1) The calibration file is read and a map of corresponding udet-group is created.
 2) The algorithm determine the X boundaries for each group as the upper and lower limits
 of all contributing detectors to this group and determine a logarithmic step that will ensure
 preserving the number of bins in the initial workspace.
 3) All histograms are read and rebinned to the new grid for their group.
 4) A new workspace with N histograms is created.

 Since the new X boundaries depend on the group and not the entire workspace,
 this focusing algorithm does not create overestimated Xranges for multi-group intruments.

 Required Properties:
 <UL>
 <LI> InputWorkspace - The name of the 2D Workspace to take as input.
 It should be an histogram and the X-unit should be d-spacing. </LI>
 <LI> GroupingFileName - The path to a grouping file</LI>
 <LI> OutputWorkspace - The name of the 2D workspace in which to store the result </LI>
 </UL>

 @author Laurent Chapon, ISIS Facility, Rutherford Appleton Laboratory
 @date 08/03/2009

 Copyright &copy; 2009 STFC Rutherford Appleton Laboratory

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
class DLLExport GetDetectorOffsets: public API::Algorithm
{
public:
  /// Default constructor
  GetDetectorOffsets();
  /// Destructor
  virtual ~GetDetectorOffsets();
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "GetDetectorOffsets"; }
  /// Algorithm's version for identification overriding a virtual method
  virtual const int version() const { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "Diffraction"; }

private:
  API::MatrixWorkspace_sptr inputW;
  API::MatrixWorkspace_sptr outputW;
  // Overridden Algorithm methods
  void init();
  void exec();
  void fitSpectra(const int);
  void retrieveProperties();
  /// Static reference to the logger class
  static Mantid::Kernel::Logger& g_log;
  double Xmin;
  double Xmax;
  double dreference;
  double step;
  int nspec;
};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_GETDETECTOROFFSETS_H_*/
