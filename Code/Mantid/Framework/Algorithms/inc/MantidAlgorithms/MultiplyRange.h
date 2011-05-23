#ifndef MANTID_ALGORITHMS_MULTIPLYRANGE_H_
#define MANTID_ALGORITHMS_MULTIPLYRANGE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace Algorithms
{
/** An algorithm to multiply a range of bins in a workspace by the factor given.

    Required Properties:
    <UL>
    <LI> InputWorkspace  - The name of the input workspace</LI>
    <LI> OutputWorkspace - The name of the output workspace</LI>
    <LI> StartBin        - The bin index at the start of the range</LI>
    <LI> EndBin          - The bin index at the end of the range</LI>
    <LI> Factor          - The value by which to multiply the input data range</LI>
    </UL>

    @author Robert Dalgliesh, ISIS, RAL
    @date 12/1/2010

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class DLLExport MultiplyRange : public API::Algorithm
{
public:
  ///no arg constructor
  MultiplyRange() : API::Algorithm() {}
  ///virtual destructor
  virtual ~MultiplyRange() {}

  virtual const std::string name() const { return "MultiplyRange";}
  virtual int version() const { return (1);}
  virtual const std::string category() const { return "CorrectionFunctions";}

private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  ///Initialisation code
  void init();
  ///Execution code
  void exec();

  size_t m_startBin; ///< start bin
  size_t m_endBin; ///< end bin
  double m_factor; ///<factor
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_MULTIPLYRANGE_H_*/
