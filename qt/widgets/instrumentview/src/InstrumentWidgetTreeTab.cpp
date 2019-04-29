// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/InstrumentView/InstrumentWidgetTreeTab.h"
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#include "MantidQtWidgets/Common/TSVSerialiser.h"
#endif
#include "MantidQtWidgets/InstrumentView/InstrumentActor.h"
#include "MantidQtWidgets/InstrumentView/InstrumentTreeWidget.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidget.h"
#include "MantidQtWidgets/InstrumentView/ProjectionSurface.h"

#include <QMessageBox>
#include <QVBoxLayout>

namespace MantidQt {
namespace MantidWidgets {

InstrumentWidgetTreeTab::InstrumentWidgetTreeTab(InstrumentWidget *instrWidget)
    : InstrumentWidgetTab(instrWidget) {
  QVBoxLayout *layout = new QVBoxLayout(this);
  // Tree Controls
  m_instrumentTree = new InstrumentTreeWidget(nullptr);
  layout->addWidget(m_instrumentTree);
  connect(m_instrumentTree, SIGNAL(componentSelected(size_t)), m_instrWidget,
          SLOT(componentSelected(size_t)));
  connect(m_instrWidget, SIGNAL(requestSelectComponent(QString)), this,
          SLOT(selectComponentByName(QString)));
}

void InstrumentWidgetTreeTab::initSurface() {
  m_instrumentTree->setInstrumentWidget(m_instrWidget);
}

/**
 * Find an instrument component by its name. This is used from the
 * scripting API and errors (component not found) are shown as a
 * message box in the GUI.
 *
 * @param name :: Name of an instrument component.
 */
void InstrumentWidgetTreeTab::selectComponentByName(const QString &name) {
  QModelIndex component = m_instrumentTree->findComponentByName(name);
  if (!component.isValid()) {
    QMessageBox::warning(this, "Instrument Window - Tree Tab - Error",
                         "No component named '" + name +
                             "' was found. "
                             "Please use a valid component name ");
    return;
  }

  m_instrumentTree->clearSelection();
  m_instrumentTree->scrollTo(component, QAbstractItemView::EnsureVisible);
  m_instrumentTree->selectionModel()->select(component,
                                             QItemSelectionModel::Select);
  m_instrumentTree->sendComponentSelectedSignal(component);
}

/**
 * Update surface when tab becomes visible.
 */
void InstrumentWidgetTreeTab::showEvent(QShowEvent * /*unused*/) {
  getSurface()->setInteractionMode(ProjectionSurface::MoveMode);
}

/** Load tree tab state from a Mantid project file
 * @param lines :: lines from the project file to load state from
 */
void InstrumentWidgetTreeTab::loadFromProject(const std::string &lines) {
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  API::TSVSerialiser tsv(lines);

  if (!tsv.selectSection("treetab"))
    return;

  std::string tabLines;
  tsv >> tabLines;
  API::TSVSerialiser tab(tabLines);

  if (tab.selectLine("SelectedComponent")) {
    std::string componentName;
    tab >> componentName;
    selectComponentByName(QString::fromStdString(componentName));
  }

  if (tab.selectLine("ExpandedItems")) {
    auto names = tab.values("ExpandedItems");
    for (auto &name : names) {
      auto qName = QString::fromStdString(name);
      auto index = m_instrumentTree->findComponentByName(qName);
      m_instrumentTree->setExpanded(index, true);
    }
  }
#else
  Q_UNUSED(lines);
  throw std::runtime_error(
      "InstrumentWidgetTreeTab::loadFromProject() not implemented for Qt >= 5");
#endif
}

/** Save the state of the tree tab to a Mantid project file
 * @return a string representing the state of the tree tab
 */
std::string InstrumentWidgetTreeTab::saveToProject() const {
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  API::TSVSerialiser tsv, tab;

  auto index = m_instrumentTree->currentIndex();
  auto model = index.model();

  if (model) {
    auto item = model->data(index);
    auto name = item.value<QString>();
    tab.writeLine("SelectedComponent") << name;
  }

  auto names = m_instrumentTree->findExpandedComponents();
  tab.writeLine("ExpandedItems");
  for (const auto &name : names) {
    tab << name;
  }

  tsv.writeSection("treetab", tab.outputLines());
  return tsv.outputLines();
#else
  throw std::runtime_error(
      "InstrumentWidgetTreeTab::saveToProject() not implemented for Qt >= 5");
#endif
}

} // namespace MantidWidgets
} // namespace MantidQt
