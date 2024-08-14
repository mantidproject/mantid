// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/ManageUserDirectories.h"
#include "MantidQtWidgets/Common/UserSubWindow.h"

#include "DllConfig.h"

namespace MantidQt {
namespace CustomInterfaces {

class MANTID_SPECTROSCOPY_DLL InelasticInterface : public MantidQt::API::UserSubWindow {
  Q_OBJECT

public:
  explicit InelasticInterface(QWidget *parent = nullptr);

public slots:
  void applySettings();

protected slots:
  void help();
  void settings();
  void manageUserDirectories();
  void showMessageBox(std::string const &message);

protected:
  virtual void initLayout() override;

private:
  virtual std::string documentationPage() const { return ""; };

  virtual void applySettings(std::map<std::string, QVariant> const &settings);
};

} // namespace CustomInterfaces
} // namespace MantidQt
