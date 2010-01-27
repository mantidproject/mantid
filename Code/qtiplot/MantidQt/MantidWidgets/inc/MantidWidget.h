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
	  //??STEVES make this an ordinary member soon, move it to the file widget?
	  /** removes the path and extension from the filename
      * @param inputFilename name of the file that the .SPE file is based on
      */
      static QString removePath(const std::string &inputFilename)
      {
        std::string root = Poco::Path(inputFilename).getBaseName();
        return root.empty() ? "" : QString::fromStdString(root);
      }
	protected:
	  /// A list of labels to use as validation markers
      QHash<const QWidget * const, QLabel *> m_validators;
	  
      MantidWidget(){}
	  void renameWorkspace(const QString &oldName, const QString &newName);
      void setupValidator(QLabel *star);
	  QLabel* newStar(const QGroupBox * const UI, int valRow, int valCol);
      QLabel* newStar(QGridLayout * const lay, int valRow, int valCol);
	  void hideValidators();
    };
  }
}

#endif //MANTIDQTMANTIDWIDGETS_MANTIDWIDGETS_H_