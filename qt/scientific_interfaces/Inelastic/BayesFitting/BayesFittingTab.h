// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "MantidQtWidgets/Spectroscopy/InelasticTab.h"

#include <QSettings>

// Suppress a warning coming out of code that isn't ours
#if defined(__INTEL_COMPILER)
#pragma warning disable 1125
#elif defined(__GNUC__)
#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#endif
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
#include "MantidQtWidgets/Common/QtPropertyBrowser/DoubleEditorFactory.h"

#include <MantidQtWidgets/Common/AlgorithmRunner.h>
#include <MantidQtWidgets/Common/IAlgorithmRunnerSubscriber.h>

#if defined(__INTEL_COMPILER)
#pragma warning enable 1125
#elif defined(__GNUC__)
#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif
#endif

namespace MantidQt {
namespace CustomInterfaces {
/**
This class defines a abstract base class for the different tabs of the
Bayes Fitting interface.
Any joint functionality shared between each of the tabs should be implemented
here as well as defining shared member functions.

@author Samuel Jackson, STFC
*/

/// precision of double properties in bayes tabs
static const unsigned int NUM_DECIMALS = 6;
/// precision for integer properties in bayes tabs
static const unsigned int INT_DECIMALS = 0;

struct BackgroundType {
  inline static const std::string SLOPING = "Sloping";
  inline static const std::string FLAT = "Flat";
  inline static const std::string ZERO = "Zero";
  inline static const std::string LINEAR = "Linear";
};

class MANTIDQT_INELASTIC_DLL BayesFittingTab : public InelasticTab, public MantidQt::API::IAlgorithmRunnerSubscriber {
  Q_OBJECT

public:
  BayesFittingTab(QWidget *parent = nullptr, std::unique_ptr<API::IAlgorithmRunner> algorithmRunner = nullptr);
  ~BayesFittingTab() override;

  /// Base methods implemented in derived classes
  virtual void loadSettings(const QSettings &settings) = 0;

  /// Prevent loading of data with incorrect naming
  void filterInputData(bool filter);

  virtual void applySettings(std::map<std::string, QVariant> const &settings);
  void notifyBatchComplete(API::IConfiguredAlgorithm_sptr &algorithm, bool error) override;

protected slots:
  /// Slot to update the guides when the range properties change
  virtual void updateProperties(QtProperty *prop, double val);

protected:
  virtual void runComplete(IAlgorithm_sptr const &algorithm, bool const error) {
    (void)algorithm;
    (void)error;
  };
  /// Formats the tree widget to make it easier to read
  void formatTreeWidget(QtTreePropertyBrowser *treeWidget, QMap<QString, QtProperty *> const &properties) const;
  /// Tree of the properties
  QtTreePropertyBrowser *m_propTree;
  std::unique_ptr<MantidQt::API::IAlgorithmRunner> m_algorithmRunner;

private:
  virtual void setFileExtensionsByName(bool filter);
  virtual void setLoadHistory(bool doLoadHistory) { (void)doLoadHistory; }
};

} // namespace CustomInterfaces
} // namespace MantidQt
