// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#ifndef MPLCPP_QHASHTODICT_H
#define MPLCPP_QHASHTODICT_H

#include "MantidQtWidgets/Common/Python/Object.h"
#include "MantidQtWidgets/Common/Python/Sip.h"

#include <QHash>

namespace MantidQt {
namespace Widgets {
namespace Common {
namespace Python {

using KwArgs = QHash<QString, QVariant>;

Python::Object qHashToDict(const KwArgs &hash) {
  auto *d = PyDict_New();

  if (!d)
    return Python::Object();

  auto it = hash.constBegin();
  auto end = hash.constEnd();
  auto sipAPI = detail::sipAPI();

  while (it != end) {
    auto *k = new KwArgs::key_type(it.key());
    auto *kobj = sipAPI->api_convert_from_new_type(
        k, sipAPI->api_find_type("QString"), Py_None);

    if (!kobj) {
      delete k;
      Py_DECREF(d);

      return Python::Object();
    }

    auto *v = new KwArgs::mapped_type(it.value());
    auto *vobj = sipAPI->api_convert_from_new_type(
        v, sipAPI->api_find_type("QVariant"), Py_None);

    if (!vobj) {
      delete v;
      Py_DECREF(kobj);
      Py_DECREF(d);

      return Python::Object();
    }

    auto rc = PyDict_SetItem(d, kobj, vobj);

    Py_DECREF(vobj);
    Py_DECREF(kobj);

    if (rc < 0) {
      Py_DECREF(d);

      return Python::Object();
    }

    ++it;
  }

  return Python::NewRef(d);
}

} // namespace Python
} // namespace Common
} // namespace Widgets
} // namespace MantidQt

#endif /* MPLCPP_QHASHTODICT_H */