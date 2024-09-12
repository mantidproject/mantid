// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DataProcessor.h"
#include "IMomentsView.h"
#include "MantidQtWidgets/Spectroscopy/RunWidget/IRunSubscriber.h"
#include "MomentsModel.h"
#include "MomentsView.h"

#include "MantidKernel/System.h"
#include "ui_MomentsTab.h"

#include <QFont>

namespace MantidQt {
namespace CustomInterfaces {

class IMomentsPresenter {
public:
  virtual void handleDataReady(std::string const &dataName) = 0;

  virtual void handleScaleChanged(bool state) = 0;
  virtual void handleScaleValueChanged(double const value) = 0;
  virtual void handleValueChanged(std::string const &propName, double value) = 0;
  virtual void handleSaveClicked() = 0;
};
/** MomentsPresenter : Calculates the S(Q,w) Moments of the provided data with
  the user specified range and scale factor


  @author Samuel Jackson
  @date 13/08/2013
*/
class MANTIDQT_INELASTIC_DLL MomentsPresenter : public DataProcessor, public IMomentsPresenter, public IRunSubscriber {

public:
  MomentsPresenter(QWidget *parent, std::unique_ptr<MantidQt::API::IAlgorithmRunner> algorithmRunner,
                   IMomentsView *view, std::unique_ptr<IMomentsModel> model);
  ~MomentsPresenter() = default;

  // runWidget
  void handleRun() override;
  void handleValidation(IUserInputValidator *validator) const override;
  const std::string getSubscriberName() const override { return "Moments"; }

  void handleDataReady(std::string const &dataName) override;

  void handleScaleChanged(bool state) override;
  void handleScaleValueChanged(double const value) override;
  void handleValueChanged(std::string const &propName, double value) override;

  void handleSaveClicked() override;

protected:
  void runComplete(bool error) override;

private:
  void plotNewData(std::string const &filename);
  void setFileExtensionsByName(bool filter) override;
  void setLoadHistory(bool doLoadHistory) override;

  IMomentsView *m_view;
  std::unique_ptr<IMomentsModel> m_model;
};
} // namespace CustomInterfaces
} // namespace MantidQt
