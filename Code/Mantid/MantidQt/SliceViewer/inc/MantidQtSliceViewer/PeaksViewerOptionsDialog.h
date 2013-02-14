#ifndef MANTIDSLICEVIEWER_PEAKSVIEWEROPTIONS_H
#define MANTIDSLICEVIEWER_PEAKSVIEWEROPTIONS_H

#include <QDialog>
#include "MantidQtSliceViewer/PeaksPresenter.h"

class QAbstractButton;
namespace Ui
{
  class PeaksViewerOptionsDialog;
}

namespace MantidQt
{
  namespace SliceViewer
  {
    class PeaksViewerOptionsDialog: public QDialog
    {
      Q_OBJECT

    public:
      explicit PeaksViewerOptionsDialog(PeaksPresenter_sptr peaksPresenter, QWidget *parent = 0);
      ~PeaksViewerOptionsDialog();

      virtual void closeEvent(QCloseEvent *);
      virtual void reject();

    private slots:

      void onSliderIntoProjectionMoved(int value);
      void onSliderOnProjectionMoved(int value);
      void onReset();
      void onCompleteClicked(QAbstractButton* button);
      void onHelp();

    private:
      Ui::PeaksViewerOptionsDialog *ui;
      PeaksPresenter_sptr m_peaksPresenter;

      double m_originalOnProjectionFraction;
      double m_originalIntoProjectionFraction;
    };
  }
}

#endif // MANTIDSLICEVIEWER_PEAKSVIEWEROPTIONS_H
