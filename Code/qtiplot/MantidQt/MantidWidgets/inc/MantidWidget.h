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
	  public:
      signals:
        /// this signal should be connected to Qtiplot to run python scripts 
        void runAsPythonScript(const QString& code);
    protected:
	    /// A list of labels to use as validation markers
      QHash<const QWidget * const, QLabel *> m_validators;
      ///to stop MantidPlot from being terminated need to check that this is false before a script is run, then set it to true and return it to false once the script has terminated
      static bool g_pythonRunning;
	  
      MantidWidget(QWidget *interface);
  	  void renameWorkspace(const QString &oldName, const QString &newName);
      void setupValidator(QLabel *star);
	    QLabel* newStar(const QGroupBox * const UI, int valRow, int valCol);
      QLabel* newStar(QGridLayout * const lay, int valRow, int valCol);
	    void hideValidators();
      QString runPythonCode(const QString & code, bool no_output = false);
      void runPython(const QString &code);
    };
  }
}

#endif //MANTIDQTMANTIDWIDGETS_MANTIDWIDGETS_H_