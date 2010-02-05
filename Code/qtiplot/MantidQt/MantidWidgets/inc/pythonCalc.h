#ifndef MANTIDQTMANTIDWIDGETS_PYTHONCALC_H_
#define MANTIDQTMANTIDWIDGETS_PYTHONCALC_H_

#include "Poco/Path.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidQtMantidWidgets/MantidWidget.h"
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
	class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS pythonCalc : public MantidWidget
    {
      Q_OBJECT
	  
    public:
      const QString& python() const;
	  /** Allows access to m_fails, the list of any validation errors
	  *  return the map is empty if there were no errors, otherwise, the keys are the internal names of the controls, the values the errors
	  */
	  QString checkNoErrors(const QHash<const QWidget * const, QLabel *> &validLbls) const;
	  
	  QString run();
      signals:
        /// Emitted to start a script running
        void runAsPythonScript(const QString& code);
    
	protected:
      pythonCalc(const QWidget * const interface);
      /// this will store the executable python code when it is generated
	  QString m_pyScript;
      /// a copy of the section of the template that contains the Python import statements
      QString m_templateH;
      /// a copy of the section of the template that contains the body of the Python code
      QString m_templateB;
      /// stores the namees of controls with invalid entries as the keys and a discription of the error as the associated value
      std::map<const QWidget * const , std::string> m_fails;
  
      virtual void readFile(const QString &pythonFile);
	  void LEChkCp(QString pythonMark, const QLineEdit * const userVal, Mantid::Kernel::Property * const check);
	  std::string replaceErrsFind(QString pythonMark, const QString &setting, Mantid::Kernel::Property * const check);
      /// Run a piece of python code and return any output that was written to stdout
      QString runPythonCode(const QString & code, bool no_output = false);
	  std::string vectorToTupple(const std::vector<std::string> &vec) const;
	private:
      /// Copy construction is not allowed because the runAsPythonScript() signal wouldn't be connected
      pythonCalc( const pythonCalc& right );
	  /// Copy assignment is not allowed because the runAsPythonScript() signal wouldn't be connected
	  pythonCalc& operator=( const pythonCalc& right );
    };
  }
}

#endif //MANTIDQTMANTIDWIDGETS_PYTHONCALC_H_
