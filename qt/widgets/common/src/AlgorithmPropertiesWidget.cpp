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
    : QWidget(parent), m_algoName(""), m_algo(), m_inputHistory(nullptr) {
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
QString AlgorithmPropertiesWidget::getAlgorithmName() const { return m_algoName; }

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
/** @return true if the workspace has an input workspace */
bool haveInputWS(const std::vector<Property *> &prop_list) {
  // For the few algorithms (mainly loading) that do not have input workspaces,
  // we do not
  // want to render the 'replace input workspace button'. Do a quick scan to
  // check.
  // Also the ones that don't have a set of allowed values as input workspace
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
  bool hasInputWS = haveInputWS(prop_list);

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
        // Empty string if not found. This means use the default.
        if (!oldValue.isEmpty()) {
          auto error = prop->setValue(oldValue.toStdString());
          widget->setError(QString::fromStdString(error));
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
      if (hasInputWS && !prop->disableReplaceWSButton())
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
      QString wsName("");
      for (auto it = m_propWidgets.begin(); it != m_propWidgets.end(); it++) {
        // Only look at workspace properties
        PropertyWidget *otherWidget = it.value();
        Property *prop = it.value()->getProperty();
        IWorkspaceProperty *wsProp = dynamic_cast<IWorkspaceProperty *>(prop);
        if (otherWidget && wsProp) {
          if (prop->direction() == Direction::Input) {
            // Input workspace property. Get the text typed in.
            wsName = otherWidget->getValue();
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
 * @param property :: the property that allows to check for the settings.
 * @param propName :: The name of the property
 */
bool AlgorithmPropertiesWidget::isWidgetEnabled(Property *property, const QString &propName) const {
  // To avoid errors
  if (propName.isEmpty())
    return true;
  if (!property)
    return true;

  // Keep things enabled if requested
  if (m_enabled.contains(propName))
    return true;

  /** The control is disabled if
   *   (1) It is contained in the disabled list or
   *   (2) A user passed a value into the dialog
   */
  if (m_disabled.contains(propName)) {
    return false;
  } else {
    // Regular C++ algo. Let the property tell us,
    // possibly using validators, if it is to be shown enabled
    if (property->getSettings())
      return property->getSettings()->isEnabled(m_algo.get());
    else
      return true;
  }
}

//-------------------------------------------------------------------------------------------------
/** Go through all the properties, and check their validators to determine
 * whether they should be made disabled/invisible.
 * It also shows/hids the validators.
 * All properties' values should be set already, otherwise the validators
 * will be running on old data.
 * @param changedPropName :: name of the property that was changed
 */
void AlgorithmPropertiesWidget::hideOrDisableProperties(const QString &changedPropName) {
  // SetValueWhenProperty as appropriate
  for (auto &widget : m_propWidgets) {
    Mantid::Kernel::Property *prop = widget->getProperty();
    IPropertySettings *settings = prop->getSettings();

    if (settings) {
      // Dynamic PropertySettings objects allow a property to change
      // validators. This removes the old widget and creates a new one
      // instead.
      if (settings->isConditionChanged(m_algo.get(), changedPropName.toStdString())) {
        settings->applyChanges(m_algo.get(), prop);

        // Delete the old widget
        int row = widget->getGridRow();
        QGridLayout *layout = widget->getGridLayout();
        widget->setVisible(false);
        widget->deleteLater();

        // Create the appropriate widget at this row in the grid.
        widget = PropertyWidgetFactory::createWidget(prop, this, layout, row);

        // Whenever the value changes in the widget, this fires
        // propertyChanged()
        connect(widget, SIGNAL(valueChanged(const QString &)), this, SLOT(propertyChanged(const QString &)));
      }
    }
  } // for each property

  // set Visible and Enabled as appropriate
  for (auto &widget : m_propWidgets) {
    Mantid::Kernel::Property *prop = widget->getProperty();
    IPropertySettings *settings = prop->getSettings();
    const auto &propName = QString::fromStdString(prop->name());

    // Set the enabled and visible flags based on what the validators say.
    // Default is always true.
    bool visible = true;
    // Dynamically check if the widget should be enabled.
    bool enabled = this->isWidgetEnabled(prop, propName);

    // Do we have a custom IPropertySettings?
    if (settings) {
      // Set the visible flag
      visible = settings->isVisible(m_algo.get());
    }

    // Show/hide the validator label (that red star)
    QString error = "";
    if (m_errors.contains(propName))
      error = m_errors[propName];
    // Always show controls that are in error
    if (error.length() != 0)
      visible = true;

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
      PropertyWidget *widget = pitr.value();
      const QString &propName = pitr.key();
      QString value = widget->getValue();
      //        Mantid::Kernel::Property *prop = widget->getProperty();
      //        if (!prop || prop->remember())
      m_inputHistory->storeNewValue(m_algoName, QPair<QString, QString>(propName, value));
    }
  }
}

} // namespace MantidQt::API
