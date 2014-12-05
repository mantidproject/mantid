#ifndef MANTID_ALGORITHMS_SIGNALOVERERROR_H_
#define MANTID_ALGORITHMS_SIGNALOVERERROR_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/UnaryOperation.h"

namespace Mantid
{
namespace Algorithms
{

  /** Calculate Y/E for a Workspace2D
    
    @date 2011-12-05

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
  class DLLExport SignalOverError  : public UnaryOperation
  {
  public:
    SignalOverError();
    virtual ~SignalOverError();
    
    virtual const std::string name() const;
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Replace Y by Y/E for a MatrixWorkspace";}

    virtual int version() const;
    virtual const std::string category() const;

  private:


    // Overridden UnaryOperation methods
    void performUnaryOperation(const double XIn, const double YIn, const double EIn, double& YOut, double& EOut);



  };


} // namespace Algorithms
} // namespace Mantid

#endif  /* MANTID_ALGORITHMS_SIGNALOVERERROR_H_ */
