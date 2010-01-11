#include "MantidQtMantidWidgets/pythonCalc.h"
#include "MantidQtCustomInterfaces/ui_Excitations.h"
#include <vector>
#include <QString>
#include <string>

namespace MantidQt
{
  namespace CustomInterfaces
  {
    using namespace MantidWidgets;
	
    class deltaECalc : public pythonCalc
    {
    public:
      //??STEVES? move this into the file widget
      class FileInput
      {
      private:
        QLineEdit &m_lineEdit;
	    QComboBox &m_box;
        std::vector<std::string> m_files;
      public:
        FileInput(QLineEdit &num, QComboBox &instr);
        const std::vector<std::string>& getRunFiles();
        void readComasAndHyphens(const std::string &in, std::vector<std::string> &out);
      };
  
      deltaECalc(const Ui::Excitations &userSettings, FileInput &runFiles);
      void maskDetects(const QString &maskWS);
	  /** removes the path from the filename passed and replaces extensions with .spe
      * @param inputFilename name of the file that the .SPE file is based on
      */
      static QString SPEFileName(const std::string &inputFilename)
      {
        std::string root = Poco::Path(inputFilename).getBaseName();
        return root.empty() ? "" : QString::fromStdString(root)+".spe";
      }
      std::string insertNumber(const std::string &filename, const int number) const;
    private:
      /// the form that ws filled in by the user
      const Ui::Excitations &m_sets;

	  QString createProcessingScript(const std::string &inFiles, const std::string &oName);
	  void createGetEIStatmens(QString &newScr);
	  void createNormalizationStatmens(QString &newScr);
      void createOutputStatmens(QString &WSName, QString &newScr);
      QString getScaling() const;
      QString getNormalization() const;
      void LEChkCpIn(QString &text, QString pythonMark, QLineEdit * userVal, Property * const check);
	  std::string replaceInErrsFind(QString &text, QString pythonMark, const QString &setting, Property * const check) const;
      void renameWorkspace(const QString &name);
  
      // holds the prefix that we give to output workspaces that will be deleted in the Python
      static const QString tempWS;
    };
  }
}
