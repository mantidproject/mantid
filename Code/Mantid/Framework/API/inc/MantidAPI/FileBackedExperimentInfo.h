#ifndef MANTID_API_FILEBACKEDEXPERIMENTINFO_H_
#define MANTID_API_FILEBACKEDEXPERIMENTINFO_H_

#include "MantidKernel/System.h"
#include "MantidAPI/ExperimentInfo.h"

#if defined(__GLIBCXX__) && __GLIBCXX__ >= 20100121 // libstdc++-4.4.3
typedef std::unique_ptr< ::NeXus::File> file_holder_type;
#else
typedef std::auto_ptr< ::NeXus::File> file_holder_type;
#endif

namespace Mantid
{
namespace API
{

  /** FileBackedExperimentInfo : TODO: DESCRIPTION

    Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
  class DLLExport FileBackedExperimentInfo : public ExperimentInfo
  {
  public:
    /// Constructor
    FileBackedExperimentInfo(::NeXus::File *file, std::string groupName);
    /// Virtual destructor
    virtual ~FileBackedExperimentInfo();

  private:
     void intialise();
     ::NeXus::File *file;
     std::string groupName;
  };


} // namespace API
} // namespace Mantid

#endif  /* MANTID_API_FILEBACKEDEXPERIMENTINFO_H_ */
