// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Analysis/FunctionBrowser/FunctionTemplateView.h"
#include "ConvTemplatePresenter.h"
#include "DllConfig.h"
#include "FitTypes.h"

#include <QMap>
#include <QWidget>

class QtProperty;

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

using namespace ConvTypes;
/**
 * Class FunctionTemplateView implements QtPropertyBrowser to display
 * and set properties that can be used to generate a fit function.
 *
 */
class MANTIDQT_INELASTIC_DLL ConvFunctionTemplateView : public FunctionTemplateView {
  Q_OBJECT
public:
  explicit ConvFunctionTemplateView(QWidget *parent = nullptr);
  void updateParameterNames(const QMap<int, std::string> &parameterNames) override;
  void setGlobalParametersQuiet(std::vector<std::string> const &globals) override;

  void setBackgroundA0(double value) override;
  void setResolution(const std::vector<std::pair<std::string, size_t>> &fitResolutions) override;
  void addDeltaFunction();
  void removeDeltaFunction();
  void addTempCorrection(double value);
  void removeTempCorrection();
  void setQValues(const std::vector<double> &qValues) override;
  void setEnum(size_t subTypeIndex, int fitType);
  void setInt(size_t subTypeIndex, int val);

  void updateTemperatureCorrectionAndDelta(bool tempCorrection, bool deltaFunction);

protected slots:
  void intChanged(QtProperty *) override;
  void boolChanged(QtProperty *) override;
  void enumChanged(QtProperty *) override;
  void parameterChanged(QtProperty *) override;

private:
  void createProperties() override;
  void createFunctionParameterProperties();
  void createDeltaFunctionProperties();
  void createTempCorrectionProperties();
  void setSubType(size_t subTypeIndex, int typeIndex);
  void setParameterValueQuiet(ParamID id, double value, double error);

  std::vector<std::unique_ptr<TemplateSubType>> m_templateSubTypes;
  // Map fit type to a list of function parameters (QtProperties for those
  // parameters)
  std::vector<QMap<int, QList<QtProperty *>>> m_subTypeParameters;
  std::vector<QList<QtProperty *>> m_currentSubTypeParameters;
  std::vector<QtProperty *> m_subTypeProperties;

  QtProperty *m_deltaFunctionOn;
  QtProperty *m_deltaFunctionHeight;
  QtProperty *m_deltaFunctionCenter;

  QtProperty *m_tempCorrectionOn;
  QtProperty *m_temperature;

  QMap<QtProperty *, ParamID> m_parameterMap;
  QMap<ParamID, QtProperty *> m_parameterReverseMap;

private:
  friend class ConvTemplatePresenter;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
