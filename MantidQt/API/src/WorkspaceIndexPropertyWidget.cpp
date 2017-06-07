#include "MantidQtAPI/WorkspaceIndexPropertyWidget.h"
#include "MantidAPI/IWorkspacePropertyWithIndex.h"
#include "MantidQtAPI/OptionsPropertyWidget.h"
#include "MantidQtAPI/TextPropertyWidget.h"

namespace MantidQt {
namespace API {

WorkspaceIndexPropertyWidget::WorkspaceIndexPropertyWidget(
    Mantid::Kernel::Property *prop, QWidget *parent, QGridLayout *layout,
    int row)
    : PropertyWidget(prop, parent, layout, row) {
  m_groupBox = new QGroupBox(parent);
  layout->addWidget(m_groupBox, row, 0, 1, 2);

  auto wProp = dynamic_cast<Mantid::API::IWorkspacePropertyWithIndex *>(prop);

  QGridLayout *groupLayout = new QGridLayout(m_groupBox);

  if (prop->allowedValues().size() > 0) {
    m_workspaceWidget =
        new OptionsPropertyWidget(prop, m_groupBox, groupLayout, 0);
  } else {
    m_workspaceWidget =
        new TextPropertyWidget(prop, m_groupBox, groupLayout, 0);
  }

  m_indexTypeWidget = new OptionsPropertyWidget(
      &wProp->mutableIndexTypeProperty(), m_groupBox, groupLayout, 1);
  m_indexListWidget = new TextPropertyWidget(&wProp->mutableIndexListProperty(),
                                             m_groupBox, groupLayout, 2);

  m_widgets.push_back(m_groupBox);
  handleConnections();
}

WorkspaceIndexPropertyWidget::~WorkspaceIndexPropertyWidget() {}

void WorkspaceIndexPropertyWidget::handleConnections() {
  // Disable internal property widget handling
  m_workspaceWidget->getMainWidget()->disconnect();
  m_indexListWidget->getMainWidget()->disconnect();
  m_indexTypeWidget->getMainWidget()->disconnect();

  connect(m_workspaceWidget->getMainWidget(), SIGNAL(currentIndexChanged(int)),
          this, SLOT(userEditedProperty()));
  connect(m_indexTypeWidget->getMainWidget(), SIGNAL(currentIndexChanged(int)),
          this, SLOT(userEditedProperty()));
  connect(m_indexListWidget->getMainWidget(), SIGNAL(editingFinished()), this,
          SLOT(userEditedProperty()));
}

QString WorkspaceIndexPropertyWidget::getValue() const {
  return m_workspaceWidget->getValue() + ";" + m_indexTypeWidget->getValue() +
         ";" + m_indexListWidget->getValue();
}

void WorkspaceIndexPropertyWidget::setValueImpl(const QString &value) {
  auto values = value.split(";");

  if (values.isEmpty()) {
    m_workspaceWidget->setValue("");
    m_indexListWidget->setValue("");
    m_indexTypeWidget->setValue("");
  } else if (values.size() == 3) {
    m_workspaceWidget->setValue(values[0]);
    m_indexListWidget->setValue(values[1]);
    m_indexTypeWidget->setValue(values[2]);
  }
}

} // namespace API
} // namespace MantidQt