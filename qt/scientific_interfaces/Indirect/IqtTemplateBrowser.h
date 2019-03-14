// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef INDIRECT_IQTTEMPLATEBROWSER_H_
#define INDIRECT_IQTTEMPLATEBROWSER_H_

#include "DllConfig.h"
#include "FunctionTemplateBrowser.h"

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
class MANTIDQT_INDIRECT_DLL IqtTemplateBrowser : public FunctionTemplateBrowser {
  Q_OBJECT
public:
  IqtTemplateBrowser(QWidget *parent = nullptr);

 signals:
   void changedNumberOfExponentials(int n);

protected slots:
  void intChanged(QtProperty *) override;

private:
  void createProperties() override;


  QtProperty *m_exponentialsGroup;
  QtProperty *m_numberOfExponentials;
  QtProperty *m_hasStretchExponential;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /*INDIRECT_IQTTEMPLATEBROWSER_H_*/
