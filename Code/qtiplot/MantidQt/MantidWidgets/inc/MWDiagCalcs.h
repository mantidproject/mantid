#ifndef MANTIDQTCUSTOMINTERFACES_MWDIAGCALCS_H_
#define MANTIDQTCUSTOMINTERFACES_MWDIAGCALCS_H_

#include "MantidQtMantidWidgets/pythonCalc.h"
#include "MantidQtMantidWidgets/ui_MWDiag.h"
#include "MantidQtMantidWidgets/DiagResults.h"

namespace MantidQt
{
  namespace MantidWidgets
  {
    class whiteBeam1 : public pythonCalc
    {
    public:
      whiteBeam1(QWidget * const interface, const Ui::MWDiag &userSettings);

    private:

      /// the form that ws filled in by the user
      const Ui::MWDiag &m_settings;
      // holds the prefix that we give to output workspaces that will be deleted in the Python
      static const QString tempWS;
    };

    class whiteBeam2 : public pythonCalc
    {
    public:
      whiteBeam2(QWidget * const interface, const Ui::MWDiag &userSettings, const QString &inFile);
	  void incPrevious(const DiagResults::TestSummary &firstTest);
  
    private:

      /// the form that ws filled in by the user
      const Ui::MWDiag &m_settings;
  
      // holds the prefix that we give to output workspaces that will be deleted in the Python
      static const QString tempWS;
    };
	
    class backTest : public pythonCalc
    {
    public:
      backTest(QWidget * const interface, const Ui::MWDiag &userSettings, const std::vector<std::string> &runs);
	  void incFirstTest(const DiagResults::TestSummary &results1);
      void incSecondTest(const DiagResults::TestSummary &results2);
	  void noSecondTest();
    private:

      /// the form that ws filled in by the user
      const Ui::MWDiag &m_settings;
  
      // holds the prefix that we give to output workspaces that will be deleted in the Python
      static const QString tempWS;
    };
  }
}

#endif // MANTIDQTCUSTOMINTERFACES_MWDIAGCALCS_H_