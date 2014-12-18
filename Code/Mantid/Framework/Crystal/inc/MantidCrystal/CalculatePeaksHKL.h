#ifndef MANTID_CRYSTAL_CALCULATEPEAKSHKL_H_
#define MANTID_CRYSTAL_CALCULATEPEAKSHKL_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"


namespace Mantid
{
namespace Crystal
{

  /** CalculatePeaksHKL : Calculate the HKL value for each peak without any rounding or optimization
   *  of the UB Matrix.
    
    Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
  class DLLExport CalculatePeaksHKL  : public API::Algorithm
  {
  public:
    CalculatePeaksHKL();
    virtual ~CalculatePeaksHKL();
    
    virtual const std::string name() const;
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Calculates Miller indices for each peak. No rounding or UB optimization.";}

    virtual int version() const;
    virtual const std::string category() const;

  private:
    void init();
    void exec();


  };


} // namespace Crystal
} // namespace Mantid

#endif  /* MANTID_CRYSTAL_CALCULATEPEAKSHKL_H_ */
