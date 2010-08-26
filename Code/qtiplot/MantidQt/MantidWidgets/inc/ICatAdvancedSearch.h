#ifndef MANTIDWIDGETS_ICATADVANCEDSEARCH_H_
#define MANTIDWIDGETS_ICATADVANCEDSEARCH_H_

#include "WidgetDllOption.h"
#include "MantidQtMantidWidgets/ui_ICatAdvancedSearch.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidQtMantidWidgets/ICatUtils.h"

#include <QCalendarWidget>
#include<QObject>

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
			Mantid::API::ITableWorkspace_sptr  executeListInvestigationTypes();

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

		private:
			Ui::ICatAdvancedSearch m_uiForm;

			///stores investigations data 
			Mantid::API::ITableWorkspace_sptr m_ws_sptr;

			///parent widget
			QWidget* m_applicationWindow;
			///pointer to object to identify starta nd end date tool button
			QObject* m_sender;

			boost::shared_ptr<ICatUtils> m_utils_sptr;
		};
	}
}
#endif