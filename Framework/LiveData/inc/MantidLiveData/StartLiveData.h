// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_LIVEDATA_STARTLIVEDATA_H_
#define MANTID_LIVEDATA_STARTLIVEDATA_H_

#include "MantidAPI/Algorithm.h"
#include "MantidLiveData/DllConfig.h"
#include "MantidLiveData/LiveDataAlgorithm.h"

namespace Mantid {
namespace LiveData {

/** Algorithm that begins live data monitoring.
 *
 * The algorithm properties specify which instrument to observe,
 * with which method and starting from when.
 *
 * The algorithm will run LoadLiveData ONCE, and return the result
 * of the processing specified.
 *
 * This algorithm will launch MonitorLiveData ASYNCHRONOUSLY.
 * The MonitorLiveData will repeatedly call LoadLiveData at the desired update
 frequency.

  @date 2012-02-16
*/
class MANTID_LIVEDATA_DLL StartLiveData : public LiveDataAlgorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Begin live data monitoring.";
  }

  int version() const override;

private:
  void init() override;
  void exec() override;
  void afterPropertySet(const std::string &) override;

  void copyListenerProperties(
      const boost::shared_ptr<Mantid::API::ILiveListener> &listener);
  void removeListenerProperties();
};

} // namespace LiveData
} // namespace Mantid

#endif /* MANTID_LIVEDATA_STARTLIVEDATA_H_ */
