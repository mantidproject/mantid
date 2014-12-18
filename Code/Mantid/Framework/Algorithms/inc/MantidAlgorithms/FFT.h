#ifndef MANTID_ALGORITHM_FFT_H_
#define MANTID_ALGORITHM_FFT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/Workspace.h"

namespace Mantid
{
namespace Algorithms
{

/** Performs a Fast Fourier Transform of data

    @author Roman Tolchenov
    @date 07/07/2009

    Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
class DLLExport FFT : public API::Algorithm
{
public:
  /// Default constructor
  FFT() : API::Algorithm() {};
  /// Destructor
  virtual ~FFT() {};
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "FFT";}
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Performs complex Fast Fourier Transform";}

  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1;}
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "Arithmetic\\FFT";}

private:
  
  // Overridden Algorithm methods
  void init();
  void exec();

};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_FFT_H_*/
