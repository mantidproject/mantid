// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_SIGNALBLOCKER_H_
#define MANTID_API_SIGNALBLOCKER_H_

#include "DllOption.h"
#include <QObject>

namespace MantidQt {
namespace API {

/** SignalBlocker : RAII signal blocker. Not available in Qt until 5.3
*/
template <typename Type> class EXPORT_OPT_MANTIDQT_COMMON SignalBlocker {

private:
  /// Object to manage blocking
  Type *m_obj;

public:
  /// Constructor
  SignalBlocker(Type *obj);
  /// Destructor
  ~SignalBlocker();
  /// Overriden function like behavior.
  Type *operator->();
  /// Release management
  void release();
};

} // namespace API
} // namespace MantidQt

#endif /* MANTID_API_SIGNALBLOCKER_H_ */
