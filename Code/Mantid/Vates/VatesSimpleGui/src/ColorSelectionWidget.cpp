#include "ColorSelectionWidget.h"

#include <pqChartValue.h>
#include <pqColorMapModel.h>
#include <pqColorPresetManager.h>
#include <pqColorPresetModel.h>

#include <QDoubleValidator>

ColorSelectionWidget::ColorSelectionWidget(QWidget *parent) : QWidget(parent)
{
  this->ui.setupUi(this);
  this->presets = new pqColorPresetManager(this);
  this->presets->restoreSettings();

  this->loadBuiltinColorPresets();

  this->ui.maxValLineEdit->setValidator(new QDoubleValidator(this));
  this->ui.minValLineEdit->setValidator(new QDoubleValidator(this));

  QObject::connect(this->ui.autoColorScaleCheckBox, SIGNAL(stateChanged(int)),
      this, SLOT(autoOrManualScaling(int)));
  QObject::connect(this->ui.presetButton, SIGNAL(clicked()),
                   this, SLOT(loadPreset()));
}

void ColorSelectionWidget::loadBuiltinColorPresets()
{
  pqColorMapModel colorMap;
  pqColorPresetModel *model = this->presets->getModel();
  colorMap.setColorSpace(pqColorMapModel::DivergingSpace);
  colorMap.addPoint(pqChartValue((double)0.0), QColor( 59, 76, 192), 0.0);
  colorMap.addPoint(pqChartValue((double)1.0), QColor(180,  4,  38), 1.0);
  colorMap.setNanColor(QColor(63, 0, 0));
  model->addBuiltinColorMap(colorMap, "Cool to Warm");

  colorMap.removeAllPoints();
  colorMap.setColorSpace(pqColorMapModel::HsvSpace);
  colorMap.addPoint(pqChartValue((double)0.0), QColor(0, 0, 255), (double)0.0);
  colorMap.addPoint(pqChartValue((double)1.0), QColor(255, 0, 0), (double)0.0);
  colorMap.setNanColor(QColor(127, 127, 127));
  model->addBuiltinColorMap(colorMap, "Blue to Red Rainbow");

  colorMap.removeAllPoints();
  colorMap.setColorSpace(pqColorMapModel::HsvSpace);
  colorMap.addPoint(pqChartValue((double)0.0), QColor(255, 0, 0), (double)0.0);
  colorMap.addPoint(pqChartValue((double)1.0), QColor(0, 0, 255), (double)1.0);
  colorMap.setNanColor(QColor(127, 127, 127));
  model->addBuiltinColorMap(colorMap, "Red to Blue Rainbow");

  colorMap.removeAllPoints();
  colorMap.setColorSpace(pqColorMapModel::RgbSpace);
  colorMap.addPoint(pqChartValue((double)0.0), QColor(0,   0,   0  ), (double)0.0);
  colorMap.addPoint(pqChartValue((double)1.0), QColor(255, 255, 255), (double)1.0);
  colorMap.setNanColor(QColor(255, 0, 0));
  model->addBuiltinColorMap(colorMap, "Grayscale");

  colorMap.removeAllPoints();
  colorMap.setColorSpace(pqColorMapModel::RgbSpace);
  colorMap.addPoint(pqChartValue((double)0.0), QColor( 10,  10, 242), (double)0.0);
  colorMap.addPoint(pqChartValue((double)1.0), QColor(242, 242,  10), (double)1.0);
  colorMap.setNanColor(QColor(255, 0, 0));
  model->addBuiltinColorMap(colorMap, "Blue to Yellow");

  colorMap.removeAllPoints();
  colorMap.setColorSpace(pqColorMapModel::RgbSpace);
  colorMap.addPoint(pqChartValue((double)0.0), QColor(0,   0,   0  ), (double)0.0);
  colorMap.addPoint(pqChartValue((double)0.4), QColor(230, 0,   0  ), (double)0.4);
  colorMap.addPoint(pqChartValue((double)0.8), QColor(230, 230, 0  ), (double)0.8);
  colorMap.addPoint(pqChartValue((double)1.0), QColor(255, 255, 255), (double)1.0);
  colorMap.setNanColor(QColor(0, 127, 255));
  model->addBuiltinColorMap(colorMap, "Black-Body Radiation");

  colorMap.removeAllPoints();
  colorMap.setColorSpace(pqColorMapModel::LabSpace);
  colorMap.addPoint(pqChartValue((double)0.0), QColor(0, 153, 191), (double)0.0);
  colorMap.addPoint(pqChartValue((double)1.0), QColor(196, 119, 87),(double)1.0);
  colorMap.setNanColor(QColor(255, 255, 0));
  model->addBuiltinColorMap(colorMap, "CIELab Blue to Red");
}

void ColorSelectionWidget::autoOrManualScaling(int state)
{
  switch (state)
  {
  case Qt::Unchecked:
    this->ui.maxValLabel->setEnabled(true);
    this->ui.maxValLineEdit->setEnabled(true);
    this->ui.minValLabel->setEnabled(true);
    this->ui.minValLineEdit->setEnabled(true);
    break;
  case Qt::Checked:
    this->ui.maxValLabel->setEnabled(false);
    this->ui.maxValLineEdit->setEnabled(false);
    this->ui.minValLabel->setEnabled(false);
    this->ui.minValLineEdit->setEnabled(false);
    break;
  }
}

void ColorSelectionWidget::loadPreset()
{
    this->presets->setUsingCloseButton(false);
    if (this->presets->exec() == QDialog::Accepted)
    {

    }
}
