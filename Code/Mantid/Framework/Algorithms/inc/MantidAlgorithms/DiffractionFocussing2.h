#ifndef MANTID_ALGORITHMS_DIFFRACTIONFOCUSSING2_H_
#define MANTID_ALGORITHMS_DIFFRACTIONFOCUSSING2_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventWorkspace.h"

// To be compatible with VSC Express edition that does not have tr1
#ifndef HAS_UNORDERED_MAP_H
#include <map>
#else
#include <tr1/unordered_map>
#endif

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
class DLLExport DiffractionFocussing2: public API::Algorithm
{
public:
  /// Default constructor
  DiffractionFocussing2();
  /// Destructor
  virtual ~DiffractionFocussing2();
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "DiffractionFocussing"; }
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 2; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "Diffraction"; }
  
private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  // Overridden Algorithm methods
  void init();
  void exec();
  void cleanup();

  // For events
  void execEvent();

  /// Read the calibration file and construct the udet2group map
  void readGroupingFile(const std::string& groupingFileName);

  /// Loop over the workspace and determine the rebin parameters (Xmin,Xmax,step) for each group.
  /// The result is stored in group2params
  void determineRebinParameters();
  int validateSpectrumInGroup(int spectrum_number);
  
  /// Shared pointer to a mutable input workspace
  API::MatrixWorkspace_sptr matrixInputW;

  /// Shared pointer to the event workspace
  DataObjects::EventWorkspace_sptr eventW;


  // This map does not need to be ordered, just a lookup for udet
#ifndef HAS_UNORDERED_MAP_H
  /// typedef for the storage of the UDET-group mapping
  typedef std::map<int, int> udet2groupmap;
#else
  typedef std::tr1::unordered_map<int,int> udet2groupmap;
#endif
  // This map needs to be ordered to process the groups in order.
  /// typedef for the storage of each group's X vector
  typedef std::map<int, boost::shared_ptr<MantidVec> > group2vectormap;
  /// Map from udet to group
  udet2groupmap udet2group;
  /// The list of group numbers
  std::vector<int> groupAtWorkspaceIndex;
  /// Map from the group number to the group's X vector
  group2vectormap group2xvector;
  /// Map from the group number to the group's summed weight vector
  group2vectormap group2wgtvector;
  /// The number of (used) groups
  int nGroups;
  /// The maximum # of the group in the file - for workspace2Ds, this can be > than nGroups.
  int maxgroup_in_file;
  /// Number of histograms
  int nHist;
  /// Number of points in the 2D workspace
  int nPoints;
};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_DIFFRACTIONFOCUSSING2_H_*/
