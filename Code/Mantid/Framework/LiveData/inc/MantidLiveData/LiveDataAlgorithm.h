#ifndef MANTID_LIVEDATA_LIVEDATAALGORITHM_H_
#define MANTID_LIVEDATA_LIVEDATAALGORITHM_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidAPI/ILiveListener.h"

namespace Mantid
{
namespace LiveData
{

  /** Abstract base class with common properties
   * for the following algorithms dealing with live data:
   * - StartLiveData
   * - LoadLiveData
   * - MonitorLiveData
    
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
  class DLLExport LiveDataAlgorithm  : public API::Algorithm
  {
  public:
    LiveDataAlgorithm();
    virtual ~LiveDataAlgorithm();
    virtual const std::string category() const;

    void copyPropertyValuesFrom(const LiveDataAlgorithm & other);

    Mantid::API::ILiveListener_sptr getLiveListener();
    void setLiveListener(Mantid::API::ILiveListener_sptr listener);

    virtual std::map<std::string, std::string> validateInputs();

  protected:
    void initProps();

    Mantid::Kernel::DateAndTime getStartTime() const;

    Mantid::API::IAlgorithm_sptr makeAlgorithm(bool postProcessing);

    bool hasPostProcessing() const;

    /// Live listener
    Mantid::API::ILiveListener_sptr m_listener;

  };


} // namespace LiveData
} // namespace Mantid

#endif  /* MANTID_LIVEDATA_LIVEDATAALGORITHM_H_ */
