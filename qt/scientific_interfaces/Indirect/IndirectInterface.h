// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECTINTERFACE_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECTINTERFACE_H_

#include "IndirectSettings.h"

#include "MantidQtWidgets/Common/UserSubWindow.h"

namespace MantidQt {
namespace CustomInterfaces {

class IndirectInterface : public MantidQt::API::UserSubWindow {
  Q_OBJECT

public:
  explicit IndirectInterface(QWidget *parent = nullptr);

protected slots:
  void help();
  void settings();
  void manageUserDirectories();
  void showMessageBox(QString const &message);

protected:
  std::map<std::string, QVariant> getInterfaceSettings() const;

private slots:
  void applySettings();

private:
  virtual void initLayout() override{};
  virtual std::string documentationPage() const { return ""; };

  virtual void applySettings(std::map<std::string, QVariant> const &settings);

  std::unique_ptr<IndirectSettings> m_settings;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACES_INDIRECTINTERFACE_H_ */
