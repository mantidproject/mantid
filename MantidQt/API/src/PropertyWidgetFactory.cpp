#include "MantidQtAPI/PropertyWidgetFactory.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/IWorkspacePropertyWithIndex.h"
#include "MantidAPI/MultipleFileProperty.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/System.h"
#include "MantidQtAPI/BoolPropertyWidget.h"
#include "MantidQtAPI/FilePropertyWidget.h"
#include "MantidQtAPI/OptionsPropertyWidget.h"
#include "MantidQtAPI/TextPropertyWidget.h"
#include "MantidQtAPI/WorkspaceIndexPropertyWidget.h"
#include <MantidQtAPI/ListPropertyWidget.h>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace MantidQt {
namespace API {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
PropertyWidgetFactory::PropertyWidgetFactory() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
PropertyWidgetFactory::~PropertyWidgetFactory() {}

//----------------------------------------------------------------------------------------------
/** Create the appropriate PropertyWidget for the given Property
 *
 * @param prop :: property for the widget
 * @param parent :: parent widget
 * @param layout :: QGridLayout of the parent, in the case of the GenericDialog
 * @param row :: row in the above QGridLayout, if specified
 * @return the right PropertyWidget * subclass
 */
PropertyWidget *
PropertyWidgetFactory::createWidget(Mantid::Kernel::Property *prop,
                                    QWidget *parent, QGridLayout *layout,
                                    int row) {
  auto *fileType = dynamic_cast<Mantid::API::FileProperty *>(prop);
  auto *multipleFileType =
      dynamic_cast<Mantid::API::MultipleFileProperty *>(prop);
  auto *boolProp = dynamic_cast<PropertyWithValue<bool> *>(prop);
  auto *wProp = dynamic_cast<IWorkspacePropertyWithIndex *>(prop);

  if (wProp) {
    return new WorkspaceIndexPropertyWidget(prop, parent, layout, row);
  } else if (boolProp) {
    // CheckBox shown for BOOL properties
    return new BoolPropertyWidget(boolProp, parent, layout, row);
  } else if (!prop->allowedValues().empty() && !fileType && !multipleFileType) {
    if (prop->isMultipleSelectionAllowed()) {
      // Check if there are only certain allowed values for the property
      return new ListPropertyWidget(prop, parent, layout, row);
    } else {
      // Check if there are only certain allowed values for the property
      return new OptionsPropertyWidget(prop, parent, layout, row);
    }
  } else {
    if (fileType || multipleFileType) {
      return new FilePropertyWidget(prop, parent, layout, row);
    } else {
      // Generic text property
      return new TextPropertyWidget(prop, parent, layout, row);
    }
  }
}

} // namespace API
} // namespace MantidQt
