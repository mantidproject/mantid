// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_SANSDIAGNOSTICS_H_
#define MANTIDQTCUSTOMINTERFACES_SANSDIAGNOSTICS_H_

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidGeometry/IDetector.h"
#include "MantidQtWidgets/Common/UserSubWindow.h"
#include "ui_SANSRunWindow.h"

namespace MantidQt {
namespace CustomInterfaces {

/**
The RectDetectorDetails class stores the rectangular detector name
and minimum and maximum detector id

@author Sofia Antony, Rutherford Appleton Laboratory
@date 03/02/2011
*/
class RectDetectorDetails {
public:
  /// constructor
  RectDetectorDetails() : m_minDetId(0), m_maxDetId(0) {}
  /// destructor
  ~RectDetectorDetails() {}
  /// set minimum detector id
  inline void setMinimumDetectorId(const Mantid::detid_t minDetId) {
    m_minDetId = minDetId;
  }
  /// set maximum detector id
  inline void setMaximumDetectorId(const Mantid::detid_t maxDetId) {
    m_maxDetId = maxDetId;
  }
  /// set detector name
  void setDetectorName(const QString &detName) { m_detName = detName; }

  /// get minimum detector id
  inline Mantid::detid_t getMinimumDetectorId() { return m_minDetId; }
  /// get maximum detector id
  inline Mantid::detid_t getMaximumDetectorId() { return m_maxDetId; }
  /// get detector name
  const QString &getDetectorName() { return m_detName; }

private:
  /// minimum detecor id
  Mantid::detid_t m_minDetId;
  /// maximum detecor id
  Mantid::detid_t m_maxDetId;
  /// detector name
  QString m_detName;
};

/**
The SANSDiagnostics is responsible for the diagnostics tab of SANS
interface.

@author Sofia Antony, Rutherford Appleton Laboratory
@date 27/01/2011
*/
class SANSDiagnostics : public MantidQt::API::UserSubWindow {
  Q_OBJECT
public:
  /// Default Constructor
  SANSDiagnostics(QWidget *parent, Ui::SANSRunWindow *ParWidgets);
  /// Destructor
  ~SANSDiagnostics() override;

signals:
  void applyMask(const QString &wsName, bool time_pixel);

private:
  /// Initilaise the current tab
  void initLayout() override;
  /// set tool tips
  void setToolTips();
  /// execute sumrowcolumn algorithm
  bool executeSumRowColumn(const std::vector<unsigned int> &values,
                           const QString ipws, const QString &op,
                           const QString &orientation);
  /// load the settings from the registry
  void loadSettings();
  /// save settings
  void saveSettings();
  /// returns the total number of periods
  int getTotalNumberofPeriods();

  /// returns true if the user string contains sequential data
  bool isSequentialValues(const std::vector<unsigned int> &values);
  /// returns the workspace name
  QString getWorkspaceNameFileName(const QString &fileName);
  /// returns the filename
  QString getFileName();
  /// run loadraw algorithm
  bool runLoadAlgorithm(const QString &fileName, const QString &specMin = "-1",
                        const QString &specMax = "-1");
  /// returns sumrowcolumn script
  bool runsumRowColumn(const QString ipwsName, const QString &opwsName,
                       const QString &orientation, const QString &hvMin,
                       const QString &hvMax);
  // get rectangular detector details
  std::vector<boost::shared_ptr<RectDetectorDetails>>
  rectangularDetectorDetails(Mantid::API::Workspace_sptr &ws_sptr);
  /// returns sumspectra script
  bool runsumSpectra(const QString &ipwsName, const QString &opwsName,
                     const QString &wsStartIndex, const QString &wsEndIndex);
  /// display total number of periods box
  void displayTotalPeriods();
  /// display rectangualr detectors
  void displayRectangularDetectors(const QString &wsName);

  // This method executes loadraw and sumrow column algorithm
  void IntegralClicked(const QString &range, const QString &orientation,
                       const QString &specMin, const QString &specMax,
                       const QString &detectorName,
                       const QString &integrationType, bool bMask,
                       bool time_pixel);

