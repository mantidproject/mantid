#ifndef MANTIDWIDGETS_ICATADVANCEDSEARCH_H_
#define MANTIDWIDGETS_ICATADVANCEDSEARCH_H_

#include "WidgetDllOption.h"
#include "ui_ICatAdvancedSearch.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidQtMantidWidgets/ICatUtils.h"
#include <algorithm>

#include <QCalendarWidget>
#include<QObject>
#include <QHash>

namespace MantidQt
{
  namespace MantidWidgets
  {
    class  EXPORT_OPT_MANTIDQT_MANTIDWIDGETS ICatAdvancedSearch : public QWidget
    {
      Q_OBJECT

    signals:
      void error(const QString&,int param=0);
    public:
      ICatAdvancedSearch(QWidget*parent =0 );
      ~ICatAdvancedSearch();
      void initLayout();

    private slots:
      ///slot for search button
      void onSearch();
      //slot for close button
      void onClose();
      ///slot for double licking the seatch result table's row
      void investigationSelected(QTableWidgetItem *);
      ///popup DateTime calender to select date
      void popupCalendar();
      ///start date changed
      void getDate(const QDate& date  );
      //handler for helpbutton
      void helpButtonClicked();

    private:
      void populateInstrumentBox();
      void populateInvestigationType();
      //execute the algorithm for populating investigation types.
      std::vector<std::string>  executeListInvestigationTypes();

      void getInvestigationName(QString& invstName);
      void getInvestigationAbstract(QString& invstAbstract);
      void getSampleName(QString& sampleName);
      void getInvestigatorSurName(QString& invstSurName);
      void getDatafileName(QString& dataFileName);
      void getCaseSensitive(bool& bCase);
      void getInvestigationType(QString& invstType);

      void getRunNumbers(double& startRun,double& endRun);
      void getDates(QString& startDate,QString& endDate);
      void getInstrument(QString& instrName);
      void getKeyWords(QString& keywords);
      void updatesearchResults(Mantid::API::ITableWorkspace_sptr& ws_sptr );
      void setparentWidget(QWidget*);
      void saveSettings();
      /// read settings from registry
      void readSettings();
      //event filtering
      bool eventFilter(QObject *obj, QEvent *event);

      /// This method sets the property of algorithm
      template<typename T >
      bool setProperty(QString name,T value)
      {

        try
        {
          m_alg->setProperty(name.toStdString(),value);
        }
        catch(std::invalid_argument& e)
        {
          emit error(e.what());
          showInvalidMarkerLabel(name);

          return false;
        }
        catch (Mantid::Kernel::Exception::NotFoundError& e)
        {
          emit error(e.what());
          showInvalidMarkerLabel(name);
          return false;
        }

        hideInvalidMarkerLabel(name);
        return true;
      }

      /// This method adds invalid marker labels to hashtable
      void addtoPropertyLabelsHash();
      /// this method creates shared pointer to search algorithm
      Mantid::API::IAlgorithm_sptr createAlgorithm();
      /// show invalid marker labels
      void showInvalidMarkerLabel(const QString& name);
      /// hide invalid marker labels
      void hideInvalidMarkerLabel(const QString& name);

    private:
      Ui::ICatAdvancedSearch m_uiForm;

      ///stores investigations data
      Mantid::API::ITableWorkspace_sptr m_ws_sptr;

      ///parent widget
      QWidget* m_applicationWindow;
      ///pointer to object to identify starta nd end date tool button
      QObject* m_sender;

      boost::shared_ptr<ICatUtils> m_utils_sptr;

      /// hash containing property name and invalid marker * label
      QHash<QString,QWidget*> m_propLabelHash;

      /// shared pointer to search algorithm
      Mantid::API::IAlgorithm_sptr m_alg;
    };
  }
}
#endif
