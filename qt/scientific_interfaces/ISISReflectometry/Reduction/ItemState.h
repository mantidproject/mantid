// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
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

enum class State { NOT_STARTED, STARTING, RUNNING, SUCCESS, ERROR, WARNING };

class MANTIDQT_ISISREFLECTOMETRY_DLL ItemState {
public:
  ItemState();

  std::string message() const;

  void setProgress(double progress, std::string const &message);
  void setStarting();
  void setRunning();
  void setSuccess();
  void setWarning(std::string const &message);
  void setError(std::string const &message);

private:
  State m_state;
  boost::optional<std::string> m_message;
  double m_progress;

  bool requiresMessage() const;
};
}
}
#endif // MANTID_CUSTOMINTERFACE_STATE_H_
