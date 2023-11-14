// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"

#include "MantidQtWidgets/Common/ManageUserDirectories.h"
#include "MantidQtWidgets/Common/UserSubWindow.h"

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_INDIRECT_DLL IndirectInterface : public MantidQt::API::UserSubWindow {
  Q_OBJECT

public:
  explicit IndirectInterface(QWidget *parent = nullptr);

public slots:
  void applySettings();

protected slots:
  void help();
  void settings();
  void manageUserDirectories();
  void showMessageBox(QString const &message);

protected:
  virtual void initLayout() override;

private:
  virtual std::string documentationPage() const { return ""; };

  virtual void applySettings(std::map<std::string, QVariant> const &settings);
};

} // namespace CustomInterfaces
} // namespace MantidQt
