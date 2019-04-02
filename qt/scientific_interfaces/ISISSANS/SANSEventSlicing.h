// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_SANSEVENTSLICING_H_
#define MANTIDQTCUSTOMINTERFACES_SANSEVENTSLICING_H_

#include "MantidQtWidgets/Common/UserSubWindow.h"
#include "ui_SANSEventSlicing.h"
#include <QString>

namespace MantidQt {
namespace CustomInterfaces {

class SANSEventSlicing : public API::UserSubWindow {
  Q_OBJECT

public:
  /// Default Constructor
  explicit SANSEventSlicing(QWidget *parent = nullptr);
  /// Destructor
  ~SANSEventSlicing() override;

  static std::string name() { return "SANS ISIS Slicing"; }
  static QString categoryInfo() { return "SANS"; }

private:
  struct ChargeAndTime {
    QString charge;
    QString time;
  };

  void initLayout() override;

  ChargeAndTime getFullChargeAndTime(const QString &name_ws);
  QString createSliceEventCode(const QString &name_ws, const QString &start,
                               const QString &stop);
  ChargeAndTime runSliceEvent(const QString &code2run);
  void checkPythonOutput(const QString &result);
  ChargeAndTime values2ChargeAndTime(const QString &input);
  void raiseWarning(QString title, QString message);

protected:
  void showEvent(QShowEvent * /*unused*/) override;
private slots:

  /// Apply the slice for the SANS data, and update the view with the last
  /// sliced data.
  void doApplySlice();
  void onChangeWorkspace(const QString &newWs);

private:
  Ui::SANSEventSlicing ui;
};
} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_SANSEVENTSLICING_H_
