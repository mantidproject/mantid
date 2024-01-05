// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "ISqwView.h"
#include "InelasticDataManipulationSqwTabModel.h"
#include "InelasticDataManipulationSqwTabView.h"
#include "InelasticDataManipulationTab.h"

#include "MantidGeometry/IComponent.h"
#include "MantidKernel/System.h"

namespace MantidQt {
namespace CustomInterfaces {

class ISqwPresenter {
public:
  virtual void handleDataReady(std::string const &dataName) = 0;

  virtual void handleQLowChanged(double const value) = 0;
  virtual void handleQWidthChanged(double const value) = 0;
  virtual void handleQHighChanged(double const value) = 0;

  virtual void handleELowChanged(double const value) = 0;
  virtual void handleEWidthChanged(double const value) = 0;
  virtual void handleEHighChanged(double const value) = 0;
  virtual void handleRebinEChanged(int const value) = 0;

  virtual void handleRunClicked() = 0;
  virtual void handleSaveClicked() = 0;
};

/** InelasticDataManipulationSqwTab

  @author Dan Nixon
  @date 23/07/2014
*/
class MANTIDQT_INELASTIC_DLL InelasticDataManipulationSqwTab : public InelasticDataManipulationTab,
                                                               public ISqwPresenter {

public:
  InelasticDataManipulationSqwTab(QWidget *parent, ISqwView *view);
  ~InelasticDataManipulationSqwTab() = default;

  void setup() override;
  void run() override;
  bool validate() override;

  void handleDataReady(std::string const &dataName) override;

  void handleQLowChanged(double const value) override;
  void handleQWidthChanged(double const value) override;
  void handleQHighChanged(double const value) override;

  void handleELowChanged(double const value) override;
  void handleEWidthChanged(double const value) override;
  void handleEHighChanged(double const value) override;
  void handleRebinEChanged(int const value) override;

  void handleRunClicked() override;
  void handleSaveClicked() override;

protected:
  void runComplete(bool error) override;

private:
  void plotRqwContour();
  void setFileExtensionsByName(bool filter) override;

  std::unique_ptr<InelasticDataManipulationSqwTabModel> m_model;
  ISqwView *m_view;
};
} // namespace CustomInterfaces
} // namespace MantidQt