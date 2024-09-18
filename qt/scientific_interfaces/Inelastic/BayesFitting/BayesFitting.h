// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "ui_BayesFitting.h"

#include "BayesFittingTab.h"
#include "DllConfig.h"
#include "MantidQtWidgets/Spectroscopy/InelasticInterface.h"

#include "MantidKernel/ConfigService.h"
#include <Poco/NObserver.h>

namespace MantidQt {
namespace CustomInterfaces {
/**
This class defines the Bayes Fitting interface. It handles the creation of the
interface window and handles the interaction between the child tabs on the
window.

@author Samuel Jackson, STFC
*/

class MANTIDQT_INELASTIC_DLL BayesFitting : public InelasticInterface {
  Q_OBJECT

public: // public constants and enums
  /// Enumeration for the index of each tab
  enum TabChoice { RES_NORM, QUASI, STRETCH };

public: // public constructor, destructor and functions
  /// Default Constructor
  BayesFitting(QWidget *parent = nullptr);
  /// Destructor
  ~BayesFitting() override;
  /// Interface name
  static std::string name() { return "Bayes Fitting"; }
  // This interface's categories.
  static QString categoryInfo() { return "Inelastic"; }
  void initLayout() override;

private:
  std::string documentationPage() const override;

  void applySettings(std::map<std::string, QVariant> const &settings) override;

  /// Called upon a close event.
  void closeEvent(QCloseEvent * /*unused*/) override;
  /// handle POCO event
  void handleDirectoryChange(Mantid::Kernel::ConfigValChangeNotification_ptr pNf);
  /// Load default interface settings for each tab
  void loadSettings();

  /// Map of tabs indexed by position on the window
  std::map<unsigned int, BayesFittingTab *> m_bayesTabs;
  /// Change Observer for ConfigService (monitors user directories)
  Poco::NObserver<BayesFitting, Mantid::Kernel::ConfigValChangeNotification> m_changeObserver;
  /// Main interface window
  Ui::BayesFitting m_uiForm;
};
} // namespace CustomInterfaces
} // namespace MantidQt
