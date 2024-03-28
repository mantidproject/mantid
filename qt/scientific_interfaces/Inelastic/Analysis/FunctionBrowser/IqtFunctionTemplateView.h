// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Analysis/FunctionBrowser/FunctionTemplateView.h"
#include "DllConfig.h"
#include "IqtTemplatePresenter.h"

#include <QMap>
#include <QWidget>

class QtProperty;

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

/**
 * Class FunctionTemplateView implements QtPropertyBrowser to display
 * and set properties that can be used to generate a fit function.
 *
 */
class MANTIDQT_INELASTIC_DLL IqtFunctionTemplateView : public FunctionTemplateView {
  Q_OBJECT
public:
  explicit IqtFunctionTemplateView(TemplateBrowserCustomizations customizations);
  void addExponentialOne();
  void removeExponentialOne();
  void addExponentialTwo();
  void removeExponentialTwo();
  void addStretchExponential();
  void removeStretchExponential();
  void addFlatBackground();
  void removeBackground();

  void setExp1Height(double, double);
  void setExp1Lifetime(double, double);
  void setExp2Height(double, double);
  void setExp2Lifetime(double, double);
  void setStretchHeight(double, double);
  void setStretchLifetime(double, double);
  void setStretchStretching(double, double);
  void setA0(double, double);

  void updateParameterNames(const std::map<int, std::string> &parameterNames) override;
  void setGlobalParametersQuiet(std::vector<std::string> const &globals) override;
  void clear() override;

protected slots:
  void intChanged(QtProperty *) override;
  void boolChanged(QtProperty *) override;
  void enumChanged(QtProperty *) override;
  void parameterChanged(QtProperty *) override;

private:
  void createProperties() override;
  void setTieIntensitiesQuiet(bool on);
  void updateState();

  QtProperty *m_numberOfExponentials;
  QtProperty *m_exp1Height = nullptr;
  QtProperty *m_exp1Lifetime = nullptr;
  QtProperty *m_exp2Height = nullptr;
  QtProperty *m_exp2Lifetime = nullptr;
  QtProperty *m_stretchExponential;
  QtProperty *m_stretchExpHeight = nullptr;
  QtProperty *m_stretchExpLifetime = nullptr;
  QtProperty *m_stretchExpStretching = nullptr;
  QtProperty *m_background;
  QtProperty *m_A0 = nullptr;
  QtProperty *m_tieIntensities = nullptr;
  QMap<QtProperty *, int> m_parameterMap;

private:
  friend class IqtTemplatePresenter;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
