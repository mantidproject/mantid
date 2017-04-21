#include <cfloat>

#include "MantidKernel/ConfigService.h"
#include "MantidQtAPI/MdConstants.h"
#include "MantidQtAPI/TSVSerialiser.h"
#include "MantidVatesAPI/ColorScaleGuard.h"
#include "MantidVatesSimpleGuiViewWidgets/ColorSelectionWidget.h"

#include "pqPresetDialog.h"
#include "vtk_jsoncpp.h"
#include "vtkSMTransferFunctionPresets.h"
#include "vtkNew.h"

#include <QDir>
#include <QDoubleValidator>
#include <QFile>
#include <QFileInfo>

namespace Mantid {
namespace Vates {
namespace SimpleGui {

/**
 * This is the class constructor. It sets up the UI and all the necessary
 * sub-components and connections.
 * @param parent the parent widget of the mode control widget
 */
ColorSelectionWidget::ColorSelectionWidget(QWidget *parent)
    : QWidget(parent), m_minHistoric(0.01), m_maxHistoric(0.01),
      m_ignoreColorChangeCallbacks(false),
      m_inProcessUserRequestedAutoScale(false), m_colorScaleLock(nullptr) {
  this->m_ui.setupUi(this);
  this->m_ui.autoColorScaleCheckBox->setChecked(true);
  this->setEditorStatus(false);

  this->loadBuiltinColorPresets();

  m_minValidator = new QDoubleValidator(this);
  m_maxValidator = new QDoubleValidator(this);

  this->m_ui.maxValLineEdit->setValidator(m_minValidator);
  this->m_ui.minValLineEdit->setValidator(m_maxValidator);

  // note the clicked() signals, not stateChanged(), beware of programmatic
  // state changes
  // comming from user events from other sources (vtk callbacks and signals from
  // the
  // Paraview color map editor)
  QObject::connect(this->m_ui.autoColorScaleCheckBox, SIGNAL(clicked(bool)),
                   this, SLOT(autoCheckBoxClicked(bool)));

  QObject::connect(this->m_ui.presetButton, SIGNAL(clicked()), this,
                   SLOT(loadPreset()));

  QObject::connect(this->m_ui.minValLineEdit, SIGNAL(editingFinished()), this,
                   SLOT(getColorScaleRange()));

  QObject::connect(this->m_ui.maxValLineEdit, SIGNAL(editingFinished()), this,
                   SLOT(getColorScaleRange()));

  QObject::connect(this->m_ui.useLogScaleCheckBox, SIGNAL(clicked(bool)), this,
                   SLOT(useLogScalingClicked(bool)));
}

/**
 * This function sets the status of the color selection min/max widgets. It
 * doesn't modify the check boxes.
 *
 * @param status the state to set the color selection widgets to
 */
void ColorSelectionWidget::setEditorStatus(bool status) {
  this->m_ui.maxValLabel->setEnabled(status);
  this->m_ui.maxValLineEdit->setEnabled(status);
  this->m_ui.minValLabel->setEnabled(status);
  this->m_ui.minValLineEdit->setEnabled(status);
}

/**
 * This function sets up various color maps.
 */
void ColorSelectionWidget::loadBuiltinColorPresets() {
  // the destructor of vtkSMTransferFunctionPresets copies these colormaps to
  // the vtkSMSettings singleton.
  vtkNew<vtkSMTransferFunctionPresets> presets;
  // Check for colormap "hot". If preset, assume custom colormaps have
  // already been loaded.
  auto colorMapName = presets->GetFirstPresetWithName("hot");
  if (colorMapName.empty()) {
    const std::string filenames[3] = {"All_slice_viewer_cmaps_for_vsi.json",
                                      "All_idl_cmaps.json",
                                      "All_mpl_cmaps.json"};
    const std::string colorMapDirectory =
        Kernel::ConfigService::Instance().getString("colormaps.directory");
    for (const auto &baseName : filenames) {
      std::string colorMap = colorMapDirectory + baseName;
      presets->ImportPresets(colorMap.c_str());
    }
  }
}

/**
 * Load the default color map
 * @param viewSwitched Flag if the view has switched or not.
 */
void ColorSelectionWidget::loadColorMap(bool viewSwitched) {

  QString defaultColorMap;

  // If the view has switched or the VSI is loaded use the last color map
  // index
  if (viewSwitched) {
    defaultColorMap = m_mdSettings.getLastSessionColorMap();
  } else {
    // Check if the user wants a general MD color map
    if (m_mdSettings.getUsageGeneralMdColorMap()) {
      // The name is sufficient for the VSI to find the color map
      defaultColorMap = m_mdSettings.getGeneralMdColorMapName();
    } else {
      // Check if the user wants to use the last session
      if (m_mdSettings.getUsageLastSession()) {
        defaultColorMap = m_mdSettings.getLastSessionColorMap();
      } else {
        defaultColorMap = m_mdSettings.getUserSettingColorMap();
      }
    }
  }

  Mantid::VATES::ColorScaleLockGuard guard(m_colorScaleLock);
  pqPresetDialog preset(this, pqPresetDialog::SHOW_NON_INDEXED_COLORS_ONLY);
  preset.setCurrentPreset(defaultColorMap.toStdString().c_str());
  Json::Value colorMap = preset.currentPreset();
  if (!colorMap.empty())
    this->onApplyPreset(colorMap);
}

/**
 * Changes the status of the autoScaling checkbox. This is in
 * principle meant to be used programmatically, when for example the
 * color map is edited elsewhere (like in the Paraview color editor).
 *
 * @param autoScale whether to set auto scaling (on/off)
 */
void ColorSelectionWidget::setAutoScale(bool autoScale) {
  this->m_ui.autoColorScaleCheckBox->setChecked(autoScale);
  this->setEditorStatus(!autoScale);
}

/**
 * Simply set the min and max values for the color scale.
 *
 * @param max maximum value (corresponding to the max line edit)
 * @param min minimum value (corresponding to the max line edit)
 */
void ColorSelectionWidget::setMinMax(double &min, double &max) {
  setMinSmallerMax(min, max);
}

/**
 * To prevent updates from external callbacks, useful for example
 * when switching. This is set to true before starting view update
 * or switch operations, and then to false to re-enable user
 * requested updates.
 *
 * @param ignore whether callbacks should be ignored.
 */
void ColorSelectionWidget::ignoreColorChangeCallbacks(bool ignore) {
  m_ignoreColorChangeCallbacks = ignore;
}

/**
 * Get the current state as for ignoring callbacks from color changes.
 *
 * @return whether this is currently ignoring color change callbacks.
 */
bool ColorSelectionWidget::isIgnoringColorCallbacks() {
  return m_ignoreColorChangeCallbacks;
}

/**
 * This slot enables or diables the min and max line edits based on
 * the (changing) state of the automatic scaling checkbox. It should
 * be used for click events on the auto-scale check box.
 *
 * @param wasOn current checked state (before the user clicks)
 */
void ColorSelectionWidget::autoCheckBoxClicked(bool wasOn) {
  m_inProcessUserRequestedAutoScale = true;
  if (!wasOn) {
    this->setEditorStatus(true);
    emit this->autoScale(this);
  } else {
    this->setEditorStatus(false);
    emit this->autoScale(this);
  }
  m_inProcessUserRequestedAutoScale = false;
}

/**
 * This function presents the user with the available color presets (maps) and
 * gets the selection result from the user.
 */
void ColorSelectionWidget::loadPreset() {
  Mantid::VATES::ColorScaleLockGuard guard(m_colorScaleLock);
  pqPresetDialog preset(this, pqPresetDialog::SHOW_NON_INDEXED_COLORS_ONLY);
  preset.setCustomizableLoadColors(false, false);
  preset.setCustomizableLoadOpacities(false, false);
  preset.setCustomizableUsePresetRange(false, false);
  preset.setCustomizableLoadAnnotations(false, false);
  preset.setCurrentPreset(
      m_mdSettings.getLastSessionColorMap().toAscii().constData());
  this->connect(&preset, SIGNAL(applyPreset(const Json::Value &)), this,
                SLOT(onApplyPreset(const Json::Value &)));
  preset.exec();
}

/**
 * This function gets the new color scale range from the value widgets and
 * passes a signal along with that new range.
 */
void ColorSelectionWidget::getColorScaleRange() {
  if (this->m_ui.useLogScaleCheckBox->isChecked()) {
    setupLogScale(true);
  } else {
    setupLogScale(false);
  }

  double min = this->m_ui.minValLineEdit->text().toDouble();
  double max = this->m_ui.maxValLineEdit->text().toDouble();

  emit this->colorScaleChanged(min, max);
}

/**
 * This function sets the color scale range into the range widgets.
 * @param min the minimum value of the color scale range
 * @param max the maximum value of the color scale range
 */
void ColorSelectionWidget::setColorScaleRange(double min, double max) {
  if (this->m_ui.autoColorScaleCheckBox->isChecked()) {
    m_minHistoric = min;
    m_maxHistoric = max;

    this->m_ui.minValLineEdit->clear();
    this->m_ui.minValLineEdit->insert(QString::number(min));
    this->m_ui.maxValLineEdit->clear();
    this->m_ui.maxValLineEdit->insert(QString::number(max));
  } else {
    this->getColorScaleRange();
  }
}

/**
 * This slot sets the flag for using log color scaling based on the
 * associated checkbox. It should be used for click events on the
 * use-log-scaling check box.
 *
 * @param on clicked-state of use-log-scaling
 */
void ColorSelectionWidget::useLogScalingClicked(bool on) {
  // Set up values for with or without log scale
  getColorScaleRange();

  emit this->logScale(on);
}

void ColorSelectionWidget::onApplyPreset(const Json::Value &value) {
  std::string presetName = value["Name"].asString();
  if (!presetName.empty()) {
    m_mdSettings.setLastSessionColorMap(QString::fromStdString(presetName));
  }
  emit this->colorMapChanged(value);
}

/**
 * This slot sets the flag for using log color scaling based on the
 * associated checkbox.
 * @param state flag for whether or not to use log color scaling
 */
void ColorSelectionWidget::useLogScaling(int state) {
  // Qt::Checked is 2, need it to be 1 for boolean true conversion
  if (Qt::Checked == state) {
    state -= 1;
  }

  // Set up values for with or without log scale
  getColorScaleRange();

  emit this->logScale(state);
}

/**
 * Set up the min and max values and validators for use with or without log
 * scaling
 * @param state The state of the log scale, where 0 is no log scale
 */
void ColorSelectionWidget::setupLogScale(int state) {
  // Get the min and max values
  double min = this->m_ui.minValLineEdit->text().toDouble();
  double max = this->m_ui.maxValLineEdit->text().toDouble();

  // Make sure that the minimum is smaller or equal to the maximum
  setMinSmallerMax(min, max);

  // If we switched to a log state make sure that values are larger than 0
  if (state) {
    if (min <= 0) {
      min = m_mdConstants.getLogScaleDefaultValue();
    }

    if (max <= 0) {
      max = m_mdConstants.getLogScaleDefaultValue();
    }
  }

  // If min and max were changed we need to persist this
  setMinSmallerMax(min, max);

  // Set the validators
  if (state) {
    m_maxValidator->setBottom(0.0);
    m_minValidator->setBottom(0.0);
  } else {
    m_maxValidator->setBottom(-DBL_MAX);
    m_minValidator->setBottom(-DBL_MAX);
  }
}

/**
 * Slot to set the checkbox if the logscaling behaviour has been set
 * programatically
 * @param state Flag whether the checkbox should be checked or not
 */
void ColorSelectionWidget::onSetLogScale(bool state) {
  m_ui.useLogScaleCheckBox->setChecked(state);
}

/**
 * Make sure that min is smaller/equal than max. If not then set to the old
 * value.
 * @param max The maximum value.
 * @param min The minimum value.
 */
void ColorSelectionWidget::setMinSmallerMax(double &min, double &max) {
  if (min <= max) {
    m_minHistoric = min;
    m_maxHistoric = max;
  } else {
    min = m_minHistoric;
    max = m_maxHistoric;
  }

  this->m_ui.minValLineEdit->clear();
  this->m_ui.minValLineEdit->insert(QString::number(min));
  this->m_ui.maxValLineEdit->clear();
  this->m_ui.maxValLineEdit->insert(QString::number(max));
}

/**
 * This function sets the state for all of the controls on the color selection
 * widget.
 * @param state the boolean to set the controls' state to
 */
void ColorSelectionWidget::enableControls(bool state) {
  this->m_ui.colorSelectionLabel->setEnabled(state);
  this->m_ui.autoColorScaleCheckBox->setEnabled(state);
  this->m_ui.useLogScaleCheckBox->setEnabled(state);
  int cbstate = this->m_ui.autoColorScaleCheckBox->isChecked();
  if (state) {
    switch (cbstate) {
    case Qt::Unchecked:
      this->setEditorStatus(true);
      break;
    case Qt::Checked:
      this->setEditorStatus(false);
      break;
    }
  } else {
    this->setEditorStatus(false);
  }
  this->m_ui.presetButton->setEnabled(state);
}

/**
 * This function returns the state of the automatic color scaling.
 * Since a checkbox is used, the checked state is actually 2, so it needs
 * to be decremented to cast to a boolean.
 * @return the state of automatic color scaling
 */
bool ColorSelectionWidget::getAutoScaleState() const {
  int state = this->m_ui.autoColorScaleCheckBox->isChecked();
  if (Qt::Checked == state) {
    state -= 1;
  }
  return static_cast<bool>(state);
}

/**
 * This function returns the state of the logarithmic color scaling.
 * Since a checkbox is used, the checked state is actually 2, so it needs
 * to be decremented to cast to a boolean.
 * @return the state of logarithmic color scaling
 */
bool ColorSelectionWidget::getLogScaleState() const {
  int state = this->m_ui.useLogScaleCheckBox->isChecked();
  if (Qt::Checked == state) {
    state -= 1;
  }

  return static_cast<bool>(state);
}

/**
 * This function returns the minimum range value for the color scaling.
 * @return current minimum color scaling value
 */
double ColorSelectionWidget::getMinRange() const {
  return this->m_ui.minValLineEdit->text().toDouble();
}

/**
 * This function returns the maximum range value for the color scaling.
 * @return current maximum color scaling value
 */
double ColorSelectionWidget::getMaxRange() const {
  return this->m_ui.maxValLineEdit->text().toDouble();
}

/**
 * This function returns the color selection widget to its original state.
 * This means that automatic color scaling is on, log scaling is off and
 * the color range line edits are empty.
 */
void ColorSelectionWidget::reset() {
  this->m_ui.autoColorScaleCheckBox->setChecked(true);
  this->m_ui.useLogScaleCheckBox->setChecked(false);
  this->m_ui.minValLineEdit->setText("");
  this->m_ui.maxValLineEdit->setText("");
}

/**
 * Set the color scale lock
 * @param lock
 */
void ColorSelectionWidget::setColorScaleLock(
    Mantid::VATES::ColorScaleLock *lock) {
  if (!m_colorScaleLock) {
    m_colorScaleLock = lock;
  }
}

/**
 * Is the color selection widget locked or not
 */
bool ColorSelectionWidget::isColorScaleLocked() const {
  if (m_colorScaleLock) {
    return m_colorScaleLock->isLocked();
  } else {
    return false;
  }
}

std::string ColorSelectionWidget::saveToProject() const {
  using namespace MantidQt::API;
  TSVSerialiser tsv;
  tsv.writeLine("Min") << getMinRange();
  tsv.writeLine("Max") << getMaxRange();
  tsv.writeLine("AutoScale") << getAutoScaleState();
  tsv.writeLine("LogScale") << getLogScaleState();
  return tsv.outputLines();
}

void ColorSelectionWidget::loadFromProject(const std::string &lines) {
  using namespace MantidQt::API;
  TSVSerialiser tsv(lines);
  bool autoScale, logScale;
  double min, max;

  tsv.selectLine("AutoScale");
  tsv >> autoScale;
  tsv.selectLine("LogScale");
  tsv >> logScale;
  tsv.selectLine("Min");
  tsv >> min;
  tsv.selectLine("Max");
  tsv >> max;

  reset();

  m_ui.autoColorScaleCheckBox->setChecked(autoScale);
  m_ui.useLogScaleCheckBox->setChecked(logScale);
  m_ui.minValLineEdit->setText(QString::number(min));
  m_ui.maxValLineEdit->setText(QString::number(max));
}

} // SimpleGui
} // Vates
} // Mantid
