// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MANTIDWIDGETS_MUONFUNCTIONBROWSER_H_
#define MANTID_MANTIDWIDGETS_MUONFUNCTIONBROWSER_H_

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

protected:
  /// Ask user for function and return it
  // QString getUserFunctionFromDialog() override;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif /* MANTID_MANTIDWIDGETS_MUONFUNCTIONBROWSER_H_ */