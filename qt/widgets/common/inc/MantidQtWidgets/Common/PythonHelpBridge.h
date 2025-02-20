// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/DllOption.h"
#include <string>

namespace MantidQt {
namespace MantidWidgets {

class EXPORT_OPT_MANTIDQT_COMMON PythonHelpBridge {
public:
  PythonHelpBridge();
  void showHelpPage(const std::string &relative_url, const std::string &local_docs = "",
                    const std::string &online_base_url = "https://docs.mantidproject.org/");
};

} // namespace MantidWidgets
} // namespace MantidQt
