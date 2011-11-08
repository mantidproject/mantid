#ifndef MANTID_ALGORITHMS_SmoothNeighbours_H_
#define MANTID_ALGORITHMS_SmoothNeighbours_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"

namespace Mantid
{
namespace Algorithms
{
  /** Smooth neighboring pixels.

    @authors Janik Zikovsky, Vickie Lynch, SNS
    @date Oct 2010

    Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class DLLExport SmoothNeighbours : public API::Algorithm
{
public:
  /// Default constructor
  SmoothNeighbours() : API::Algorithm() {};
  /// Destructor
  virtual ~SmoothNeighbours() {};
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "SmoothNeighbours";}
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return (1);}
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "General";}

private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  // Overridden Algorithm methods
  void init();
  void exec();

  void execWorkspace2D(Mantid::API::MatrixWorkspace_sptr ws);
  void execEvent(Mantid::DataObjects::EventWorkspace_sptr ws);

  void findNeighboursRectangular();
  void findNeighboursUbiqutious();

  /// Pixels in the detector
  int XPixels;
  /// Pixels in the detector
  int YPixels;

  /// Number to sum
  int AdjX;
  /// Number to sum
  int AdjY;
  /// Edge pixels to ignore
  int Edge;
  /// Radius to search nearest neighbours
  double Radius;
  /// Weight the neighbours during summing
  bool WeightedSum;
  /// PreserveEvents
  bool PreserveEvents;

  /// Input workspace
  Mantid::API::MatrixWorkspace_sptr inWS;

  /// Each neighbours is specified as a pair with workspace index, weight.
  typedef std::pair<size_t, double> weightedNeighbour;

  /// Vector of list of neighbours (with weight) for each workspace index.
  std::vector< std::vector< weightedNeighbour > > m_neighbours;

  /// Progress reporter
  Mantid::API::Progress * m_prog;

};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_SmoothNeighbours_H_*/
