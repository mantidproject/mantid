#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECT_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECT_H_

#include "MantidQtAPI/UserSubWindow.h"
#include "MantidQtCustomInterfaces/ui_ConvertToEnergy.h"

//-----------------------------------------------------
// Forward declarations
//-----------------------------------------------------

namespace MantidQt
{
  namespace CustomInterfaces
  {

    class Indirect : public MantidQt::API::UserSubWindow
    {
      Q_OBJECT

    public:
      /// explicit constructor, not to allow any overloading
      explicit Indirect(QWidget *parent, Ui::ConvertToEnergy & uiForm);

      /// Initialize the layout
      virtual void initLayout();
      /// run Python-based initialisation commands
      virtual void initLocalPython();
      /// open the wiki page for this interface in a web browser
      void helpClicked();
      /// perform whatever operations needed for analysis
      void runClicked();
      /// gather necessary information from Instument Definition Files
      virtual void setIDFValues(const QString & prefix);

    private:
	  /// user interface form object
      Ui::ConvertToEnergy m_uiForm;
	  /// get path to instrument definition file
	  QString getIDFPath(const QString& name);
	  /// populate the spectra ranges for the "Calibration" tab.
	  void getSpectraRanges(const QString& defFile);
	  /// clear various line edit boxes
	  void clearReflectionInfo();

	private slots:
	  void analyserSelected(int index);
	  void reflectionSelected(int index);
	  void backgroundRemoval();
	  void plotRaw();
	  void rebinData();
	  void calibPlot();
	  void calibCreate();

	private: /*
	  int m_minSpectra, m_maxSpectra;
	  int m_peakStart, m_peakEnd;
	  int m_backStart, m_backEnd;
	  double m_eFixed; */
    };
  }
}

#endif // MANTIDQTCUSTOMINTERFACES_INDIRECT_H_
