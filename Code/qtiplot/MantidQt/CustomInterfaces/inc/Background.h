#ifndef MANTIDQTCUSTOMINTERFACES_BACKGROUND_H_
#define MANTIDQTCUSTOMINTERFACES_BACKGROUND_H_

#include "MantidQtAPI/MantidDialog.h"
#include <QCheckBox>
#include <QLineEdit>
#include <QPair>

//--------------------------------------------------------
// Forward declarations
//--------------------------------------------------------
class QShowEvent;
class QCloseEvent;

namespace MantidQt
{
  namespace CustomInterfaces
  {

    class Background : public API::MantidDialog
    {
      Q_OBJECT

    public:
      Background(QWidget *parent = NULL);
      
      bool removeBackground() const;
      void removeBackground(bool remove);
      QPair<double, double> getRange() const;
      void setRange(double min, double max);
      
    private:
      void initLayout();
      void showEvent(QShowEvent*);
      void closeEvent(QCloseEvent*);
      bool sanityCheck();
    
    private:
      QCheckBox *m_ckDoRemove;
      QLineEdit *m_leStart;
      QLineEdit *m_leEnd;

      /// Actual values for analysis, stored separately so that the dialog can be reverted
      double m_rangeMin;
      double m_rangeMax;
      bool m_doRemoval;

    };

  }
}

#endif // MANTIDQTCUSTOMINTERFACES_BACKGROUND_H_
