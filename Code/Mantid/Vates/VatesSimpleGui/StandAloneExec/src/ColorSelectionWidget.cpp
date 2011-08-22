#include "ColorSelectionWidget.h"

#include <pqChartValue.h>
#include <pqColorMapModel.h>
#include <pqColorPresetManager.h>
#include <pqColorPresetModel.h>

#include <QDoubleValidator>

#include <iostream>
ColorSelectionWidget::ColorSelectionWidget(QWidget *parent) : QWidget(parent)
{
  this->ui.setupUi(this);
  this->ui.autoColorScaleCheckBox->setChecked(true);
  this->setEditorStatus(false);

  this->presets = new pqColorPresetManager(this);
  this->presets->restoreSettings();

  this->loadBuiltinColorPresets();

  this->ui.maxValLineEdit->setValidator(new QDoubleValidator(this));
  this->ui.minValLineEdit->setValidator(new QDoubleValidator(this));

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

void ColorSelectionWidget::setEditorStatus(bool status)
{
  this->ui.maxValLabel->setEnabled(status);
  this->ui.maxValLineEdit->setEnabled(status);
  this->ui.minValLabel->setEnabled(status);
  this->ui.minValLineEdit->setEnabled(status);
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
    this->setEditorStatus(true);
    break;
  case Qt::Checked:
    this->setEditorStatus(false);
    emit this->autoScale();
    break;
  }
}

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

void ColorSelectionWidget::getColorScaleRange()
{
  double min = this->ui.minValLineEdit->text().toDouble();
  double max = this->ui.maxValLineEdit->text().toDouble();
  emit this->colorScaleChanged(min, max);
}

void ColorSelectionWidget::setColorScaleRange(double min, double max)
{
  if (this->ui.autoColorScaleCheckBox->isChecked())
  {
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

void ColorSelectionWidget::useLogScaling(int state)
{
  // Qt::Checked is 2, need it to be 1 for boolean true conversion
  if (Qt::Checked == state)
  {
    state -= 1;
  }
  emit this->logScale(state);
}
