#ifndef MANTID_DATAHANDLING_DOWNLOADINSTRUMENT_H_
#define MANTID_DATAHANDLING_DOWNLOADINSTRUMENT_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

#include <map>

namespace Mantid
{
namespace DataHandling
{
  typedef std::map<std::string,std::string> String2StringMap;

  /** DownloadInstrument : Downloads one or more instrument files to the local instrument cache from the instrument repository

    Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  class DLLExport DownloadInstrument  : public API::Algorithm
  {
  public:
    DownloadInstrument();
    virtual ~DownloadInstrument();
    
    virtual const std::string name() const;
    virtual int version() const;
    virtual const std::string category() const;
    virtual const std::string summary() const;

  private:
    void init();
    void exec();
    virtual int doDownloadFile(const std::string& urlFile, const std::string& localFilePath = "", const String2StringMap& headers = String2StringMap());
    void updateJsonFile(const std::string& directoryPath, const std::string& filePath);
    const std::string MangleFileName(const std::string& filename) const;
    const std::string getDownloadableRepoUrl(const std::string& filename) const;

    String2StringMap processRepository();



    std::string remote_url;

  };


} // namespace DataHandling
} // namespace Mantid

#endif  /* MANTID_DATAHANDLING_DOWNLOADINSTRUMENT_H_ */
