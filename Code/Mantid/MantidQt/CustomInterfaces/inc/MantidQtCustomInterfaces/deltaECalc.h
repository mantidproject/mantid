#ifndef MANTIDQTCUSTOMINTERFACES_DELTAECALC_H_
#define MANTIDQTCUSTOMINTERFACES_DELTAECALC_H_

#include "MantidQtMantidWidgets/pythonCalc.h"
#include "ui_ConvertToEnergy.h"
#include <QFileInfo>

namespace MantidQt
{
  namespace CustomInterfaces
  {	
    class deltaECalc : public MantidWidgets::pythonCalc
    {
    public:
      deltaECalc(QWidget * const interface, const Ui::ConvertToEnergy &userSettings, const bool removalBg, 
		 const double TOFWinSt, const double TOFWinEnd);
      void setDiagnosedWorkspaceName(const QString &maskWS);
      void createProcessingScript(const QStringList & inFiles, const QString &whiteB,
				  const QStringList &absInFiles, const QString &absWhiteB,
				  const QString & saveName);
 
      /** Removes the path from the filename passed and replaces extensions with .spe
      * @param inputFilename name of the file that the .SPE file is based on
      */
      static QString SPEFileName(const QString &inputFilename)
      {
        QString root = QFileInfo(inputFilename).baseName();
        if( root.isEmpty() ) return "";
        return root + ".spe";
      }
      std::string insertNumber(const std::string &filename, const int number) const;
    private:
      /// the form that ws filled in by the user
      const Ui::ConvertToEnergy &m_sets;
      /// whether to the remove background count rate from the data
      const bool m_bgRemove;
	    /// used in remove background, the start of the background region
  	  const double m_TOFWinSt;
	    /// used in remove background, the end of the background region
	    const double m_TOFWinEnd;

      QString m_diagnosedWS;
      void addAnalysisOptions(QString & analysisScript);
      void addMaskingCommands(QString & analysisScript);
      QString createPyListAsString(const QStringList & names) const;
  
      // holds the prefix that we give to output workspaces that will be deleted in the Python
      static const QString tempWS;
    };
  }
}

#endif	//MANTIDQTCUSTOMINTERFACES_DELTAECALC_H_
