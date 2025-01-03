// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/ListPropertyWidget.h"
#include "MantidAPI/IWorkspaceProperty.h"
#include "MantidKernel/ConfigService.h"

#include <QLabel>

#include <sstream>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace MantidQt::API {

//----------------------------------------------------------------------------------------------
/** Destructor
 */
ListPropertyWidget::~ListPropertyWidget() = default;

//----------------------------------------------------------------------------------------------
/** Constructor
 */
ListPropertyWidget::ListPropertyWidget(Mantid::Kernel::Property *prop, QWidget *parent, QGridLayout *layout, int row)
    : PropertyWidget(prop, parent, layout, row) {
  // Label at column 0
  m_label = new QLabel(QString::fromStdString(prop->name()), m_parent);
  m_label->setToolTip(m_doc);
  m_gridLayout->addWidget(m_label, m_row, 0);
  m_widgets.push_back(m_label);

  // It is a choice of certain allowed values and can use a list box
  // Check if this is the row that matches the one that we want to link to the
  // output box
  m_list = new QListWidget(this);
  m_list->setToolTip(m_doc);
  m_list->setSortingEnabled(false);
  m_list->setSelectionMode(QAbstractItemView::ExtendedSelection);
  m_widgets.push_back(m_list);

  std::vector<std::string> items = prop->allowedValues();
  for (auto &item : items) {
    m_list->addItem(QString::fromStdString(item));
  }
  // Make current value visible
  this->setValue(QString::fromStdString(m_prop->value()));

  // Make sure the connection comes after updating any values
  connect(m_list, SIGNAL(itemSelectionChanged()), this, SLOT(userEditedProperty()));

  // Put the combo in column 1
  m_gridLayout->addWidget(m_list, m_row, 1);
}

//----------------------------------------------------------------------------------------------
/** @return the value of the property, as typed in the GUI, as a string */
QString ListPropertyWidget::getValue() const {
  auto selectedItems = m_list->selectedItems();
  std::stringstream ss;
  for (int i = 0; i < selectedItems.size(); ++i) {
    if (i != 0) {
      ss << ",";
    }
    ss << selectedItems[i]->text().toStdString();
  }
  return QString::fromStdString(ss.str());
}

//----------------------------------------------------------------------------------------------
/** Set the value into the GUI
 *
 * @param value :: string representation of the value */
void ListPropertyWidget::setValueImpl(const QString &value) {
  const QString temp = value.isEmpty() ? QString::fromStdString(m_prop->getDefault()) : value;

  auto items = m_list->findItems(temp, Qt::MatchWildcard);
  for (auto item : items) {
    item->setSelected(true);
    m_list->setCurrentItem(item);
    m_list->scrollToItem(item);
  }
}
} // namespace MantidQt::API
