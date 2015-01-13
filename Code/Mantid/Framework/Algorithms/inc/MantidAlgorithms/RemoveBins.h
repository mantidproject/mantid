#ifndef MANTID_ALGORITHMS_REMOVEBINS_H_
#define MANTID_ALGORITHMS_REMOVEBINS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {
/** Removes bins from a workspace.

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the Workspace to take as input </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the
   result </LI>
    <LI> XMin - The value to start removing from..</LI>
    <LI> XMax - The time bin to end removing from.</LI>
    <LI> RangeUnit - The unit in which the above range is given.</LI>
    <LI> Interpolation - The type of interpolation to use for removed bins.
   </LI>
    </UL>

    Optional Property:
    <UL>
    <LI> WorkspaceIndex - The spectrum to remove bins from. Will operate on all
   spectra if absent. </LI>
    </UL>

    @author Matt Clarke
    @date 08/12/2008

    Copyright &copy; 2008-9 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
   National Laboratory & European Spallation Source

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

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class DLLExport RemoveBins : public API::Algorithm {
public:
  /// Default constructor
  RemoveBins();
  /// Destructor
  virtual ~RemoveBins(){};
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "RemoveBins"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Used to remove data from a range of bins in a workspace.";
  }

  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "Transforms\\Splitting"; }

private:
  // Overridden Algorithm methods
  void init();
  void exec();

  void checkProperties();
  void crop(const double &start, const double &end);
  void transformRangeUnit(const int &index, double &startX, double &endX);
  void calculateDetectorPosition(const int &index, double &l1, double &l2,
                                 double &twoTheta);
  int findIndex(const double &value, const MantidVec &vec);
  void RemoveFromEnds(int start, int end, MantidVec &Y, MantidVec &E);
  void RemoveFromMiddle(const int &start, const int &end,
                        const double &startFrac, const double &endFrac,
                        MantidVec &Y, MantidVec &E);

  API::MatrixWorkspace_const_sptr m_inputWorkspace; ///< The input workspace
  double m_startX;                                  ///< The range start point
  double m_endX;                                    ///< The range end point
  Kernel::Unit_sptr m_rangeUnit; ///< The unit in which the above range is given
  bool m_interpolate; ///< Whether removed bins should be interpolated
};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_REMOVEBINS_H_*/