  // This method executes sumspectra algorithm
  void TimeIntegralClicked(const QString &range, const QString &specMin,
                           const QString &specMax, const QString &opws,
                           bool pixel_mask);

  /// plot spectrum
  void plotSpectrum(const QString &wsName, int specNum);

  /// hide the group boxes
  void disableDetectorGroupBoxes(bool bStatus);

  /// minimum and maximum spectrum Nos for detector
  void minandMaxSpectrumNos(const std::vector<Mantid::specnum_t> &specList,
                            QString &minSpec, QString &maxSpec);

  /// get workspaceIndexes from spectrum list
  void getWorkspaceIndexes(const Mantid::API::MatrixWorkspace_sptr &mws_sptr,
                           const std::vector<Mantid::specnum_t> &specList,
                           QString &startWSIndex, QString &endWSIndex);
  /// get spectra list from workspace.
  void getSpectraList(const Mantid::API::MatrixWorkspace_sptr &mws_sptr,
                      const Mantid::detid_t detNum,
                      std::vector<Mantid::specnum_t> &specList);

  /// get detector name
  const QString getDetectorName(int index);

  /// get the user entered period number
  int getPeriodNumber();
  /// get the member workspace name for the period
  QString getMemberWorkspace(int period);

  /// returns true if the loaded workspace is multiperiod(group workspace)
  bool isMultiPeriod();
  /// This method returns name of the   workspace which is to be
  /// used as the i/p  for sumrowcolumn or sumspectra algorithm
  QString getWorkspaceToProcess();
  /// This method returns name of the whole workspace loaded from
  /// the data file, which will be further processed to separate the data from
  /// the
  /// banks (Rear-Detector and Front-Detector)
  QString getDataLoadedWorkspace();

  /// returns true if the spec min and max are in the valid range
  bool isValidSpectra(const QString &specMin, const QString &specMax);
  // disable total periods boxes and labels
  void changePeriodsControls(bool bEnable);
  /// applys the time channel or pixel mask
  void maskDetector(const QString &wsName, bool bMask, bool time_pixel);
  // getvalues of HVMin and HvMax values for sumrowcolumn algorithm
  void HVMinHVMaxStringValues(const int minVal, const int maxVal,
                              QString &hvMin, QString &hvMax);
  /// Create the name for the outputworkspace
  QString createOutputWorkspaceName(QString originalWorkspaceName,
                                    QString detectorName,
                                    QString integrationType, QString min,
                                    QString max);

private:
  QString m_dataDir;       ///< default data search directory
  QString m_settingsGroup; ///< settings used for permanent store.
  QString m_wsName;        ///<  workspace name created by load raw
  QString m_fileName;      ///< name of the file
  QString m_spec_min;      ///< spectrum min
  QString m_spec_max;      ///< spectrum max
  QString m_outws_load;    ///< output workspace for load algorithm
  QString m_memberwsName;
  /// set to point to the object that has the Add Files controls
  Ui::SANSRunWindow *m_SANSForm;
  // set to a pointer to the parent form
  QWidget *parForm;

  int m_totalPeriods;               ///< total periods
  int m_Period;                     ///< Current period
  std::vector<std::string> m_wsVec; ///< workspace vector
  std::vector<boost::shared_ptr<RectDetectorDetails>> m_rectDetectors;

private slots:

  /// first detector horizontal integral button clicked.
  void firstDetectorHorizontalIntegralClicked();
  /// first detector vertical integral button clicked.
  void firstDetectorVerticalIntegralClicked();
  /// first detector Time Integral button clicked.
  void firstDetectorTimeIntegralClicked();
  /// second  detector horizontal integral button clicked.
  void secondDetectorHorizontalIntegralClicked();
  /// second detector vertical integral button clicked.
  void secondDetectorVerticalIntegralClicked();
  /// second detector Time Integral button clicked.
  void secondDetectorTimeIntegralClicked();

  /// display the detector banks for the selected period
  void displayDetectorBanksofMemberWorkspace();

  /// load the first spectrum using the user given file.
  void loadFirstSpectrum();
  /// enable the mask controls in the diagnostics UI
  void enableMaskFileControls();
};
} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_SANSADDFILES_H_
