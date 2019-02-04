// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_LIVELISTENER_H_
#define MANTID_API_LIVELISTENER_H_

#include "MantidAPI/ILiveListener.h"

namespace Mantid {
namespace API {
/**
  Base implementation for common behaviour of all live listener classes. It
  implements the ILiveListener interface.
*/
class DLLExport LiveListener : public API::ILiveListener {
public:
  bool dataReset() override;
  void setSpectra(const std::vector<specnum_t> &specList) override;
  void setAlgorithm(const class IAlgorithm &callingAlgorithm) override;

protected:
  /// Indicates receipt of a reset signal from the DAS.
  bool m_dataReset = false;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_LIVELISTENER_H_ */
