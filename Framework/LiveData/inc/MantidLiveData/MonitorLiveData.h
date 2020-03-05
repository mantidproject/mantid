// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_LIVEDATA_MONITORLIVEDATA_H_
#define MANTID_LIVEDATA_MONITORLIVEDATA_H_

#include "MantidAPI/Algorithm.h"
#include "MantidLiveData/DllConfig.h"
#include "MantidLiveData/LiveDataAlgorithm.h"

namespace Mantid {
namespace LiveData {

/** Algorithm that repeatedly calls LoadLiveData, at a given
 * update frequency. This is started asynchronously by
 * StartLiveData.

  @date 2012-02-16
*/
class MANTID_LIVEDATA_DLL MonitorLiveData : public LiveDataAlgorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Call LoadLiveData at a given update frequency. Do not call this "
           "algorithm directly; instead call StartLiveData.";
  }

  const std::string category() const override;
  int version() const override;

private:
  void init() override;
  void exec() override;
  void doClone(const std::string &originalName, const std::string &newName);

public:
  /// Latest chunk number loaded
  size_t m_chunkNumber{0};
};

} // namespace LiveData
} // namespace Mantid

#endif /* MANTID_LIVEDATA_MONITORLIVEDATA_H_ */
