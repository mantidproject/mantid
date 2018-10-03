// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_LIVEDATA_LIVEDATAALGORITHM_H_
#define MANTID_LIVEDATA_LIVEDATAALGORITHM_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ILiveListener.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace LiveData {

/** Abstract base class with common properties
 * for the following algorithms dealing with live data:
 * - StartLiveData
 * - LoadLiveData
 * - MonitorLiveData

  @date 2012-02-16
*/
class DLLExport LiveDataAlgorithm : public API::Algorithm {
public:
  const std::string category() const override;

  void copyPropertyValuesFrom(const LiveDataAlgorithm &other);

  Mantid::API::ILiveListener_sptr getLiveListener(bool start = true);
  Mantid::API::ILiveListener_sptr createLiveListener(bool connect = false);
  void setLiveListener(Mantid::API::ILiveListener_sptr listener);

  std::map<std::string, std::string> validateInputs() override;

protected:
  void initProps();

  Mantid::Types::Core::DateAndTime getStartTime() const;

  Mantid::API::IAlgorithm_sptr makeAlgorithm(bool postProcessing);

  bool hasPostProcessing() const;

  /// Live listener
  Mantid::API::ILiveListener_sptr m_listener;
};

} // namespace LiveData
} // namespace Mantid

#endif /* MANTID_LIVEDATA_LIVEDATAALGORITHM_H_ */
