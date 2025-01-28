// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidParallel/DllConfig.h"
#include "MantidParallel/IO/EventsListsShmemManager.h"
#include <vector>

namespace ip = boost::interprocess;

namespace Mantid {
namespace Parallel {
namespace IO {

/** EventsListsShmemStorage : NOT an !!!OWNER of shared memory!!!
  Shared memory segments are detached and can be fined by name.
  The concern of this class is allocating shared memory and
  naming segments, all other operations are on the base class:
  EventsListsShmemManager.

  @author Igor Gudich
  @date 2018
*/
class MANTID_PARALLEL_DLL EventsListsShmemStorage : public EventsListsShmemManager {
public:
  EventsListsShmemStorage(const std::string &segmentName, const std::string &elName, size_t size, size_t chunksCnt,
                          size_t pixelsCount);
  virtual ~EventsListsShmemStorage() override = default;

  void reserve(std::size_t chunkN, std::size_t pixelN, std::size_t size);

  MANTID_PARALLEL_DLL friend std::ostream &operator<<(std::ostream &os, const EventsListsShmemStorage &storage);
};

} // namespace IO
} // namespace Parallel
} // namespace Mantid
