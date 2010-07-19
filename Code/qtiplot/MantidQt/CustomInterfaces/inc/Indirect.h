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
	/// Forward Declarations
    class Background;
	
	/** 
    This class defines handles the ConvertToEnergy interface for indirect instruments (IRIS/OSIRIS).    

    @author Michael Whitty
	@author Martyn Gigg

    Copyright &copy; 2010 STFC Rutherford Appleton Laboratories

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>    
    */

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

	  /// get path to instrument definition file
	  QString getIDFPath(const QString& name);
	  /// populate the spectra ranges for the "Calibration" tab.
	  void getSpectraRanges(const QString& defFile);
	  /// clear various line edit boxes
	  void clearReflectionInfo();
	  /// create the mapping/grouping file
	  QString createMapFile(const QString& groupType);

	private slots:
	  void analyserSelected(int index); ///< set up cbReflection based on Analyser selection
	  void reflectionSelected(int index); ///< set up parameter file values based on reflection
	  void mappingOptionSelected(const QString& groupType); ///< change ui to display appropriate options
	  void browseRun(); ///< show openFileDialog for run file
	  void browseCalib(); ///< show openFileDialog for calibration file
	  void browseMap(); ///< show openFileDialog for mapping file
	  void browseSave(); ///< show saveFileDialog for save file
	  void backgroundClicked(); ///< handles showing and hiding m_backgroundDialog
	  void plotRaw(); ///< plot raw data from instrument
	  void rebinData(); ///< rebin transformed data
	  void calibPlot(); ///< plot raw data for calibration run
	  void calibCreate(); ///< create calibration file

	private:
	  /// user interface form object
      Ui::ConvertToEnergy m_uiForm;
	  /// background removal dialog
	  Background *m_backgroundDialog;

	  QString m_dataDir; ///< default data search directory
	  QString m_saveDir; ///< default data save directory
	  /// whether user has set values for BG Removal
	  bool m_bgRemoval;
    };
  }
}

#endif // MANTIDQTCUSTOMINTERFACES_INDIRECT_H_
