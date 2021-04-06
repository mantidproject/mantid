// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IndirectDataAnalysis.h"
#include "IndirectPlotOptionsPresenter.h"
#include "IndirectTab.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"

#include <memory>

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
class MANTIDQT_INDIRECT_DLL IndirectDataAnalysisTab : public IndirectTab {
  Q_OBJECT

public:
  /// Constructor
  IndirectDataAnalysisTab(QWidget *parent = nullptr);
  virtual ~IndirectDataAnalysisTab() override = default;

  /// Set the presenter for the output plotting options
  void setOutputPlotOptionsPresenter(std::unique_ptr<IndirectPlotOptionsPresenter> presenter);
  /// Set the active workspaces used in the plotting options
  void setOutputPlotOptionsWorkspaces(std::vector<std::string> const &outputWorkspaces);
  /// Clear the workspaces held by the output plotting options
  void clearOutputPlotOptionsWorkspaces();

  /// Loads the tab's settings.
  void loadTabSettings(const QSettings &settings);

  /// Prevent loading of data with incorrect naming
  void filterInputData(bool filter);

protected:
  /// Function to run a string as python code
  void runPythonScript(const QString &pyInput);

  /// Retrieve input workspace
  Mantid::API::MatrixWorkspace_sptr getInputWorkspace() const;

  /// Set input workspace
  void setInputWorkspace(Mantid::API::MatrixWorkspace_sptr inputWorkspace);

  /// Retrieve preview plot workspace
  Mantid::API::MatrixWorkspace_sptr getPreviewPlotWorkspace();

  /// Set preview plot workspace
  void setPreviewPlotWorkspace(const Mantid::API::MatrixWorkspace_sptr &previewPlotWorkspace);

  /// Retrieve the selected spectrum
  int getSelectedSpectrum() const;

  /// Retrieve the selected minimum spectrum
  int getMinimumSpectrum() const;

  /// Retrieve the selected maximum spectrum
  int getMaximumSpectrum() const;

  void plotInput(MantidQt::MantidWidgets::PreviewPlot *previewPlot);

  void clearAndPlotInput(MantidQt::MantidWidgets::PreviewPlot *fitPreviewPlot,
                         MantidQt::MantidWidgets::PreviewPlot *diffPreviewPlot);

  void updatePlot(const std::string &outputWSName, size_t index, MantidQt::MantidWidgets::PreviewPlot *fitPreviewPlot,
                  MantidQt::MantidWidgets::PreviewPlot *diffPreviewPlot);

  void updatePlot(const Mantid::API::WorkspaceGroup_sptr &workspaceGroup, size_t index,
                  MantidQt::MantidWidgets::PreviewPlot *fitPreviewPlot,
                  MantidQt::MantidWidgets::PreviewPlot *diffPreviewPlot);

  void updatePlot(const Mantid::API::WorkspaceGroup_sptr &outputWS,
                  MantidQt::MantidWidgets::PreviewPlot *fitPreviewPlot,
                  MantidQt::MantidWidgets::PreviewPlot *diffPreviewPlot);

  void updatePlot(const std::string &workspaceName, MantidQt::MantidWidgets::PreviewPlot *fitPreviewPlot,
                  MantidQt::MantidWidgets::PreviewPlot *diffPreviewPlot);

  void updatePlot(const Mantid::API::MatrixWorkspace_sptr &outputWS,
                  MantidQt::MantidWidgets::PreviewPlot *fitPreviewPlot,
                  MantidQt::MantidWidgets::PreviewPlot *diffPreviewPlot);

  /// DoubleEditorFactory
  DoubleEditorFactory *m_dblEdFac;
  /// QtCheckBoxFactory
  QtCheckBoxFactory *m_blnEdFac;

private:
  virtual void loadSettings(const QSettings &settings) = 0;
  virtual void setFileExtensionsByName(bool filter) = 0;
  virtual void setBrowserWorkspace(){};

  /// A pointer to the parent (friend) IndirectDataAnalysis object.
  IndirectDataAnalysis *m_parent;
  Mantid::API::MatrixWorkspace_sptr m_inputWorkspace;
  std::weak_ptr<Mantid::API::MatrixWorkspace> m_previewPlotWorkspace;
  int m_selectedSpectrum;
  int m_minSpectrum;
  int m_maxSpectrum;
  std::unique_ptr<IndirectPlotOptionsPresenter> m_plotOptionsPresenter;

protected slots:
  /// Slot that can be called when a user edits an input.
  void inputChanged();

  /// Plots the current preview data
  void plotCurrentPreview();

  /// Sets the selected spectrum
  virtual void setSelectedSpectrum(int spectrum);

  /// Sets the maximum spectrum
  void setMaximumSpectrum(int spectrum);

  /// Sets the minimum spectrum
  void setMinimumSpectrum(int spectrum);
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
