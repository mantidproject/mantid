// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "InelasticDataManipulationSqwTabModel.h"
#include "InelasticDataManipulationSqwTabView.h"
#include "InelasticDataManipulationTab.h"

#include "MantidGeometry/IComponent.h"
#include "MantidKernel/System.h"

namespace MantidQt {
namespace CustomInterfaces {
/** InelasticDataManipulationSqwTab

  @author Dan Nixon
  @date 23/07/2014
*/
class DLLExport InelasticDataManipulationSqwTab : public InelasticDataManipulationTab {
  Q_OBJECT

public:
  InelasticDataManipulationSqwTab(QWidget *parent = nullptr);
  ~InelasticDataManipulationSqwTab() = default;

  void setup() override;
  void run() override;
  bool validate() override;

private slots:
  void handleDataReady(QString const &dataName) override;
  void sqwAlgDone(bool error);

  void runClicked();
  void saveClicked();

  void qLowChanged(double value);
  void qWidthChanged(double value);
  void qHighChanged(double value);
  void eLowChanged(double value);
  void eWidthChanged(double value);
  void eHighChanged(double value);
  void rebinEChanged(int value);

private:
  void connectSignals();
  void plotRqwContour();
  void setFileExtensionsByName(bool filter) override;

  std::unique_ptr<InelasticDataManipulationSqwTabModel> m_model;
  std::unique_ptr<InelasticDataManipulationSqwTabView> m_view;
};
} // namespace CustomInterfaces
} // namespace MantidQt
