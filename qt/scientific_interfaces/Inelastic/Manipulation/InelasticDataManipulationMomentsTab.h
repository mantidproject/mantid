// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IMomentsView.h"
#include "InelasticDataManipulationMomentsTabModel.h"
#include "InelasticDataManipulationMomentsTabView.h"
#include "InelasticDataManipulationTab.h"

#include "MantidKernel/System.h"
#include "ui_InelasticDataManipulationMomentsTab.h"

#include <QFont>

namespace MantidQt {
namespace CustomInterfaces {

class IMomentsPresenter {
public:
  virtual void handleDataReady(std::string const &dataName) = 0;

  virtual void handleScaleChanged(bool state) = 0;
  virtual void handleScaleValueChanged(double const value) = 0;
  virtual void handleValueChanged(std::string &propName, double value) = 0;

  virtual void handleRunClicked() = 0;
  virtual void handleSaveClicked() = 0;
};
/** InelasticDataManipulationMomentsTab : Calculates the S(Q,w) Moments of the provided data with
  the user specified range and scale factor


  @author Samuel Jackson
  @date 13/08/2013
*/
class MANTIDQT_INELASTIC_DLL InelasticDataManipulationMomentsTab : public InelasticDataManipulationTab,
                                                                   public IMomentsPresenter {

public:
  InelasticDataManipulationMomentsTab(QWidget *parent, IMomentsView *view);
  ~InelasticDataManipulationMomentsTab() = default;

  void setup() override;
  void run() override;
  bool validate() override;

  void handleDataReady(std::string const &dataName) override;

  void handleScaleChanged(bool state) override;
  void handleScaleValueChanged(double const value) override;
  void handleValueChanged(std::string &propName, double value) override;

  void handleRunClicked() override;
  void handleSaveClicked() override;

protected:
  void runComplete(bool error);

private:
  void plotNewData(std::string const &filename);
  void setFileExtensionsByName(bool filter) override;
  std::unique_ptr<InelasticDataManipulationMomentsTabModel> m_model;
  IMomentsView *m_view;
};
} // namespace CustomInterfaces
} // namespace MantidQt
