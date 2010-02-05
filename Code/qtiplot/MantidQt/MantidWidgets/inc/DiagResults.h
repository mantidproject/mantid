#ifndef MANTIDQTCUSTOMINTERFACES_EXCITATIONSDIAGRESULTS_H_
#define MANTIDQTCUSTOMINTERFACES_EXCITATIONSDIAGRESULTS_H_

#include "MantidQtAPI/MantidQtDialog.h"
#include "MantidQtMantidWidgets/MantidWidget.h"
#include "WidgetDllOption.h"
#include <map>
#include <string>
#include <QDialog>
#include <QSignalMapper>
#include <QGridLayout>
#include <climits>

namespace MantidQt
{
  namespace MantidWidgets
  {

    class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS DiagResults : public MantidWidget
    {
      Q_OBJECT

    public:
      // stores the informtion returned by the python scripts that look for bad detectors
      struct EXPORT_OPT_MANTIDQT_MANTIDWIDGETS TestSummary
      {
		explicit TestSummary(QString name);
	    QString pythonResults(const QString &pyhtonOut);
		void setStatus(QString &info) {status = info;}
		const QString& getStatus() const {return status;}
		
		QString test;                       //< Name of the test is displayed to users
        QString status;                     //< status is displayed to users
        QString outputWS;                   //< Name of the workspace that contains the bad detectors
        int numBad;                         //< The total number of bad detectors
        QString inputWS;                    //< If these results came from loading another workspace this contains the name of that workspace
        enum resultsStatus {NORESULTS = 15-INT_MAX};  //< a flag value to indicate that there are no results to show, could be that the test has not completed or there was an error
      };
  
      static const QString tests[];
      static const int numTests;
	  
      DiagResults(QWidget *parent = 0);
      void notifyDialog(const TestSummary &display);

    signals:
      ///Emits this signal to run some python code
      void runAsPythonScript(const QString&);
      /// is emitted just before the window dies to let the window that created know the pointer it has is invalid
      void died();

    private:
      /// the layout that widgets are added to
      QGridLayout *m_Grid;
      /// points to the slot that deals with list buttons being pressed
      QSignalMapper *m_ListMapper;
      /// points to the slot that deals with view buttons being pressed
      QSignalMapper *m_ViewMapper;
      /// stores the name of the workspaces that contains the results of each test
      std::map<QString, QString> outputWorkS;

      int addRow(QString firstColumn, QString secondColumn);
      void addButtonsDisab(int row);
      void showButtons(int row, QString test);
      void updateRow(int row, QString firstColumn, int secondColumn);
      void closeEvent(QCloseEvent *event);

    private slots:
      void tableList(const QString &name);
      void instruView(const QString &name);
    };
  }
}

#endif //MANTIDQTCUSTOMINTERFACES_EXCITATIONSDIAGRESULTS_H_
