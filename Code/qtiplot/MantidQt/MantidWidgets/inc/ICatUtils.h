#ifndef MANTIDWIDGETS_ICATUTILS_H_
#define MANTIDWIDGETS_ICATUTILS_H_

#include "MantidQtMantidWidgets/ICatInvestigation.h"
#include "MantidAPI/ITableWorkspace.h"

#include <QTableWidgetItem>
#include <QTableWidget>
#include <QWidget>
#include <QCalendarWidget>

namespace MantidQt
{
	namespace MantidWidgets
	{
		class SearchCalendar: public QCalendarWidget
		{
		public:
			SearchCalendar(QWidget* parent=0);
			virtual void leaveEvent(QEvent* qevent);

		};

		class ICatUtils
		{
		public:
			///constrctor
			ICatUtils(){}
			//destructor
			~ICatUtils(){}


			/// This method displays the selected investigation details
			void investigationSelected(QTableWidget* tablewidget,QTableWidgetItem* item,
				QWidget* parent,Mantid::API::ITableWorkspace_sptr ws_sptr);

			/// update the table widget with search results
			void updatesearchResults(Mantid::API::ITableWorkspace_sptr& ws_sptr,QTableWidget* tablewidget );
		private:
			///investigation widget
			ICatInvestigation* m_invstWidget;
			///parent widget
			QWidget* m_applicationWindow;

		};


	}
}
#endif