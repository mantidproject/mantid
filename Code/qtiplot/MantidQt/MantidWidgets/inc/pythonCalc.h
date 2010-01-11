#ifndef MANTIDQTMANTIDWIDGETS_PYTHONCALC_H_
#define MANTIDQTMANTIDWIDGETS_PYTHONCALC_H_

#include "Poco/Path.h"
#include "MantidKernel/PropertyWithValue.h"
#include "WidgetDllOption.h"
#include <QLineEdit>
#include <QString>
#include <string>
#include <map>
#include <climits>

namespace MantidQt
{
  namespace MantidWidgets
  {
    using namespace Mantid::Kernel;
    
	class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS pythonCalc : public QWidget
    {
      Q_OBJECT
	  
    public:
      const QString& python() const;
	  /** Allows access to m_fails, the list of any validation errors
	  *  return the map is empty if there were no errors, otherwise, the keys are the internal names of the controls, the values the errors
	  */
      const std::map<const QWidget * const, std::string>& invalid() const {return m_fails;}
	  
	  QString run();
      signals:
        /// Emitted to start a (generally small) script running
        void runAsPythonScript(const QString& code);
    
	protected:
      pythonCalc() : m_pyScript(), m_templateH(), m_templateB(), m_fails() {}
      /// this will store the executable python code when it is generated
	  QString m_pyScript;
      /// a copy of the section of the template that contains the Python import statements
      QString m_templateH;
      /// a copy of the section of the template that contains the body of the Python code
      QString m_templateB;
      /// stores the namees of controls with invalid entries as the keys and a discription of the error as the associated value
      std::map<const QWidget * const , std::string> m_fails;
  
      virtual void readFile(const QString &pythonFile);
	  void LEChkCp(QString pythonMark, const QLineEdit * const userVal, Property * const check);
	  std::string replaceErrsFind(QString pythonMark, const QString &setting, Property * const check);
      /// Run a piece of python code and return any output that was written to stdout
      QString runPythonCode(const QString & code, bool no_output = false);
    };
  }
}

#endif //MANTIDQTMANTIDWIDGETS_PYTHONCALC_H_
