#ifndef MANTIDQTMANTIDWIDGETS_MANTIDWIDGETS_H_
#define MANTIDQTMANTIDWIDGETS_MANTIDWIDGETS_H_

#include "Poco/Path.h"
#include "WidgetDllOption.h"
#include <QGroupBox>
#include <QGridLayout>
#include <QLabel>
#include <QWidget>
#include <QString>
#include <QHash>
#include <string>
#include <map>

/** The ase class from which mantid custom widgets are derived it contains
*  some useful functions
*/
namespace MantidQt
{
  namespace MantidWidgets
  {    
	class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS MantidWidget : public QWidget
    {
      Q_OBJECT
	  
	protected:
	  /// A list of labels to use as validation markers
      QHash<const QWidget * const, QLabel *> m_validators;
	  
      MantidWidget(QWidget *parent=NULL) : QWidget(parent) {}
	  void renameWorkspace(const QString &oldName, const QString &newName);
      void setupValidator(QLabel *star);
	  QLabel* newStar(const QGroupBox * const UI, int valRow, int valCol);
      QLabel* newStar(QGridLayout * const lay, int valRow, int valCol);
	  void hideValidators();
    };
  }
}

#endif //MANTIDQTMANTIDWIDGETS_MANTIDWIDGETS_H_