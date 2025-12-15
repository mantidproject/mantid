// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/AlgorithmPropertiesWidget.h"

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/IWorkspaceProperty.h"
#include "MantidKernel/IPropertySettings.h"
#include "MantidKernel/Property.h"
#include "MantidQtWidgets/Common/PropertyWidget.h"
#include "MantidQtWidgets/Common/PropertyWidgetFactory.h"

#include <QCoreApplication>
#include <QGridLayout>
#include <QGroupBox>
#include <QScrollArea>

#include <algorithm>
#include <utility>

#include <vector>

using namespace Mantid::Kernel;
using Mantid::API::Algorithm_sptr;
using Mantid::API::AlgorithmManager;
using Mantid::API::FrameworkManager;
using Mantid::API::IWorkspaceProperty;

namespace MantidQt::API {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
AlgorithmPropertiesWidget::AlgorithmPropertiesWidget(QWidget *parent)
    : QWidget(parent), m_algoName(""), m_algo(), m_errors(nullptr), m_inputHistory(nullptr) {
  // Create the grid layout that will have all the widgets
  m_inputGrid = new QGridLayout;

  // Create the viewport that holds only the grid layout
  m_viewport = new QWidget(this);

  // Put everything in a vertical box and put it inside the m_scroll area
  auto *mainLay = new QVBoxLayout();
  m_viewport->setLayout(mainLay);
  // The property boxes
  mainLay->addLayout(m_inputGrid);
  // Add a stretchy item to allow the properties grid to be top-aligned
  mainLay->addStretch(1);

  // Create a m_scroll area for the (rare) occasion when an algorithm has
  // so many properties it won't fit on the screen
  m_scroll = new QScrollArea(this);
  m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  m_scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  m_scroll->setWidget(m_viewport);
  m_scroll->setWidgetResizable(true);
  m_scroll->setAlignment(Qt::Alignment(Qt::AlignLeft & Qt::AlignTop));

  // Add a layout for the whole widget, containing just the m_scroll area
  auto *dialog_layout = new QVBoxLayout();
  dialog_layout->addWidget(m_scroll);
  setLayout(dialog_layout);

  this->initLayout();

  // Wide widgets inside the QScrollArea do not cause the dialog to grow
  // and so can be cut off. Force a minimum width
  setObjectName("AlgorithmPropertiesWidget");
  setStyleSheet("#AlgorithmPropertiesWidget {min-width: 25em;}");
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
AlgorithmPropertiesWidget::~AlgorithmPropertiesWidget() = default;

//----------------------------------------------------------------------------------------------
/** Sets the AlgorithmInputHistoryImpl object holding all histories.
 * This object does NOT take ownership
 *
 * @param inputHistory :: AlgorithmInputHistoryImpl ptr
 */
void AlgorithmPropertiesWidget::setInputHistory(MantidQt::API::AbstractAlgorithmInputHistory *inputHistory) {
  m_inputHistory = inputHistory;
}

//----------------------------------------------------------------------------------------------
///@return the algorithm being viewed
Mantid::API::IAlgorithm_sptr AlgorithmPropertiesWidget::getAlgorithm() { return m_algo; }

//----------------------------------------------------------------------------------------------
/** Directly set the algorithm to view. Sets the name to match
 *
 * @param algo :: IAlgorithm bare ptr */
void AlgorithmPropertiesWidget::setAlgorithm(const Mantid::API::IAlgorithm_sptr &algo) {
  if (!algo)
    return;
  saveInput();
  m_algo = algo;
  m_algoName = QString::fromStdString(m_algo->name());
  this->initLayout();
}

//----------------------------------------------------------------------------------------------
///@return the name of the algorithm being displayed
const QString &AlgorithmPropertiesWidget::getAlgorithmName() const { return m_algoName; }

/** Set the algorithm to view using its name
 *
 * @param name :: The algorithm name*/
void AlgorithmPropertiesWidget::setAlgorithmName(QString name) {
  FrameworkManager::Instance();
  m_algoName = std::move(name);
  try {
    Algorithm_sptr alg = AlgorithmManager::Instance().createUnmanaged(m_algoName.toStdString());
    alg->initialize();

    // Set the algorithm ptr. This will redo the layout
    this->setAlgorithm(alg);
  } catch (std::runtime_error &) {
  }
}

//---------------------------------------------------------------------------------------------------------------
/** Sets the properties to force as enabled/disabled */
void AlgorithmPropertiesWidget::addEnabledAndDisableLists(const QStringList &enabled, const QStringList &disabled) {
  this->m_enabled = enabled;
  this->m_disabled = disabled;
}

//---------------------------------------------------------------------------------------------------------------
/** Share the errors map with the parent dialog */
void AlgorithmPropertiesWidget::shareErrorsMap(const QHash<QString, QString> &errors) { this->m_errors = &errors; }

//---------------------------------------------------------------------------------------------------------------
/** @return true if there is any input workspace in the properties list */
bool AlgorithmPropertiesWidget::hasInputWS(const std::vector<Property *> &prop_list) const {
  // For any algorithm that does not have any input workspaces,
  // we do not want to render the 'replace input workspace button'.
  // Do a scan to check.
  std::vector<Property *>::const_iterator pEnd = prop_list.end();
  for (std::vector<Property *>::const_iterator pIter = prop_list.begin(); pIter != pEnd; ++pIter) {
    Property *prop = *pIter;
    if (prop->direction() == Direction::Input && dynamic_cast<IWorkspaceProperty *>(prop)) {
      return true;
    }
  }
  return false;
}

//---------------------------------------------------------------------------------------------------------------
/**
 * Create the layout for this dialog.
 */
void AlgorithmPropertiesWidget::initLayout() {
  if (!getAlgorithm())
    return;

  // Delete all widgets in the layout
  QLayoutItem *child;
  while ((child = m_inputGrid->takeAt(0)) != nullptr) {
    if (child->widget())
      child->widget()->deleteLater();

    delete child;
  }

  // This also deletes the PropertyWidget, which does not actually
  // contain the sub-widgets because they are shared in the grid layout
  for (auto &propWidget : m_propWidgets)
    propWidget->deleteLater();
  QCoreApplication::processEvents();
  m_propWidgets.clear();

  // Create a grid of properties if there are any available
  const std::vector<Property *> &prop_list = getAlgorithm()->getProperties();
  bool hasInputWS_ = hasInputWS(prop_list);

  if (!prop_list.empty()) {
    // Put the property boxes in a grid
    m_currentGrid = m_inputGrid;

    std::string group = "";

    // Each property is on its own row
    int row = 0;

    for (auto prop : prop_list) {
      QString propName = QString::fromStdString(prop->name());

      // Are we entering a new group?
      if (prop->getGroup() != group) {
        group = prop->getGroup();

        if (group == "") {
          // Return to the original grid
          m_currentGrid = m_inputGrid;
        } else {
          // Make a groupbox with a border and a light background
          QGroupBox *grpBox = new QGroupBox(QString::fromStdString(group));
          grpBox->setAutoFillBackground(true);
          grpBox->setStyleSheet("QGroupBox { border: 1px solid gray;  border-radius: 4px; "
                                "font-weight: bold; margin-top: 4px; margin-bottom: 4px; "
                                "padding-top: 16px; }"
                                "QGroupBox::title { background-color: transparent;  "
                                "subcontrol-position: top center;  padding-top:4px; "
                                "padding-bottom:4px; } ");
          QPalette pal = grpBox->palette();
          pal.setColor(grpBox->backgroundRole(), pal.alternateBase().color());
          grpBox->setPalette(pal);

          // Put the frame in the main grid
          m_inputGrid->addWidget(grpBox, row, 0, 1, 4);

          m_groupWidgets[QString::fromStdString(group)] = grpBox;

          // Make a layout in the grp box
          m_currentGrid = new QGridLayout;
          grpBox->setLayout(m_currentGrid);

          row++;
        }
      }

      // Only accept input for output properties or workspace properties
      bool isWorkspaceProp(dynamic_cast<IWorkspaceProperty *>(prop));
      if (prop->direction() == Direction::Output && !isWorkspaceProp)
        continue;

      // Create the appropriate widget at this row in the grid.
      PropertyWidget *widget = PropertyWidgetFactory::createWidget(prop, this, m_currentGrid, row);

      // Set the previous input value, if any
      if (m_inputHistory) {
        QString oldValue = m_inputHistory->previousInput(m_algoName, propName);
        // Empty string if not found. This means use the default value.
        if (!oldValue.isEmpty()) {
          auto error = prop->setValue(oldValue.toStdString());
          // TODO: [Known defect]: this does not match the initialization sequence
          //   in `AlgorithmDialog`.  In the `AlgorithmDialog` case, the 'valueChanged' SIGNAL
          //   will already have been connected at the point the previous values are set.
          //   By implication: this initialization will not initialize properties
          //   with dynamic-default values correctly.
          //   Since at present this clause is not actually executed anywhere in the codebase.
          //   WHEN this clause is used, this issue should be fixed!
          widget->setError(QString::fromStdString(error));
          widget->setPrevious_isDynamicDefault(false);
          widget->setPreviousValue(oldValue);
        }
      }

      m_propWidgets[propName] = widget;

      // Whenever the value changes in the widget, this fires propertyChanged()
      connect(widget, SIGNAL(valueChanged(const QString &)), this, SLOT(propertyChanged(const QString &)));

      // For clicking the "Replace Workspace" button (if any)
      connect(widget, SIGNAL(replaceWorkspaceName(const QString &)), this, SLOT(replaceWSClicked(const QString &)));

      // Only show the "Replace Workspace" button if the algorithm has an input
      // workspace.
      if (hasInputWS_ && !prop->disableReplaceWSButton())
        widget->addReplaceWSButton();

      ++row;
    } //(end for each property)

  } // (there are properties)
}

//--------------------------------------------------------------------------------------
/** SLOT to be called whenever a property's value has just been changed
 * and the widget has lost focus/value has been changed.
 * @param changedPropName :: name of the property that was changed
 */
void AlgorithmPropertiesWidget::propertyChanged(const QString &changedPropName) {
  this->hideOrDisableProperties(changedPropName);
}

bool isCalledInputWorkspaceOrLHSWorkspace(PropertyWidget *const candidate) {
  Mantid::Kernel::Property const *const property = candidate->getProperty();
  const std::string &propertyName = property->name();
  return propertyName == "InputWorkspace" || propertyName == "LHSWorkspace";
}

//-------------------------------------------------------------------------------------------------
/** A slot to handle the replace workspace button click
 *
 * @param propName :: the property for which we clicked "Replace Workspace"
 */
void AlgorithmPropertiesWidget::replaceWSClicked(const QString &propName) {
  if (m_propWidgets.contains(propName)) {
    PropertyWidget *propWidget = m_propWidgets[propName];
    if (propWidget) {
      using CollectionOfPropertyWidget = std::vector<PropertyWidget *>;
      CollectionOfPropertyWidget candidateReplacementSources;
      // Find the name to put in the spot
      for (auto it = m_propWidgets.begin(); it != m_propWidgets.end(); it++) {
        // Only look at workspace properties
        PropertyWidget *otherWidget = it.value();
        Property *prop = it.value()->getProperty();
        const IWorkspaceProperty *wsProp = dynamic_cast<IWorkspaceProperty *>(prop);
        if (otherWidget && wsProp) {
          if (prop->direction() == Direction::Input) {
            // Input workspace property. Get the text typed in.
            QString wsName = otherWidget->getValue();
            if (!wsName.isEmpty()) {
              // Add the candidate to the list of candidates.
              candidateReplacementSources.emplace_back(otherWidget);
            }
          }
        }
      }

      // Choose from candidates, only do this if there are candidates to select
      // from.
      if (candidateReplacementSources.size() > 0) {
        const auto selectedIt = std::find_if(candidateReplacementSources.cbegin(), candidateReplacementSources.cend(),
                                             isCalledInputWorkspaceOrLHSWorkspace);
        if (selectedIt != candidateReplacementSources.end()) {
          // Use the InputWorkspace property called "InputWorkspace" or "LHSWorkspace" as the
          // source for the OutputWorkspace.
          propWidget->setValue((*selectedIt)->getValue());
        } else {
          // Take the first candidate if there are none called "InputWorkspace"
          // as the source for the OutputWorkspace.
          propWidget->setValue(candidateReplacementSources.front()->getValue());
        }
        propWidget->userEditedProperty();
      }
    }
  }
}

//-------------------------------------------------------------------------------------------------
/** Check if the control should be enabled for this property
 * @param prop :: the property to check
 */
bool AlgorithmPropertiesWidget::isWidgetEnabled(const Property *prop) const {
  if (!prop)
    throw std::runtime_error("`AlgorithmPropertiesWidget::isWidgetEnabled` called with null property pointer");
  const std::string &propName = prop->name();

  // Keep things enabled if requested
  if (m_enabled.contains(QString::fromStdString(propName)))
    return true;

  /** The control is disabled if
   *   (1) It is contained in the disabled list or
   *   (2) the property's settings chain indicates it should be disabled.
   */
  if (m_disabled.contains(QString::fromStdString(propName)))
    return false;

  return m_algo->isPropertyEnabled(propName);
}

//-------------------------------------------------------------------------------------------------
/** Compute if the control should be visible for this property based on settings and error state.
 *  WARNING: the GUI itself may override this visibility setting (e.g. if a parent widget is hidden).
 * @param prop :: the property that allows to check for the settings.
 */
bool AlgorithmPropertiesWidget::isWidgetVisible(const Property *prop) const {
  if (!prop)
    throw std::runtime_error("`AlgorithmPropertiesWidget::isWidgetVisible` called with null property pointer");
  const std::string &propName = prop->name();

  bool visible = m_algo->isPropertyVisible(propName);

  // Always show properties that are in an error state
  if (m_errors && !m_errors->value(QString::fromStdString(propName)).isEmpty())
    visible = true;

  return visible;
}

//-------------------------------------------------------------------------------------------------
/** Go through all the properties, and check their settings in order to implement
 * any changes dependent on upstream properties.
 * Then, once any changes have been applied, go through settings and validators again
 * to determine whether properties will be hidden or disabled.
 * At entry to this method: all properties' values must be current.
 * @param changedPropName :: name of the property that was changed
 */
void AlgorithmPropertiesWidget::hideOrDisableProperties(const QString &changedPropName) {
  // Apply `SetValueWhenProperty` or other `IPropertySettings` as appropriate.
  auto const *changedPropWidget = !changedPropName.isEmpty() ? m_propWidgets[changedPropName] : nullptr;
  for (auto &widget : m_propWidgets) {
    Mantid::Kernel::Property *prop = widget->getProperty();
    const QString propName = QString::fromStdString(prop->name());

    auto const &settings = prop->getSettings();
    if (!settings.empty()) {
      // Dynamic PropertySettings objects allow a property to change
      // validators. This removes the old widget and creates a new one
      // instead.

      for (auto const &setting : settings)
        if (setting->isConditionChanged(m_algo.get(), changedPropName.toStdString())) {
          if (setting->applyChanges(m_algo.get(), prop->name())) {
            // WARNING: allow for the possibility that the current property has been replaced inside of `applyChanges`!
            prop = m_algo->getPointerToProperty(propName.toStdString());

            widget->setVisible(false);

            // Create a new widget at the same position in the layout grid:
            //   since widget is a reference, this also replaces the `widget*` in `m_propWidgets`.
            auto *oldWidget = widget;
            int row = widget->getGridRow();
            QGridLayout *layout = widget->getGridLayout();
            widget = PropertyWidgetFactory::createWidget(prop, this, layout, row);
            widget->transferHistoryState(oldWidget, changedPropWidget);

            // Delete the old widget
            oldWidget->deleteLater();

            // Whenever the value changes in the widget, this fires
            // propertyChanged()
            connect(widget, SIGNAL(valueChanged(const QString &)), this, SLOT(propertyChanged(const QString &)));
          }
        }
    }
  } // for each property

  // set Visible and Enabled as appropriate
  for (auto &widget : m_propWidgets) {
    Mantid::Kernel::Property const *prop = widget->getProperty();

    // Set the enabled and visible flags based on what the settings and validators say.
    bool enabled = this->isWidgetEnabled(prop);
    bool visible = this->isWidgetVisible(prop);

    // Hide/disable the widget
    widget->setEnabled(enabled);
    widget->setVisible(visible);
  } // for each property

  this->repaint();
}

//-------------------------------------------------------------------------------------------------
/** When closing or changing algorithm, this saves the input
 * history to QSettings
 */
void AlgorithmPropertiesWidget::saveInput() {
  if (m_inputHistory) {
    for (auto pitr = m_propWidgets.begin(); pitr != m_propWidgets.end(); ++pitr) {
      PropertyWidget const *widget = pitr.value();
      auto const *prop = widget->getProperty();
      QString const &propName = pitr.key();

      // Normalize default values to empty string.
      QString value = (prop->isDefault() || prop->isDynamicDefault() ? "" : widget->getValue());

      // save the value
      m_inputHistory->storeNewValue(m_algoName, QPair<QString, QString>(propName, value));
    }
  }
}

} // namespace MantidQt::API
