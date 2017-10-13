#ifndef MANTIDSLICEVIEWER_PEAKSVIEWEROVERLAYDIALOG_H
#define MANTIDSLICEVIEWER_PEAKSVIEWEROVERLAYDIALOG_H

#include <QDialog>
#include "MantidQtWidgets/SliceViewer/PeaksPresenter.h"

class QAbstractButton;
namespace Ui {
class PeaksViewerOverlayDialog;
}

namespace MantidQt {
namespace SliceViewer {
class PeaksViewerOverlayDialog : public QDialog {
  Q_OBJECT

public:
  explicit PeaksViewerOverlayDialog(PeaksPresenter_sptr peaksPresenter,
                                    QWidget *parent = nullptr);
  ~PeaksViewerOverlayDialog() override;

  void closeEvent(QCloseEvent *) override;
  void reject() override;

private slots:

  void onSliderIntoProjectionMoved(int value);
  void onSliderOnProjectionMoved(int value);
  void onReset();
  void onCompleteClicked(QAbstractButton *button);
  void onHelp();

private:
  Ui::PeaksViewerOverlayDialog *ui;
  PeaksPresenter_sptr m_peaksPresenter;

  double m_originalOnProjectionFraction;
  double m_originalIntoProjectionFraction;
};
}
}

#endif // MANTIDSLICEVIEWER_PEAKSVIEWEROVERLAYDIALOG_H
