#ifndef MANTIDWIDGETS_ICATUTILS_H_
#define MANTIDWIDGETS_ICATUTILS_H_

#include "MantidQtMantidWidgets/ICatInvestigation.h"
#include "MantidAPI/ITableWorkspace.h"

#include <QTableWidgetItem>
#include <QTableWidget>
#include <QWidget>
#include <QCalendarWidget>
#include <QComboBox>
#include <QLabel>

namespace MantidQt
{
	namespace MantidWidgets
	{
		class SearchCalendar: public QCalendarWidget
		{
		public:
			SearchCalendar(QWidget* parent=0);
		};


		class ICatUtils : public QObject
		{
			Q_OBJECT
			
		public:
			///constrctor
			ICatUtils();
			//destructor
			~ICatUtils(){}


			/// This method displays the selected investigation details
			void investigationSelected(QTableWidget* tablewidget,QTableWidgetItem* item,
				QWidget* parent,Mantid::API::ITableWorkspace_sptr ws_sptr);

			/// update the table widget with search results
			void updatesearchResults(Mantid::API::ITableWorkspace_sptr& ws_sptr,QTableWidget* tablewidget );

			//for clearing teh table widget
			void resetSearchResultsWidget(QTableWidget* tablewidget );

			/// for displaying the investigatiosn count 
			void updateSearchLabel(const Mantid::API::ITableWorkspace_sptr& ws_sptr,QLabel* label);

			void populateInstrumentBox(QComboBox* instrumentBox);

			///popup DateTime calender to select date
			void popupCalendar(QWidget* parent);

			/// close calendarwidget
			void closeCalendarWidget(); 

			/// returns the calendar widget
			QCalendarWidget* calendarWidget();
			
			
		private:

			Mantid::API::ITableWorkspace_sptr  executeListInstruments();

		private:
			///investigation widget
			ICatInvestigation* m_invstWidget;
			///parent widget
			QWidget* m_applicationWindow;
			
			///pointer to calender object
			QCalendarWidget* m_calendarWidget ;
			

		};


	}
}
#endif