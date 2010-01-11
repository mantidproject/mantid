#ifndef MANTIDQTCUSTOMINTERFACES_MWDIAGCALCS_H_
#define MANTIDQTCUSTOMINTERFACES_MWDIAGCALCS_H_

#include "MantidQtMantidWidgets/pythonCalc.h"
#include "MantidQtMantidWidgets/MWDiag.h"
#include "MantidQtMantidWidgets/DiagResults.h"

namespace MantidQt
{
  namespace MantidWidgets
  {
    class whiteBeam1 : public pythonCalc
    {
    public:
      explicit whiteBeam1(const Ui::MWDiag &userSettings);

    private:

      /// the form that ws filled in by the user
      const Ui::MWDiag &m_settings;
      // holds the prefix that we give to output workspaces that will be deleted in the Python
      static const QString tempWS;
    };

    class whiteBeam2 : public pythonCalc
    {
    public:
      explicit whiteBeam2(const Ui::MWDiag &userSettings);
	  void incPrevious(DiagResults::TestSummary &firstTest);
  
    private:

      /// the form that ws filled in by the user
      const Ui::MWDiag &m_settings;
  
      // holds the prefix that we give to output workspaces that will be deleted in the Python
      static const QString tempWS;
    };
  }
}

#endif // MANTIDQTCUSTOMINTERFACES_MWDIAGCALCS_H_