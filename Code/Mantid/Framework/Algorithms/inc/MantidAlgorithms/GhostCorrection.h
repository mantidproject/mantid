#ifndef GHOSTCORRECTION_H_
#define GHOSTCORRECTION_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/DeprecatedAlgorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidKernel/UnitFactory.h"

namespace Mantid
{
namespace Algorithms
{

/**
 * Simple structure to map ghost corrections TO pixels.
 */
#pragma pack(push, 4) //Make the compiler pack the data size aligned to 4-bytes, so that the struct uses 12 bytes only
struct GhostDestinationValue
{
  /// The pixel ID at which the ghost peak occurs
  uint32_t pixelId;
  /// The weight of the event, as a fraction from 0.0 to 1.0 of the real event.
  double weight;
};
#pragma pack(pop) //restore default packing

///**
// * Simple structure to map ghost corrections FROM pixels.
// */
//struct GhostSourceValue
//{
//  /// The _workspace index_ in the input workspace from which the ghost pixel occurs.
//  int workspace_index;
//  /// The weight of the event, as a fraction from 0.0 to 1.0 of the real event.
//  double weight;
//};

/** Map of the pixels that are CAUSING a ghost in a given (grouped) pixel.
 * KEY = the workspace Index in the input workspace that is causing the ghost.
 * VALUE = corresponding input detector ID (saved to speed up a bit)
 */
typedef std::map<int, int> GhostSourcesMap;


/** Takes an EventWorkspace and performs Ghost correction for ghost peaks in
 * some SNS area detectors, for example on PG3.

    Required Properties:
    <UL>
    <LI> InputWorkspace - an EventWorkspace, it must have been aligned (d-spacing units) </LI>
    <LI>  </LI>
    </UL>

    And now, a comparison of the computational complexity of different possible algorithms...

    Definitions:
      N events per pixel
      M different pixels
      G ghost pixels per incoming pixel
      B histogramming bins.

    ALGORITHM A:
     - Sort input N events by TOF: time = M * (N log N)
     - For M pixels, go through N events (sorted by TOF)
       - For each event, find the bin; time = 1, max = B
       - Generate G ghost pixels per incoming event. They all have same TOF, therefore share the bin.
         - For each ghost, find the group to which it belongs; time = 1 (array lookup)
         - Increment the given bin by the weight of the bin; time = 1
     - Final complexity:  MN log N + 2*MNG
     - This algorithm CANNOT give you the ungrouped (unfocussed) pixels

    ALGORITHM B:
     - Make a reverse map of ghosting (with key of the output pixel #, find a list of input ghost-causing pixels).
     - Sort input N events by TOF: time = M * (N log N)
     - For M output pixels
       - For G ghost-causing pixels in the output
         - Add the N weighted events to a list; time=1
       - Sort the (fractional) events by TOF; time = GN log G log N
       - Perform the binning; time = GN
     - Final complexity: MN log N + 2*MGN + MGN log G log N
     - This algorithm CAN give you the ungrouped (unfocussed) pixels, if you have the memory!


    @author Janik Zikovsky, SNS
    @date Friday, August 13, 2010.

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class DLLExport GhostCorrection : public API::Algorithm, public API::DeprecatedAlgorithm
{
public:
  /// Default constructor
  GhostCorrection();
  /// Destructor
  virtual ~GhostCorrection();
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "GhostCorrection";}
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1;}
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "Diffraction";}

protected:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  // Overridden Algorithm methods
  void init();
  virtual void exec();
  void loadGhostMap(std::string ghostMapFile);
  void readGroupingFile(const std::string& groupingFilename);

  /// Grouping map
  Mantid::API::IndexToIndexMap detId_to_group;

  /// Map from detector ID to the offset (used in alignment)
  std::map<int, double> detId_to_offset;

  /// Pointer to the raw ghost map info
  std::vector<GhostDestinationValue> * rawGhostMap;

  /// Number of groups (max group # + 1)
  int nGroups;

  /// List of lists of ghost correction sources.
  std::vector< GhostSourcesMap * > groupedGhostMaps;

  /// Workspaces we are working with.
  Mantid::API::MatrixWorkspace_const_sptr inputW;

  /// Mapping between indices
  Mantid::API::IndexToIndexMap * input_detectorIDToWorkspaceIndexMap;

  /// Map where KEY = pixel ID; value = tof to D conversion factor (tof * factor = d).
  std::map<int, double> * tof_to_d;
};

} // namespace Algorithms
} // namespace Mantid





#endif /* GHOSTCORRECTION_H_ */
