#ifndef MANTIDQTWIDGETS_ICATSEARCHTWO_H_
#define MANTIDQTWIDGETS_ICATSEARCHTWO_H_

#include "ui_ICatSearchTwo.h"
#include "WidgetDllOption.h"

namespace MantidQt
{
  namespace MantidWidgets
  {

    class  EXPORT_OPT_MANTIDQT_MANTIDWIDGETS ICatSearchTwo : public QWidget
    {
      Q_OBJECT

    public:
      /// Default constructor
      ICatSearchTwo(QWidget *parent = 0);
      /// Destructor
      ~ICatSearchTwo();

    private:
      /// Initialise the layout
      virtual void initLayout();

    };
  }
}
#endif // MANTIDQTWIDGETS_ICATSEARCHTWO_H_
