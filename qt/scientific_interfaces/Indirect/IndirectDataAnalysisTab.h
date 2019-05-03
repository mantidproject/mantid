// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACESIDA_IDATAB_H_
#define MANTIDQTCUSTOMINTERFACESIDA_IDATAB_H_

#include "IndirectDataAnalysis.h"
#include "IndirectTab.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"

#include <boost/weak_ptr.hpp>

class QwtPlotCurve;
class QwtPlot;
class QSettings;
class QString;

namespace MantidQt {
namespace MantidWidgets {
class RangeSelector;
}
} // namespace MantidQt

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
#include "MantidQtWidgets/Common/QtPropertyBrowser/qteditorfactory.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qtpropertymanager.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qttreepropertybrowser.h"
#if defined(__INTEL_COMPILER)
#pragma warning enable 1125
#elif defined(__GNUC__)
#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif
#endif

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
class DLLExport IndirectDataAnalysisTab : public IndirectTab {
  Q_OBJECT

public:
  /// Constructor
  IndirectDataAnalysisTab(QWidget *parent = nullptr);

  /// Loads the tab's settings.
  void loadTabSettings(const QSettings &settings);

protected:
  /// Function to run a string as python code
  void runPythonScript(const QString &pyInput);

  /// Retrieve input workspace
  Mantid::API::MatrixWorkspace_sptr inputWorkspace() const;

  /// Set input workspace
  void setInputWorkspace(Mantid::API::MatrixWorkspace_sptr inputWorkspace);

  /// Retrieve preview plot workspace
  Mantid::API::MatrixWorkspace_sptr previewPlotWorkspace();

  /// Set preview plot workspace
  void setPreviewPlotWorkspace(
      Mantid::API::MatrixWorkspace_sptr previewPlotWorkspace);

  /// Retrieve the selected spectrum
  int selectedSpectrum() const;

  /// Retrieve the selected minimum spectrum
  int minimumSpectrum() const;

  /// Retrieve the selected maximum spectrum
  int maximumSpectrum() const;

  void plotInput(MantidQt::MantidWidgets::PreviewPlot *previewPlot);

  void clearAndPlotInput(MantidQt::MantidWidgets::PreviewPlot *fitPreviewPlot,
                         MantidQt::MantidWidgets::PreviewPlot *diffPreviewPlot);

  void updatePlot(const std::string &outputWSName, size_t index,
                  MantidQt::MantidWidgets::PreviewPlot *fitPreviewPlot,
                  MantidQt::MantidWidgets::PreviewPlot *diffPreviewPlot);

  void updatePlot(Mantid::API::WorkspaceGroup_sptr workspaceGroup, size_t index,
                  MantidQt::MantidWidgets::PreviewPlot *fitPreviewPlot,
                  MantidQt::MantidWidgets::PreviewPlot *diffPreviewPlot);

  void updatePlot(Mantid::API::WorkspaceGroup_sptr outputWS,
                  MantidQt::MantidWidgets::PreviewPlot *fitPreviewPlot,
                  MantidQt::MantidWidgets::PreviewPlot *diffPreviewPlot);

  void updatePlot(const std::string &workspaceName,
                  MantidQt::MantidWidgets::PreviewPlot *fitPreviewPlot,
                  MantidQt::MantidWidgets::PreviewPlot *diffPreviewPlot);

  void updatePlot(Mantid::API::MatrixWorkspace_sptr outputWS,
                  MantidQt::MantidWidgets::PreviewPlot *fitPreviewPlot,
                  MantidQt::MantidWidgets::PreviewPlot *diffPreviewPlot);

  void updatePlotRange(const QString &rangeName,
                       MantidQt::MantidWidgets::PreviewPlot *previewPlot,
                       const QString &startRangePropName = "",
                       const QString &endRangePropName = "");

  /// DoubleEditorFactory
  DoubleEditorFactory *m_dblEdFac;
  /// QtCheckBoxFactory
  QtCheckBoxFactory *m_blnEdFac;

protected slots:
  /// Slot that can be called when a user eidts an input.
  void inputChanged();

  /// Plots the current preview data
  void plotCurrentPreview();

  /// Sets the selected spectrum
  virtual void setSelectedSpectrum(int spectrum);

  /// Sets the maximum spectrum
  void setMaximumSpectrum(int spectrum);

  /// Sets the minimum spectrum
  void setMinimumSpectrum(int spectrum);

private:
  /// Overidden by child class.
  void setup() override = 0;
  /// Overidden by child class.
  void run() override = 0;
  /// Overidden by child class.
  bool validate() override = 0;
  /// Overidden by child class.
  virtual void loadSettings(const QSettings &settings) = 0;

  /// A pointer to the parent (friend) IndirectDataAnalysis object.
  IndirectDataAnalysis *m_parent;
  Mantid::API::MatrixWorkspace_sptr m_inputWorkspace;
  boost::weak_ptr<Mantid::API::MatrixWorkspace> m_previewPlotWorkspace;
  int m_selectedSpectrum;
  int m_minSpectrum;
  int m_maxSpectrum;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_IDATAB_H_ */
