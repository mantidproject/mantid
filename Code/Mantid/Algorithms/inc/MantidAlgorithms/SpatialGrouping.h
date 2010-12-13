#ifndef MANTID_ALGORITHMS_SPATIAL_GROUPING_H_
#define MANTID_ALGORITHMS_SPATIAL_GROUPING_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{

namespace Geometry
{
/// Forward Declarations
class IDetector;
class IInstrument;
}

namespace Algorithms
{
/**
    This algorithm creates an XML Grouping File for use in the GroupDetectos (v2) or
    ReadGroupsFromFile algorithms. It does this by querying the NearestNeighbours in
    Geometry through the detector's parameter map.

    @author Michael Whitty, STFC ISIS
    @date 13/12/2010

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
class DLLExport SpatialGrouping : public API::Algorithm
{
public:
  /// (Empty) Constructor
  SpatialGrouping() : API::Algorithm() {}
  /// Virtual destructor
  virtual ~SpatialGrouping() {}
  /// Algorithm's name
  virtual const std::string name() const { return "SpatialGrouping"; }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "General"; }

private:
  /// Initialisation code
  void init();
  /// Execution code
  void exec();

  /// check whether a Detector ID has already been assosciated with a group
  bool included(std::map<int,double> nearest);
  /// expand our search out to the next neighbours along
  bool expandNet(std::map<int,double> & nearest, boost::shared_ptr<Mantid::Geometry::IDetector> det, const int noNeighbours);
  /// sort by distance
  void sortByDistance(std::map<int, double> & nearest, const int noNeighbours);

  /// map of detectors in the instrument
  std::map<int, boost::shared_ptr<Mantid::Geometry::IDetector> > m_detectors;
  /// flag which detectors are included in a group already
  std::map<int, bool> m_included;
  /// first and last values for each group
  std::vector<std::vector<int> > m_groups;
  /// distance (radius) parameter for search
  double m_radius;
  /// map of Detector ID to Workspace Index (for seeing which detectors are in the workspace, and creating grouping file)
  Mantid::API::IndexToIndexMap* m_detIDtoWI;
  /// map of Workspace Index to Spectrum Number (for creating grouping file)
  Mantid::API::IndexToIndexMap* m_wiToSpect;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_SPATIAL_GROUPING_H_*/
