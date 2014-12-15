#ifndef MANTID_LIVEDATA_STARTLIVEDATA_H_
#define MANTID_LIVEDATA_STARTLIVEDATA_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidLiveData/LiveDataAlgorithm.h"

namespace Mantid
{
namespace LiveData
{

  /** Algorithm that begins live data monitoring.
   *
   * The algorithm properties specify which instrument to observe,
   * with which method and starting from when.
   *
   * The algorithm will run LoadLiveData ONCE, and return the result
   * of the processing specified.
   *
   * This algorithm will launch MonitorLiveData ASYNCHRONOUSLY.
   * The MonitorLiveData will repeatedly call LoadLiveData at the desired update frequency.
    
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
  class DLLExport StartLiveData  : public LiveDataAlgorithm
  {
  public:
    StartLiveData();
    virtual ~StartLiveData();
    
    virtual const std::string name() const;
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Begin live data monitoring.";}

    virtual int version() const;

  private:
    void init();
    void exec();
    void afterPropertySet(const std::string&);

  };

} // namespace LiveData
} // namespace Mantid

#endif  /* MANTID_LIVEDATA_STARTLIVEDATA_H_ */
