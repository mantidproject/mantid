// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/Python/QHashToDict.h"
#include "MantidQtWidgets/Common/Python/Sip.h"

namespace MantidQt {
namespace Widgets {
namespace Common {
namespace Python {

Python::Dict qHashToDict(const KwArgs &hash) {
  auto mod = PyImport_ImportModule("qtpy.QtCore");
  Py_DECREF(mod);

  auto d = Python::Dict();

  auto it = hash.constBegin();
  auto end = hash.constEnd();
  auto sipAPI = Detail::sipAPI();

  while (it != end) {
    auto *k = new KwArgs::key_type(it.key());
    auto *kobj = sipAPI->api_convert_from_new_type(
        k, sipAPI->api_find_type("QString"), Py_None);

    if (!kobj) {
      delete k;
      return Python::Dict();
    }

    auto *v = new KwArgs::mapped_type(it.value());
    auto *vobj = sipAPI->api_convert_from_new_type(
        v, sipAPI->api_find_type("QVariant"), Py_None);

    if (!vobj) {
      delete v;
      Py_DECREF(kobj);
      return Python::Dict();
    }

    auto rc = PyDict_SetItem(d.ptr(), kobj, vobj);

    Py_DECREF(vobj);
    Py_DECREF(kobj);

    if (rc < 0) {
      return Python::Dict();
    }

    ++it;
  }

  return d;
}

} // namespace Python
} // namespace Common
} // namespace Widgets
} // namespace MantidQt
