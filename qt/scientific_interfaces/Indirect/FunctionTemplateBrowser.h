// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef INDIRECT_FUNCTIONTEMPLATEBROWSER_H_
#define INDIRECT_FUNCTIONTEMPLATEBROWSER_H_

#include "DllConfig.h"

#include <QMap>
#include <QWidget>

/* Forward declarations */
class QtProperty;
class QtTreePropertyBrowser;
class QtDoublePropertyManager;
class QtIntPropertyManager;
class QtBoolPropertyManager;
class QtStringPropertyManager;
class QtEnumPropertyManager;
class QtGroupPropertyManager;
class QSettings;

namespace Mantid {
namespace Kernel {
class Property;
}
namespace API {
class IAlgorithm;
}
} // namespace Mantid

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

/**
 * Class FunctionTemplateBrowser implements QtPropertyBrowser to display
 * and set properties that can be used to generate a fit function.
 *
 */
class MANTIDQT_INDIRECT_DLL FunctionTemplateBrowser : public QWidget {
  Q_OBJECT
public:
  FunctionTemplateBrowser(QWidget *parent = nullptr);

private:
  void createBrowser();
  virtual void createProperties() {}

protected:
  QtBoolPropertyManager *m_boolManager;
  QtIntPropertyManager *m_intManager;
  QtDoublePropertyManager *m_doubleManager;
  QtStringPropertyManager *m_stringManager;
  QtEnumPropertyManager *m_enumManager;
  QtGroupPropertyManager *m_groupManager;

  /// Qt property browser which displays properties
  QtTreePropertyBrowser *m_browser;

  /// Precision of doubles in m_doubleManager
  int m_decimals;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /*INDIRECT_FUNCTIONTEMPLATEBROWSER_H_*/
