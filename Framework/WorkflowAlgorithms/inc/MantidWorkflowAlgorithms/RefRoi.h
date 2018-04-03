#ifndef MANTID_ALGORITHMS_RefRoi_H_
#define MANTID_ALGORITHMS_RefRoi_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace WorkflowAlgorithms {
/**

    Workflow algorithm for reflectometry to sum up a region of interest on a 2D
   detector.

    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

class DLLExport RefRoi : public API::Algorithm {
public:
  /// Constructor
  RefRoi();
  /// Algorithm's name
  const std::string name() const override { return "RefRoi"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Workflow algorithm for reflectometry to sum up a region of "
           "interest on a 2D detector.";
  }
  /// Algorithm's version
  int version() const override { return (1); }

  /// Algorithm's category for identification
  const std::string category() const override {
    return "Workflow\\Reflectometry";
  }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;

  void extractReflectivity();
  void reverse(API::MatrixWorkspace_sptr WS);
  void extract2D();

  int m_xMin;
  int m_xMax;
  int m_yMin;
  int m_yMax;
  int m_nXPixel;
  int m_nYPixel;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_RefRoi_H_*/
