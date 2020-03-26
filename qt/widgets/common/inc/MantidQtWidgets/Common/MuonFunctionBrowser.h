// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "MantidQtWidgets/Common/FunctionBrowser.h"

namespace MantidQt {
namespace MantidWidgets {

/** MuonFunctionBrowser : Subclasses FunctionBrowser for muon-specific use
 */
class EXPORT_OPT_MANTIDQT_COMMON MuonFunctionBrowser : public FunctionBrowser {
  Q_OBJECT

public:
  /// Constructor
  MuonFunctionBrowser(QWidget *parent = nullptr, bool multi = false);
  /// Destructor
  virtual ~MuonFunctionBrowser() override;
};

} // namespace MantidWidgets
} // namespace MantidQt
