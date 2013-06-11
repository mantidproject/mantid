#include "LowHighStepInputWidget.h"
#include <QDoubleValidator>
#include <QLineEdit>
#include <QLabel>
#include <QBoxLayout>
#include <sstream>

/**
Calculate the number of bins from low,high,step.
@param low: The low limit
@param high: The high limit
@param step: The step
@return calculated number of bins.
*/
int calculateNBins(double low, double high, double step)
{
  ///Note that the result is being truncated here!
  return int((high-low)/step);
}

/**
Calculate the step from the high, low, number of bins
@param low: The low limit
@param high: The high limit
@param nBins: The number of bins
@return calculated number of bins.
*/
double calculateStep(double low, double high, double nBins)
{
  return (high-low)/nBins;
}

/// Constructor
LowHighStepInputWidget::LowHighStepInputWidget()
{
  m_step = new QLineEdit();

  connect(m_step, SIGNAL(editingFinished()), this, SLOT(nBinsListener()));

  QDoubleValidator* stepValidator = new QDoubleValidator(0, 10000, 5, this);
  m_step->setValidator(stepValidator);

  QHBoxLayout* layout = new QHBoxLayout;
  layout->addWidget(new QLabel("Step"));
  layout->addWidget(m_step);

  this->setLayout(layout);
}

/*
Getter for the entry
@min : min value
@max : max value
*/
int LowHighStepInputWidget::getEntry(double min, double max) const
{
  double step = m_step->text().toDouble();
  return calculateNBins(min, max, step);
}

/**
Set the entry assuming that only the step will change.
@nbins : number of bins to use.
@min : minimum (low)
@max : maximum (high)
*/
void LowHighStepInputWidget::setEntry(int nBins, double min, double max)
{
  double step = calculateStep(min, max, double(nBins));
  std::stringstream stream;
  stream << step;
  m_step->setText(stream.str().c_str());
}

/// Destructor.
LowHighStepInputWidget::~LowHighStepInputWidget()
{
}

/**
Listener for internal events, pubishing a public 'changed' call
*/
void LowHighStepInputWidget::nBinsListener()
{
  emit valueChanged();
}
