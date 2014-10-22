#ifndef MANTID_MANTIDWIDGETS_HINTINGLINEEDIT_H_
#define MANTID_MANTIDWIDGETS_HINTINGLINEEDIT_H_

#include "MantidKernel/System.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "WidgetDllOption.h"

#include <map>
#include <QtGui>
#include <string>

//------------------------------------------------------------------------------
// Forward declaration
//------------------------------------------------------------------------------
namespace MantidQt
{
  namespace MantidWidgets
  {
    class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS HintingLineEdit : public QLineEdit
    {
      Q_OBJECT
    public:
      HintingLineEdit(QWidget *parent, const std::map<std::string,std::string> &hints);
      virtual ~HintingLineEdit();
    protected:
      void updateMatches();
      std::string m_curKey;
      std::map<std::string,std::string> m_matches;
      std::map<std::string,std::string> m_hints;
    public slots:
      void showHint();
      void updateHint(const QString& text);
    };
  } //namespace MantidWidgets
} //namepsace MantidQt

#endif /* MANTID_MANTIDWIDGETS_HINTINGLINEEDIT_H_ */
