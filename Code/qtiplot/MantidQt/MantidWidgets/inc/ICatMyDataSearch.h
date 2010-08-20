#ifndef MANTIDWIDGETS_ICATMYDATASEARCH_H_
#define MANTIDWIDGETS_ICATMYDATASEARCH_H_

#include "MantidQtMantidWidgets/ui_ICatMyDataSearch.h"
#include "WidgetDllOption.h"
#include "MantidAPI/ITableWorkspace.h"

namespace MantidQt
{
namespace MantidWidgets
{
class  EXPORT_OPT_MANTIDQT_MANTIDWIDGETS ICatMyDataSearch : public QWidget
{
  Q_OBJECT
public:
	ICatMyDataSearch(QWidget*parent);
	//~ICatMyDataSearch();
signals:
	 ///this signal prints error messge to log window
   void error(const QString&);

   private slots:
	   /// Thsi method gets called when an investigation from list of investigations cliked.
	   void investigationSelected(QTableWidgetItem* );

private:
   /// execute mydatsesrch algorithm
	bool executeMyDataSearch(Mantid::API::ITableWorkspace_sptr& ws_sptr);
	/// setting the parent widget
	void setparentWidget(QWidget* par);

private:
	Ui::MyData m_uiForm;

	///parent widget
	QWidget* m_applicationWindow;

	/// workspace which contains investigation data like abstrct,facility user names,sample names etc.
	Mantid::API::ITableWorkspace_sptr  m_ws2_sptr ;

};
}
}
#endif