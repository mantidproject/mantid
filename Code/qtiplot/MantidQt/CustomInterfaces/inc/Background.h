#ifndef MANTIDQTCUSTOMINTERFACES_BACKGROUND_H_
#define MANTIDQTCUSTOMINTERFACES_BACKGROUND_H_

#include "MantidQtAPI/MantidQtDialog.h"
#include <map>
#include <string>
#include <QCheckBox>
#include <QLineEdit>
#include <QDialog>
#include <QSignalMapper>
#include <QGridLayout>
#include <QSettings>
#include <QCloseEvent>
#include <climits>

namespace MantidQt
{
  namespace CustomInterfaces
  {

    class Background : public API::MantidQtDialog
    {
      Q_OBJECT

    public:
      /// Contains the all the background removal settings, which is all the settings from the Background dialog
	  struct TOFWindow
	  {
	    /// is set to don't do background removal
		TOFWindow() : doBackRemoval(false), start(-1), end(-1) {}
		/// stores whether or not the user selected that background removal should take place
		bool doBackRemoval;
		/// TOF where the background region is taken to start
		double start;
	    /// TOF where the background region is taken to end
		double end;
	  };

      /// Default Constructor
      Background(QWidget *parent, const QString &settingsGrp);

    signals:
      /// Is emitted just before the window dies to enable the parents Run button again
      void formClosed();

    private:
	  QCheckBox *m_ckDoRemove;
	  QLineEdit *m_leStart;
	  QLineEdit *m_leEnd;

      /// the values on the form the last time OK was clicked are accessed through this object
	  QSettings m_prevSets;
	  
      void initLayout();
	  void loadSettings();
	private slots:
	  void saveSettings();
      void closeEvent(QCloseEvent *event);
    };

  }
}

#endif // MANTIDQTCUSTOMINTERFACES_BACKGROUND_H_
