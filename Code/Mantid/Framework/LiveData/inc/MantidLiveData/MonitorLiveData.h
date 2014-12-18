#ifndef MANTID_LIVEDATA_MONITORLIVEDATA_H_
#define MANTID_LIVEDATA_MONITORLIVEDATA_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidLiveData/LiveDataAlgorithm.h"

namespace Mantid
{
namespace LiveData
{

  /** Algorithm that repeatedly calls LoadLiveData, at a given
   * update frequency. This is started asynchronously by
   * StartLiveData.
    
    @date 2012-02-16

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
  class DLLExport MonitorLiveData  : public LiveDataAlgorithm
  {
  public:
    MonitorLiveData();
    virtual ~MonitorLiveData();
    
    virtual const std::string name() const;
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Call LoadLiveData at a given update frequency. Do not call this algorithm directly; instead call StartLiveData.";}

    virtual const std::string category() const;
    virtual int version() const;

  private:
    void init();
    void exec();
    void doClone(const std::string & originalName, const std::string & newName);

  public:
    /// Latest chunk number loaded
    size_t m_chunkNumber;

  };


} // namespace LiveData
} // namespace Mantid

#endif  /* MANTID_LIVEDATA_MONITORLIVEDATA_H_ */
