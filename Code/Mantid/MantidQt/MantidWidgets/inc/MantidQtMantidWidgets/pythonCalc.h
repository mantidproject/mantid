#ifndef MANTIDQTMANTIDWIDGETS_PYTHONCALC_H_
#define MANTIDQTMANTIDWIDGETS_PYTHONCALC_H_

#include "Poco/Path.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidQtAPI/MantidWidget.h"
#include "WidgetDllOption.h"
#include <QLineEdit>
#include <QLabel>
#include <QString>
#include <string>
#include <map>
#include <climits>

namespace MantidQt
{
  namespace MantidWidgets
  {    
	class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS pythonCalc : public API::MantidWidget
    {
      Q_OBJECT
	  
    public:
      const QString& python() const;
	    /** Allows access to m_fails, the list of any validation errors
	    *  return the map is empty if there were no errors, otherwise, the keys are the internal names of the controls, the values the errors
	    */
	    QString checkNoErrors(const QHash<const QWidget * const, QLabel *> &validLbls) const;
	  
	    QString run();
    
	protected:
      pythonCalc(QWidget *interface);
      ///this will store the executable python code when it is generated
      QString m_pyScript;
      ///stores the namees of controls with invalid entries as the keys and a discription of the error as the associated value
      std::map<const QWidget * const , std::string> m_fails;
  
      virtual void appendFile(const QString &pythonFile);
      virtual void loadFile(const QString &pythonFile);

      void LEChkCp(QString pythonMark, const QLineEdit * const userVal, Mantid::Kernel::Property * const check);
      std::string replaceErrsFind(QString pythonMark, const QString &setting, Mantid::Kernel::Property * const check);

      void appendChk(const QLineEdit * const userVal, Mantid::Kernel::Property * const check);
    };
  }
}

#endif //MANTIDQTMANTIDWIDGETS_PYTHONCALC_H_
