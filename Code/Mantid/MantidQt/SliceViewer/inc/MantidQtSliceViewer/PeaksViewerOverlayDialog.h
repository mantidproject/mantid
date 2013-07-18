#ifndef MANTIDSLICEVIEWER_PEAKSVIEWEROVERLAYDIALOG_H
#define MANTIDSLICEVIEWER_PEAKSVIEWEROVERLAYDIALOG_H

#include <QDialog>
#include "MantidQtSliceViewer/PeaksPresenter.h"

class QAbstractButton;
namespace Ui
{
  class PeaksViewerOverlayDialog;
}

namespace MantidQt
{
  namespace SliceViewer
  {
    class PeaksViewerOverlayDialog: public QDialog
    {
      Q_OBJECT

    public:
      explicit PeaksViewerOverlayDialog(PeaksPresenter_sptr peaksPresenter, QWidget *parent = 0);
      ~PeaksViewerOverlayDialog();

      virtual void closeEvent(QCloseEvent *);
      virtual void reject();

    private slots:

      void onSliderIntoProjectionMoved(int value);
      void onSliderOnProjectionMoved(int value);
      void onReset();
      void onCompleteClicked(QAbstractButton* button);
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
