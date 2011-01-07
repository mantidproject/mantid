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

			///
			void setParent(QWidget*);

			/// returns the calendar widget
			QCalendarWidget* calendarWidget();

			//This method clears the data associated to the previous search
			void clearSearch( QTableWidget*,const std::string & wsName);
			
			bool login();
			///Thsi methos return true if it's valid session
			bool isSessionValid(const Mantid::API::IAlgorithm_sptr& alg);
			/// set label widget's text
			void setLabelText(QLabel* plabel,const QString& text);
		private:

		std::vector<std::string> executeListInstruments();

	
		
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
