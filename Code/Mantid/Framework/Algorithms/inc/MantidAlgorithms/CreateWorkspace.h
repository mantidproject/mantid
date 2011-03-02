#ifndef MANTID_ALGORITHMS_CREATEWORKSPACE_H_
#define MANTID_ALGORITHMS_CREATEWORKSPACE_H_

#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace Algorithms
{
/**
*  CreateWorkspace Algorithm
*
*  This algorithm constructs a MatrixWorkspace when passed a vector for each of the X, Y, and E
*  data values. The unit for the X Axis can optionally be specified as any of the units in the
*  Kernel's UnitFactory.
*
*  Multiple spectra may be created by supplying the NSpec Property (integer, default 1). When this
*  is provided the vectors are split into equal-sized spectra (all X, Y, E values must still be
*  in a single vector for input).
*  
*  @date 20/10/2010
*  @author Michael Whitty
*
*  Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory
*   
*  This file is part of Mantid.
*
*  Mantid is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 3 of the License, or
*  (at your option) any later version.
*    
*  Mantid is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*  File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
*  Code Documentation is available at: <http://doxygen.mantidproject.org>    
*/
class DLLExport CreateWorkspace : public API::Algorithm
{
public:
  /// Default constructor
  CreateWorkspace();
  /// Default desctructor
  virtual ~CreateWorkspace();

  virtual const std::string name() const { return "CreateWorkspace"; } ///< @return the algorithms name
  virtual const std::string category() const { return "General"; } ///< @return the algorithms category
  virtual int version() const { return (1); } ///< @return version number of algorithm

private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  /// Initialise the Algorithm (declare properties)
  void init();
  /// Execute the Algorithm
  void exec();

};

} // namespace Algorithms
} // namespace Mantid
#endif // MANTID_ALGORITHMS_CREATEWORKSPACE_H_
