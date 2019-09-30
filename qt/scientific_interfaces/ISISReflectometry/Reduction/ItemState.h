// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#ifndef MANTID_CUSTOMINTERFACES_STATE_H_
#define MANTID_CUSTOMINTERFACES_STATE_H_
#include "Common/DllConfig.h"
#include <boost/optional.hpp>
#include <string>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

enum class State {
  ITEM_NOT_STARTED = 0,
  ITEM_STARTING = 1,
  ITEM_RUNNING = 2,
  ITEM_COMPLETE = 3,
  ITEM_ERROR = 4,
  ITEM_WARNING = 5
};

/** @class ItemState

    The ItemState class provides information about the processing state of an
    item (i.e. row or group) the the runs table
 */
class MANTIDQT_ISISREFLECTOMETRY_DLL ItemState {
public:
  ItemState();
  explicit ItemState(State state);

  State state() const;
  std::string message() const;
  double progress() const;

  void setProgress(double progress, std::string const &message);
  void setStarting();
  void setRunning();
  void setSuccess();
  void setWarning(std::string const &message);
  void setError(std::string const &message);
  void reset();

private:
  State m_state;
  boost::optional<std::string> m_message;
  double m_progress;

  bool requiresMessage() const;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_CUSTOMINTERFACE_STATE_H_
