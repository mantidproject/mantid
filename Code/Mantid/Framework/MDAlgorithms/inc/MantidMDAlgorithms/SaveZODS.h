#ifndef MANTID_MDALGORITHMS_SAVEZODS_H_
#define MANTID_MDALGORITHMS_SAVEZODS_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace MDAlgorithms
{

  /** Save a MDHistoWorkspace to a HDF5 format for use with the ZODS
   * analysis software.
    
    @date 2012-01-27

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
  class DLLExport SaveZODS  : public API::Algorithm
  {
  public:
    SaveZODS();
    virtual ~SaveZODS();
    
    virtual const std::string name() const;
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Save a MDHistoWorkspace in HKL space to a HDF5 format for use with the ZODS analysis software.";}

    virtual int version() const;
    virtual const std::string category() const;

  private:
    void init();
    void exec();


  };


} // namespace MDAlgorithms
} // namespace Mantid

#endif  /* MANTID_MDALGORITHMS_SAVEZODS_H_ */
