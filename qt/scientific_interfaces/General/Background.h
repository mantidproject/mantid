// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_BACKGROUND_H_
#define MANTIDQTCUSTOMINTERFACES_BACKGROUND_H_

#include "MantidQtWidgets/Common/MantidDialog.h"
#include <QCheckBox>
#include <QLineEdit>
#include <QPair>

//--------------------------------------------------------
// Forward declarations
//--------------------------------------------------------
class QShowEvent;
class QCloseEvent;

namespace MantidQt {
namespace CustomInterfaces {

class Background : public API::MantidDialog {
  Q_OBJECT

public:
  explicit Background(QWidget *parent = nullptr);

  bool removeBackground() const;
  void removeBackground(bool remove);
  QPair<double, double> getRange() const;
  void setRange(double min, double max);

private:
  void initLayout();
  void showEvent(QShowEvent * /*e*/) override;
  void closeEvent(QCloseEvent * /*event*/) override;
  bool sanityCheck();

private:
  QCheckBox *m_ckDoRemove;
  QLineEdit *m_leStart;
  QLineEdit *m_leEnd;

  /// Actual values for analysis, stored separately so that the dialog can be
  /// reverted
  double m_rangeMin;
  double m_rangeMax;
  bool m_doRemoval;
};
} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_BACKGROUND_H_
