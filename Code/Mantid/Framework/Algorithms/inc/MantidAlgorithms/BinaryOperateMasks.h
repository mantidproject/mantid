#ifndef MANTID_ALGORITHMS_BINARYOPERATEMASKS_H_
#define MANTID_ALGORITHMS_BINARYOPERATEMASKS_H_
    
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace Algorithms
{

  /** BinaryOperateMasks : TODO: DESCRIPTION

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
  class DLLExport BinaryOperateMasks : public API::Algorithm
  {
  public:
    BinaryOperateMasks();
    ~BinaryOperateMasks();
    
    /// Algorithm's name for identification overriding a virtual method
    virtual const std::string name() const { return "BinaryOperateMasks";};
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Performs binary operation, including and, or and xor, on two mask Workspaces, i.e., SpecialWorkspace2D.";}

    /// Algorithm's version for identification overriding a virtual method
    virtual int version() const { return 1;};
    /// Algorithm's category for identification overriding a virtual method
    virtual const std::string category() const { return "Transforms\\Masking";}

  private:
    
    /// Initialisation code
    void init();
    /// Execution code
    void exec();

  };


} // namespace Algorithms
} // namespace Mantid

#endif  /* MANTID_ALGORITHMS_BINARYOPERATEMASKS_H_ */
