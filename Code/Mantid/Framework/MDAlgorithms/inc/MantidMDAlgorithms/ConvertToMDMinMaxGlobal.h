#ifndef MANTID_MDALGORITHMS_CONVERTTOMD_MINMAXGLOBAL_H_
#define MANTID_MDALGORITHMS_CONVERTTOMD_MINMAXGLOBAL_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace MDAlgorithms
{

  /** ConvertToMDMinMaxGlobal : Algorithm to calculate limits for ConvertToMD transformation which can be observed using an instrument which covers whole MD-space 
      The current version knows 
    
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
  class DLLExport ConvertToMDMinMaxGlobal  : public API::Algorithm
  {
  public:
    ConvertToMDMinMaxGlobal();
    virtual ~ConvertToMDMinMaxGlobal();
    
    virtual const std::string name() const;
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Calculate limits for ConvertToMD transformation, achievable on a spheric instrument.";}

    virtual int version() const;
    virtual const std::string category() const;

  private:
    void init();
    void exec();


  };


} // namespace MDAlgorithms
} // namespace Mantid

#endif  /* MANTID_MDALGORITHMS_CONVERTTOMDHELPER_H_ */
