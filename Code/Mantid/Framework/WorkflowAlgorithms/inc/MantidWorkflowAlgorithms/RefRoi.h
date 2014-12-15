#ifndef MANTID_ALGORITHMS_RefRoi_H_
#define MANTID_ALGORITHMS_RefRoi_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace WorkflowAlgorithms
{
/**

    Workflow algorithm for reflectometry to sum up a region of interest on a 2D detector.

    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

class DLLExport RefRoi : public API::Algorithm
{
public:
  /// (Empty) Constructor
  RefRoi() : API::Algorithm() {}
  /// Virtual destructor
  virtual ~RefRoi() {}
  /// Algorithm's name
  virtual const std::string name() const { return "RefRoi"; }
  ///Summary of algorithms purpose
  virtual const std::string summary() const {return "Workflow algorithm for reflectometry to sum up a region of interest on a 2D detector.";}
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "Workflow\\Reflectometry"; }

private:
  /// Initialisation code
  void init();
  /// Execution code
  void exec();

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
