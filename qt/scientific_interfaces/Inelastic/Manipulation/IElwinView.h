// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtWidgets/Common/FileFinderWidget.h"
#include "MantidQtWidgets/Common/IAddWorkspaceDialog.h"
#include <QModelIndexList>
#include <QStringList>

#include <memory>
#include <string>
#include <tuple>

namespace MantidQt {
namespace CustomInterfaces {

class OutputPlotOptionsView;
class IElwinPresenter;

class MANTIDQT_INELASTIC_DLL IElwinView {

public:
  virtual void subscribePresenter(IElwinPresenter *presenter) = 0;
  virtual void setup() = 0;
  virtual OutputPlotOptionsView *getPlotOptions() const = 0;
  virtual void setFBSuffixes(QStringList const &suffix) = 0;

  virtual void setAvailableSpectra(MantidWidgets::WorkspaceIndex minimum, MantidWidgets::WorkspaceIndex maximum) = 0;
  virtual void setAvailableSpectra(const std::vector<MantidWidgets::WorkspaceIndex>::const_iterator &from,
                                   const std::vector<MantidWidgets::WorkspaceIndex>::const_iterator &to) = 0;

  virtual void newPreviewFileSelected(const Mantid::API::MatrixWorkspace_sptr &workspace) = 0;
  virtual int getCurrentInputIndex() = 0;
  virtual MantidQt::API::FileFinderWidget *getFileFinderWidget() = 0;

  virtual void plotInput(Mantid::API::MatrixWorkspace_sptr inputWS, int spectrum) = 0;
  virtual void newInputFiles() = 0;
  virtual void newInputFilesFromDialog(std::vector<std::string> const &names) = 0;
  virtual void clearPreviewFile() = 0;
  virtual void clearInputFiles() = 0;
  virtual void setRunIsRunning(const bool running) = 0;
  virtual void setSaveResultEnabled(const bool enabled) = 0;
  virtual int getPreviewSpec() = 0;
  virtual std::string getPreviewWorkspaceName(int index) const = 0;
  virtual std::string getPreviewFilename(int index) const = 0;
  virtual std::string getCurrentPreview() const = 0;
  virtual QStringList getInputFilenames() = 0;

  // controls for dataTable
  virtual void clearDataTable() = 0;
  virtual void addTableEntry(int row, std::string const &name, int spectrum) = 0;

  virtual QModelIndexList getSelectedData() = 0;

  // boolean flags for LoadHistory/GroupInput Checkboxes
  virtual bool isLoadHistory() = 0;
  virtual bool isGroupInput() = 0;

  // getters/setters for m_properties
  virtual bool getNormalise() = 0;
  virtual bool getBackgroundSubtraction() = 0;
  virtual std::string getLogName() = 0;
  virtual std::string getLogValue() = 0;
  virtual void setIntegrationStart(double value) = 0;
  virtual void setIntegrationEnd(double value) = 0;
  virtual void setBackgroundStart(double value) = 0;
  virtual void setBackgroundEnd(double value) = 0;

  virtual double getIntegrationStart() = 0;
  virtual double getIntegrationEnd() = 0;
  virtual double getBackgroundStart() = 0;
  virtual double getBackgroundEnd() = 0;

  virtual void showMessageBox(std::string const &message) const = 0;
};
} // namespace CustomInterfaces
} // namespace MantidQt