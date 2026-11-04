(CustomAlgorithmDialogs)=

# Custom Algorithm Dialogs

```{contents}
---
local:
---
```

Mantid's graphical code falls into three groups:

- **algorithm dialogs**, which collect the input for a single algorithm and run it, described on this page,
- **interfaces**, which tie many tasks together, see {ref}`MVPDesign` and the {doc}`MVP tutorial <MVPTutorial/index>`, and
- **reusable widgets**, shared between interfaces, see {ref}`ReusableWidgets`.

## Generic and custom dialogs

Most algorithms never need dialog code.
When an algorithm is run from the GUI, `InterfaceManager::createDialog` looks for a dialog registered under the algorithm's name followed by `Dialog`, e.g. `SmoothNeighboursDialog` for `SmoothNeighbours`.
If there is none, it creates a `GenericDialog`, which builds a widget for each property from the property's type, validators and settings.

Write a custom dialog only when the generic one cannot present the algorithm well, for example when the choices offered depend on the selected workspace, or when properties need to be grouped or hidden in ways that property settings cannot express.
The custom dialog replaces the generic one wherever the algorithm is opened from the GUI.

The existing custom dialogs are in `qt/widgets/plugins/algorithm_dialogs`, e.g. `LoadDialog`, `FitDialog`, `SmoothNeighboursDialog`, `SortTableWorkspaceDialog` and `StartLiveDataDialog`.
They are good examples to start from.

## Writing a dialog

A custom dialog is a subclass of `MantidQt::API::AlgorithmDialog` that overrides two methods:

`initLayout()`
: Creates the widgets and connects each one to the algorithm property it sets.

`parseInput()`
: Passes values that are not connected to a single property to the algorithm. This is optional.

The class must be named after the algorithm with `Dialog` appended, and registered with `DECLARE_DIALOG`:

```cpp
// MyAlgDialog.h
#pragma once

#include "MantidQtWidgets/Common/AlgorithmDialog.h"
#include "ui_MyAlgDialog.h"

namespace MantidQt::CustomDialogs {

class MyAlgDialog final : public API::AlgorithmDialog {
  Q_OBJECT

public:
  MyAlgDialog(QWidget *parent = nullptr);

private:
  void initLayout() override;
  void parseInput() override;

  Ui::MyAlgDialog m_form;
};

} // namespace MantidQt::CustomDialogs
```

```cpp
// MyAlgDialog.cpp
#include "MantidQtWidgets/Plugins/AlgorithmDialogs/MyAlgDialog.h"

namespace MantidQt::CustomDialogs {

DECLARE_DIALOG(MyAlgDialog)

MyAlgDialog::MyAlgDialog(QWidget *parent) : API::AlgorithmDialog(parent) {}

void MyAlgDialog::initLayout() {
  m_form.setupUi(this);
  // Connect widgets to the properties they set
  tie(m_form.inputWorkspace, "InputWorkspace", m_form.inputLayout);
  tie(m_form.outputWorkspace, "OutputWorkspace", m_form.outputLayout);
  // Add the standard Run, Cancel and Help buttons
  m_form.dialogLayout->addLayout(createDefaultButtonLayout());
}

void MyAlgDialog::parseInput() {
  // Values built from several widgets are passed explicitly
  storePropertyValue("Columns", m_form.columnList->text());
}

} // namespace MantidQt::CustomDialogs
```

Useful methods provided by `AlgorithmDialog` include:

- `tie(widget, propertyName, ...)`, which connects a widget to a property, shows the validator marker next to it and restores the value used last time,
- `storePropertyValue(name, value)`, which sets a property from `parseInput()`,
- `createDefaultButtonLayout()`, which creates the standard buttons, and
- `addOptionalMessage(layout)`, which shows the optional message passed to the dialog.

A dialog does not have to build every widget itself.
`SmoothNeighboursDialog` uses an `AlgorithmPropertiesWidget`, which creates the same widgets as the generic dialog, and then changes how some of them behave.

## Laying out the dialog

Lay the dialog out in Qt Designer rather than by hand, except for the simplest dialogs.
Give each widget a meaningful object name so it can be used from the code, and save the form as `MyAlgDialog.ui` next to the header.
At build time the form becomes the generated header `ui_MyAlgDialog.h` and the class `Ui::MyAlgDialog`, and `m_form.setupUi(this)` in `initLayout()` creates the widgets.
Reusable widgets, such as `DataSelector` or `FileFinderWidget`, can be added to the form by promoting a placeholder widget in Qt Designer.

{ref}`QtDesignerForPython` describes the same process for Python views.

## Building the dialog

Custom dialogs are built into the `MantidQtWidgetsPluginsAlgorithmDialogs` plugin library, which is installed in Workbench's plugin directory and loaded when Workbench starts.
To add a dialog:

1. put the header in `qt/widgets/plugins/algorithm_dialogs/inc/MantidQtWidgets/Plugins/AlgorithmDialogs`, next to the `.ui` file, and the source file in `qt/widgets/plugins/algorithm_dialogs/src`, and
1. add the source file to `SRC_FILES`, the header to `MOC_FILES` and the form to `UI_FILES` in `qt/widgets/plugins/algorithm_dialogs/CMakeLists.txt`.

`qt/widgets/plugins/algorithm_dialogs/customDialogTemplate.py` writes a skeleton header and source file for a new dialog.
Its include paths and namespaces are from an older layout of the code, so update them to match the example above before building.
