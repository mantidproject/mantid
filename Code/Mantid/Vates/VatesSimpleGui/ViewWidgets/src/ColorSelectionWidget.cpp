#include "MantidVatesSimpleGuiViewWidgets/ColorSelectionWidget.h"

#include "MantidKernel/ConfigService.h"
#include "MantidQtAPI/MdConstants.h"

#include <pqBuiltinColorMaps.h>
#include <pqChartValue.h>
#include <pqColorMapModel.h>
#include <pqColorPresetManager.h>
#include <pqColorPresetModel.h>
#include <vtkPVXMLElement.h>
#include <vtkPVXMLParser.h>

#include <QDir>
#include <QDoubleValidator>
#include <QFile>
#include <QFileInfo>

#include <iostream>
#include <cfloat>

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{

/**
 * This is the class constructor. It sets up the UI and all the necessary
 * sub-components and connections.
 * @param parent the parent widget of the mode control widget
 */
  ColorSelectionWidget::ColorSelectionWidget(QWidget *parent) : QWidget(parent), m_minHistoric(0.01), m_maxHistoric(0.01)
{
  this->ui.setupUi(this);
  this->ui.autoColorScaleCheckBox->setChecked(true);
  this->setEditorStatus(false);

  this->presets = new pqColorPresetManager(this);
  this->presets->restoreSettings();

  this->loadBuiltinColorPresets();

  m_minValidator = new QDoubleValidator(this);
  m_maxValidator = new QDoubleValidator(this);

  this->ui.maxValLineEdit->setValidator(m_minValidator);
  this->ui.minValLineEdit->setValidator(m_maxValidator);

  QObject::connect(this->ui.autoColorScaleCheckBox, SIGNAL(stateChanged(int)),
  this, SLOT(autoOrManualScaling(int)));
  QObject::connect(this->ui.presetButton, SIGNAL(clicked()),
  this, SLOT(loadPreset()));
  QObject::connect(this->ui.minValLineEdit, SIGNAL(editingFinished()),
  this, SLOT(getColorScaleRange()));
  QObject::connect(this->ui.maxValLineEdit, SIGNAL(editingFinished()),
  this, SLOT(getColorScaleRange()));
  QObject::connect(this->ui.useLogScaleCheckBox, SIGNAL(stateChanged(int)),
                   this, SLOT(useLogScaling(int)));
}

/**
 * This function sets the status of the color selection widgets.
 * @param status the state to set the color selection widgets to
 */
void ColorSelectionWidget::setEditorStatus(bool status)
{
  this->ui.maxValLabel->setEnabled(status);
  this->ui.maxValLineEdit->setEnabled(status);
  this->ui.minValLabel->setEnabled(status);
  this->ui.minValLineEdit->setEnabled(status);
}

/**
 * This function sets up various color maps. This is copied verbaitum from
 * pqColorScaleEditor.
 */
void ColorSelectionWidget::loadBuiltinColorPresets()
{
  pqColorPresetModel *presetModel = this->presets->getModel();

  // get builtin color maps xml
  const char *xml = pqComponentsGetColorMapsXML();

  // create xml parser
  vtkPVXMLParser *xmlParser = vtkPVXMLParser::New();
  xmlParser->InitializeParser();
  xmlParser->ParseChunk(xml, static_cast<unsigned>(strlen(xml)));
  xmlParser->CleanupParser();

  this->addColorMapsFromXML(xmlParser, presetModel);

  // Add color maps from IDL and Matplotlib
  this->addColorMapsFromFile("All_idl_cmaps.xml", xmlParser, presetModel);
  this->addColorMapsFromFile("All_mpl_cmaps.xml", xmlParser, presetModel);

  // cleanup parser
  xmlParser->Delete();
}

/**
 * This function takes color maps from a XML file, parses them and loads and
 * adds them to the color preset model.
 * @param fileName : The file with color maps to parse.
 * @param parser : The XML parser with the color maps.
 * @param model : The color preset model to add the maps too.
 */
void ColorSelectionWidget::addColorMapsFromFile(std::string fileName,
                                                vtkPVXMLParser *parser,
                                                pqColorPresetModel *model)
{
  std::string colorMapDir = Kernel::ConfigService::Instance().getString("colormaps.directory");
  if (!colorMapDir.empty())
  {
    QFileInfo cmaps(QDir(QString::fromStdString(colorMapDir)),
                    QString::fromStdString(fileName));
    if (cmaps.exists())
    {
      parser->SetFileName(cmaps.absoluteFilePath().toStdString().c_str());
      parser->Parse();
      this->addColorMapsFromXML(parser, model);
    }
  }
}

/**
 * This function takes a XML parser and loads and adds color maps to the color
 * preset model.
 * @param parser : The XML parser with the color maps.
 * @param model : The color preset model to add the maps too.
 */
void ColorSelectionWidget::addColorMapsFromXML(vtkPVXMLParser *parser,
                                               pqColorPresetModel *model)
{
  // parse each color map element
  vtkPVXMLElement *root = parser->GetRootElement();
  for(unsigned int i = 0; i < root->GetNumberOfNestedElements(); i++)
  {
    vtkPVXMLElement *colorMapElement = root->GetNestedElement(i);
    if(std::string("ColorMap") != colorMapElement->GetName())
    {
      continue;
    }

    // load color map from its XML
    pqColorMapModel colorMap =
        pqColorPresetManager::createColorMapFromXML(colorMapElement);
    QString name = colorMapElement->GetAttribute("name");

    // add color map to the model
    model->addBuiltinColorMap(colorMap, name);
  }
}

/**
 * This function enables or diables the min and max line edits based on state
 * of the automatic scaling checkbox.
 * @param state the current state of the checkbox
 */
void ColorSelectionWidget::autoOrManualScaling(int state)
{
  switch (state)
  {
  case Qt::Unchecked:
    this->setEditorStatus(true);
    emit this->autoScale(this);
    break;
  case Qt::Checked:
    this->setEditorStatus(false);
    emit this->autoScale(this);
    break;
  }
}

/**
 * This function presents the user with the available color presets (maps) and
 * gets the selection result from the user.
 */
void ColorSelectionWidget::loadPreset()
{
  this->presets->setUsingCloseButton(false);
  if (this->presets->exec() == QDialog::Accepted)
  {
    // Get the color map from the selection.
    QItemSelectionModel *selection = this->presets->getSelectionModel();
    QModelIndex index = selection->currentIndex();
    const pqColorMapModel *colorMap = this->presets->getModel()->getColorMap(index.row());
    if (colorMap)
    {
      emit this->colorMapChanged(colorMap);
    }
  }
}

/**
 * This function gets the new color scale range from the value widgets and
 * passes a signal along with that new range.
 */
void ColorSelectionWidget::getColorScaleRange()
{
  if (this->ui.useLogScaleCheckBox->isChecked())
  {
    setupLogScale(true);
  }
  else
  {
    setupLogScale(false);
  }

  double min = this->ui.minValLineEdit->text().toDouble();
  double max = this->ui.maxValLineEdit->text().toDouble();

  emit this->colorScaleChanged(min, max);
}

/**
 * This function sets the color scale range into the range widgets.
 * @param min the minimum value of the color scale range
 * @param max the maximum value of the color scale range
 */
void ColorSelectionWidget::setColorScaleRange(double min, double max)
{
  if (this->ui.autoColorScaleCheckBox->isChecked())
  {
    m_minHistoric = min;
    m_maxHistoric = max;

    this->ui.minValLineEdit->clear();
    this->ui.minValLineEdit->insert(QString::number(min));
    this->ui.maxValLineEdit->clear();
    this->ui.maxValLineEdit->insert(QString::number(max));
  }
  else
  {
    this->getColorScaleRange();
  }
}

/**
 * This function sets the flag for using log color scaling based on the
 * associated checkbox.
 * @param state flag for whether or not to use log color scaling
 */
void ColorSelectionWidget::useLogScaling(int state)
{
  // Qt::Checked is 2, need it to be 1 for boolean true conversion
  if (Qt::Checked == state)
  {
    state -= 1;
  }

  // Set up values for with or without log scale
  getColorScaleRange();

  emit this->logScale(state);
}

/**
 * Set up the min and max values and validators for use with or without log scaling
 * @param state The state of the log scale, where 0 is no log scale
 */
void ColorSelectionWidget::setupLogScale(int state)
{
  // Get the min and max values
  double min = this->ui.minValLineEdit->text().toDouble();
  double max = this->ui.maxValLineEdit->text().toDouble();

  // Make sure that the minimum is smaller or equal to the maximum
  setMinSmallerMax(min, max);

  // If we switched to a log state make sure that values are larger than 0
  if (state)
  {
    if (min <= 0 )
    {
      min = m_mdConstants.getLogScaleDefaultValue();
    }

    if (max <= 0)
    {
      max = m_mdConstants.getLogScaleDefaultValue();
    }
  }

  // If min and max were changed we need to persist this
  setMinSmallerMax(min, max);

  // Set the validators
  if (state)
  {
    m_maxValidator->setBottom(0.0);
    m_minValidator->setBottom(0.0);
  }
  else
  {
    m_maxValidator->setBottom(-DBL_MAX);
    m_minValidator->setBottom(-DBL_MAX);
  }
}

/**
 * Slot to set the checkbox if the logscaling behaviour has been set programatically
 * @param state Flag whether the checkbox should be checked or not
 */
void ColorSelectionWidget::onSetLogScale(bool state)
{
    ui.useLogScaleCheckBox->setChecked(state);
}

/**
 * Make sure that min is smaller/equal than max. If not then set to the old value.
 * @param max The maximum value.
 * @param min The minimum value.
 */
void ColorSelectionWidget::setMinSmallerMax(double& min, double& max)
{
  if (min <= max)
  {
    m_minHistoric = min;
    m_maxHistoric = max;
  }
  else
  {
    min = m_minHistoric;
    max = m_maxHistoric;
  }

  this->ui.minValLineEdit->clear();
  this->ui.minValLineEdit->insert(QString::number(min));
  this->ui.maxValLineEdit->clear();
  this->ui.maxValLineEdit->insert(QString::number(max));
}

/**
 * This function sets the state for all of the controls on the color selection
 * widget.
 * @param state the boolean to set the controls' state to
 */
void ColorSelectionWidget::enableControls(bool state)
{
  this->ui.colorSelectionLabel->setEnabled(state);
  this->ui.autoColorScaleCheckBox->setEnabled(state);
  this->ui.useLogScaleCheckBox->setEnabled(state);
  int cbstate = this->ui.autoColorScaleCheckBox->isChecked();
  if (state)
  {
    switch (cbstate)
    {
    case Qt::Unchecked:
      this->setEditorStatus(true);
      break;
    case Qt::Checked:
      this->setEditorStatus(false);
      break;
    }
  }
  else
  {
    this->setEditorStatus(false);
  }
  this->ui.presetButton->setEnabled(state);
}

/**
 * This function returns the state of the automatic color scaling.
 * Since a checkbox is used, the checked state is actually 2, so it needs
 * to be decremented to cast to a boolean.
 * @return the state of automatic color scaling
 */
bool ColorSelectionWidget::getAutoScaleState()
{
  int state = this->ui.autoColorScaleCheckBox->isChecked();
  if (Qt::Checked == state)
  {
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
bool ColorSelectionWidget::getLogScaleState()
{
  int state = this->ui.useLogScaleCheckBox->isChecked();
  if (Qt::Checked == state)
  {
    state -= 1;
  }

  return static_cast<bool>(state);
}

/**
 * This function returns the minimum range value for the color scaling.
 * @return current minimum color scaling value
 */
double ColorSelectionWidget::getMinRange()
{
  return this->ui.minValLineEdit->text().toDouble();
}

/**
 * This function returns the maximum range value for the color scaling.
 * @return current maximum color scaling value
 */
double ColorSelectionWidget::getMaxRange()
{
  return this->ui.maxValLineEdit->text().toDouble();
}

/**
 * This function returns the color selection widget to its original state.
 * This means that automatic color scaling is on, log scaling is off and
 * the color range line edits are empty.
 */
void ColorSelectionWidget::reset()
{
  this->ui.autoColorScaleCheckBox->setChecked(true);
  this->ui.useLogScaleCheckBox->setChecked(false);
  this->ui.minValLineEdit->setText("");
  this->ui.maxValLineEdit->setText("");
}

} // SimpleGui
} // Vates
} // Mantid
