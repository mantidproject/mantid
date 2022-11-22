// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IndirectDataManipulationTab.h"

#include "IndirectSymmetriseModel.h"
#include "IndirectSymmetriseView.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidKernel/System.h"
#include "ui_IndirectSymmetrise.h"

// Suppress a warning coming out of code that isn't ours
#if defined(__INTEL_COMPILER)
#pragma warning disable 1125
#elif defined(__GNUC__)
#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#endif
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
#include "MantidQtWidgets/Common/QtPropertyBrowser/QtCheckBoxFactory"
#if defined(__INTEL_COMPILER)
#pragma warning enable 1125
#elif defined(__GNUC__)
#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif
#endif

namespace MantidQt {
namespace CustomInterfaces {
/** IndirectSymmetrise

  @author Dan Nixon
  @date 23/07/2014
*/
class DLLExport IndirectSymmetrise : public IndirectDataManipulationTab {
  Q_OBJECT

public:
  IndirectSymmetrise(QWidget *parent = nullptr);
  ~IndirectSymmetrise() override;

  void setup() override;
  void run() override;
  bool validate() override;

private slots:
  void handleValueChanged(QtProperty *, double);
  void handleDataReady(QString const &dataName) override;
  void algorithmComplete(bool error);
  void preview();
  void previewAlgDone(bool error);

  void runClicked();
  void saveClicked();

private:
  void setFileExtensionsByName(bool filter) override;

  Mantid::API::AnalysisDataServiceImpl &m_adsInstance;
  std::unique_ptr<IndirectSymmetriseView> m_view;
  std::unique_ptr<IndirectSymmetriseModel> m_model;
};
} // namespace CustomInterfaces
} // namespace MantidQt
