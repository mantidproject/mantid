
#ifndef INSTRUMENTWINDOW_H_
#define INSTRUMENTWINDOW_H_

#include "Instrument3DWidget.h"
#include "InstrumentTreeWidget.h"
#include "../../MdiSubWindow.h"
#include "BinDialog.h"
#include <string>
#include <vector>
#include "qwt_scale_widget.h"

namespace Mantid
{

namespace API
{
  class MatrixWorkspace;
}

}

// Qt forward declarations
class QPushButton;
class QDialog;
class QSlider;
class QSpinBox;
class QTabWidget;
class QLineEdit;
class QLabel;
class QCheckBox;
class QComboBox;

/*!
  \class  InstrumentWindow
  \brief  This is the main window for the control of display on geometry
  \author Srikanth Nagella
  \date   September 2008
  \version 1.0

  This is a QT widget for the controls and display of instrument geometry

  Copyright &copy; 2007 STFC Rutherford Appleton Laboratories

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
*/
class InstrumentWindow : public MdiSubWindow
{
	Q_OBJECT
public:
	InstrumentWindow(const QString& label = QString(), ApplicationWindow *app = 0, const QString& name = QString(), Qt::WFlags f = 0);
	~InstrumentWindow();
	void setWorkspaceName(std::string wsName);
        void updateWindow();
        void showWindow();

  /// Alter data from a script. These just foward calls to the 3D widget
  void setColorMapMinValue(double minValue);
  void setColorMapMaxValue(double maxValue);
  void setColorMapRange(double minValue, double maxValue);
  void setDataMappingIntegral(double minValue,double maxValue);
  void selectComponent(const QString & name);

public slots:
	void modeSelectButtonClicked();
	void selectBinButtonClicked();
	void spectraInformation(int);
	void detectorInformation(int value);
	void detectorHighlighted(int detectorId,int spectraid,int count);
	void spectraListInformation(const std::vector<int>&);
	void detectorListInformation(const std::vector<int>&);
	void spectraInfoDialog();
	void spectraGroupInfoDialog();
	void changeColormap(const QString & filename = "");
    void sendPlotSpectraSignal();
	void sendPlotSpectraGroupSignal();
	void minValueChanged();
	void maxValueChanged();
	void setViewDirection(const QString&);
	void componentSelected(const QItemSelection&, const QItemSelection&);
	void pickBackgroundColor();

signals:
    void plotSpectra(const QString&,int);
	void plotSpectraList(const QString&,const std::vector<int>&);

private slots:
        void scaleTypeChanged(int);

private:

	void loadSettings();
	void saveSettings();
	void renderInstrument(Mantid::API::MatrixWorkspace* workspace);
        void setupColorBarScaling();

	QLabel*      mInteractionInfo;
	QTabWidget*  mControlsTab;
    QMenu*       mPopupContext; ///< Popup menu for detector picking
	QMenu*       mDetectorGroupPopupContext; ///< Popup menu for detector picking
	QPushButton* mSelectButton; ///< Select the mode Pick/Normal
	QPushButton* mSelectColormap; ///< Select colormap button
	Instrument3DWidget* mInstrumentDisplay; ///< This is the opengl 3d widget for instrument
	int          mSpectraIDSelected; ///< spectra index id
	int          mDetectorIDSelected; ///< detector id
	std::vector<int> mSpectraIDSelectedList;
	std::vector<int> mDetectorIDSelectedList;
	QwtScaleWidget* mColorMapWidget; ///< colormap display widget
	QLineEdit*   mMinValueBox;       ///< Minvalue for the colormap
	QLineEdit*   mMaxValueBox;       ///< Max value for the colormap
        QComboBox *mScaleOptions;
	BinDialog*   mBinMapDialog;
	InstrumentTreeWidget* mInstrumentTree; ///< Widget to display instrument tree
        QCheckBox *mLightingToggle; ///< A tick box to toggle the lighting
  
        std::string mWorkspaceName; ///The name of workpace that this window is associated with
        QString mDefaultColorMap; ///The full path of the default color map
        QString mCurrentColorMap;
};

#endif /*INSTRUMENTWINDOW_H_*/

