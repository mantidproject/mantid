// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ILiveListener.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidLiveData/DllConfig.h"

#include <string>
#include <vector>

namespace Mantid {
namespace LiveData {

/** Abstract base class with common properties
 * for the following algorithms dealing with live data:
 * - StartLiveData
 * - LoadLiveData
 * - MonitorLiveData

  @date 2012-02-16
*/
class MANTID_LIVEDATA_DLL LiveDataAlgorithm : public API::Algorithm {
public:
  const std::string category() const override;

  void copyPropertyValuesFrom(const LiveDataAlgorithm &other);

  /// The facility that 'Instrument' is resolved against: 'Facility' if given, else the Mantid default.
  const Kernel::FacilityInfo &facility() const;

  /// Instrument names, in `facility()`, that have a live listener configured.
  std::vector<std::string> liveListenerInstruments() const;

  Mantid::API::ILiveListener_sptr getLiveListener(bool start = true);
  Mantid::API::ILiveListener_sptr createLiveListener(bool connect = false);
  void setLiveListener(Mantid::API::ILiveListener_sptr listener);

  std::map<std::string, std::string> validateInputs() override;

protected:
  ~LiveDataAlgorithm() override = default;
  void initProps();

  Mantid::Types::Core::DateAndTime getStartTime() const;

  Mantid::API::IAlgorithm_sptr makeAlgorithm(bool postProcessing);

  bool hasPostProcessing() const;

  /// Live listener
  Mantid::API::ILiveListener_sptr m_listener;
};

} // namespace LiveData
} // namespace Mantid
