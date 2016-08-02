#include "MantidQtAPI/SignalBlocker.h"
#include <QAction>
#include <QPushButton>
#include <QComboBox>
#include <stdexcept>

namespace MantidQt {
namespace API {

/**
 * Constructor
 * @param obj : QObject to block signals for.
 */
template <typename Type>
SignalBlocker<Type>::SignalBlocker(Type *obj)
    : m_obj(obj) {
  if (m_obj == NULL) {
    throw std::runtime_error("Object to block is NULL");
  }
  m_obj->blockSignals(true);
}

/** Destructor
 */
template <typename Type> SignalBlocker<Type>::~SignalBlocker() {
  // Release blocking if possible
  if (m_obj != NULL) {
    m_obj->blockSignals(false);
  }
}

template <typename Type> Type *SignalBlocker<Type>::operator->() {
  if (m_obj != NULL) {
    return m_obj;
  } else {
    throw std::runtime_error("SignalBlocker cannot access released object");
  }
}

template <typename Type> void SignalBlocker<Type>::release() { m_obj = NULL; }

// Template instances we need.
template class EXPORT_OPT_MANTIDQT_API SignalBlocker<QObject>;
template class EXPORT_OPT_MANTIDQT_API SignalBlocker<QAction>;
template class EXPORT_OPT_MANTIDQT_API SignalBlocker<QPushButton>;
template class EXPORT_OPT_MANTIDQT_API SignalBlocker<QComboBox>;

} // namespace API
} // namespace Mantid
