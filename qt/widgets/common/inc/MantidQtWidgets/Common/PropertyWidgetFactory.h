// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_PROPERTYWIDGETFACTORY_H_
#define MANTID_API_PROPERTYWIDGETFACTORY_H_

#include "MantidKernel/System.h"

class QGridLayout;
class QWidget;

namespace Mantid {
namespace Kernel {
class Property;
}
} // namespace Mantid

namespace MantidQt {
namespace API {
class PropertyWidget;
/** PropertyWidgetFactory : TODO: DESCRIPTION

  @date 2012-02-17
*/
class DLLExport PropertyWidgetFactory {
public:
  PropertyWidgetFactory();
  virtual ~PropertyWidgetFactory();

  static PropertyWidget *createWidget(Mantid::Kernel::Property *prop,
                                      QWidget *parent = nullptr,
                                      QGridLayout *layout = nullptr,
                                      int row = -1);
};

} // namespace API
} // namespace MantidQt

#endif /* MANTID_API_PROPERTYWIDGETFACTORY_H_ */
