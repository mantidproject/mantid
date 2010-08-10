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
private:
	Mantid::API::ITableWorkspace_sptr executeMyDataSearch();
private:
	Ui::Form m_uiForm;

};
}
}
#endif