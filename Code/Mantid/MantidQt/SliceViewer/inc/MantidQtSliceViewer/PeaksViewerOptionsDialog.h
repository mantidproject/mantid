#ifndef MANTIDSLICEVIEWER_PEAKSVIEWEROPTIONS_H
#define MANTIDSLICEVIEWER_PEAKSVIEWEROPTIONS_H

#include <QDialog>
#include <boost/shared_ptr.hpp>

class QAbstractButton;
namespace Ui
{
  class PeaksViewerOptionsDialog;
}

namespace MantidQt
{
  namespace SliceViewer
  {

    class PeaksPresenter;

    class PeaksViewerOptionsDialog: public QDialog
    {
      Q_OBJECT

    public:
      explicit PeaksViewerOptionsDialog(boost::shared_ptr<PeaksPresenter> peaksPresenter, QWidget *parent = 0);
      ~PeaksViewerOptionsDialog();

      virtual void closeEvent(QCloseEvent *);
      virtual void reject();

    private slots:

      void onSliderIntoProjectionMoved(int value);
      void onSliderOnProjectionMoved(int value);
      void onReset();
      void onCompleteClicked(QAbstractButton* button);

    private:
      Ui::PeaksViewerOptionsDialog *ui;
      boost::shared_ptr<PeaksPresenter> m_peaksPresenter;

      double m_originalOnProjectionFraction;
      double m_originalIntoProjectionFraction;
    };
  }
}

#endif // MANTIDSLICEVIEWER_PEAKSVIEWEROPTIONS_H
