#ifndef MANTID_LIVEDATA_IKAFKASTREAMSUBSCRIBER_H_
#define MANTID_LIVEDATA_IKAFKASTREAMSUBSCRIBER_H_

#include "MantidKernel/System.h"
#include <string>

namespace Mantid {
namespace LiveData {

/**
  Interface for classes that subscribe to Kafka streams.

  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

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
class DLLExport IKafkaStreamSubscriber {
public:
  virtual ~IKafkaStreamSubscriber() = default;
  virtual void subscribe() = 0;
  virtual void subscribe(int64_t offset) = 0;
  virtual void consumeMessage(std::string *message) = 0;
};

} // namespace LiveData
} // namespace Mantid

#endif /* MANTID_LIVEDATA_IKAFKASTREAMSUBSCRIBER_H_ */
