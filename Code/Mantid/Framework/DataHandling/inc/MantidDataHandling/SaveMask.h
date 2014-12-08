#ifndef MANTID_DATAHANDLING_SAVEMASKINGTOFILE_H_
#define MANTID_DATAHANDLING_SAVEMASKINGTOFILE_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace DataHandling
{

  /** SaveMaskingToFile : TODO: DESCRIPTION
    
    @date 2011-11-09

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
  class DLLExport SaveMask : public API::Algorithm
  {
  public:
    SaveMask();
    virtual ~SaveMask();
    
    /// Algorithm's name for identification
    virtual const std::string name() const { return "SaveMask";};
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Save a MaskWorkspace/SpecialWorkspace2D to an XML file.";}

    /// Algorithm's version for identification
    virtual int version() const { return 1;};
    /// Algorithm's category for identification
    virtual const std::string category() const { return "DataHandling;Transforms\\Masking";}

  private:

    /// Define input parameters
    void init();

    /// Main body to execute algorithm
    void exec();

  };


} // namespace DataHandling
} // namespace Mantid

#endif  /* MANTID_DATAHANDLING_SAVEMASKINGTOFILE_H_ */
