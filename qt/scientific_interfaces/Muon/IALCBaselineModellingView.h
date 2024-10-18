// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/System.h"

#include "DllConfig.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace.h"

#include <QObject>

namespace MantidQt {
namespace CustomInterfaces {

class IALCBaselineModellingPresenter;

/** IALCBaselineModellingView : Interface for ALC Baseline Modelling view step
 */
class MANTIDQT_MUONINTERFACE_DLL IALCBaselineModellingView : public QObject {
  Q_OBJECT

public:
  using SectionRow = std::pair<QString, QString>;
  using SectionSelector = std::pair<double, double>;

  virtual void subscribePresenter(IALCBaselineModellingPresenter *presenter) = 0;

  /// Function chosen to fit the data to
  /// @return Function string, or empty string if nothing chosen
  virtual std::string function() const = 0;

  /**
   * @param row :: Row number
   * @return Row values from the section table
   */
  virtual SectionRow sectionRow(int row) const = 0;

  /**
   * @param index :: Section selector index
   * @return Section selector values
   */
  virtual SectionSelector sectionSelector(int index) const = 0;

  /**
   * @return Number of rows in section table
   */
  virtual int noOfSectionRows() const = 0;

  virtual void removePlot(QString const &plotName) = 0;

private:
  virtual void initConnections() const = 0;

public slots:
  /// Performs any necessary initialization
  virtual void initialize() = 0;

  /**
   * Update displayed data curve
   * @param workspace :: The workspace containing the data to plot
   * @param workspaceIndex :: The index of the data to plot
   */
  virtual void setDataCurve(Mantid::API::MatrixWorkspace_sptr workspace, std::size_t const &workspaceIndex = 0) = 0;

  /**
   * Update displayed corrected data curve
   * @param workspace :: The workspace containing the data to plot
   * @param workspaceIndex :: The index of the data to plot
   */
  virtual void setCorrectedCurve(Mantid::API::MatrixWorkspace_sptr workspace,
                                 std::size_t const &workspaceIndex = 0) = 0;

  /**
   * Update displayed baseline curve
   * @param workspace :: The workspace containing the data to plot
   * @param workspaceIndex :: The index of the data to plot
   */
  virtual void setBaselineCurve(Mantid::API::MatrixWorkspace_sptr workspace, std::size_t const &workspaceIndex = 0) = 0;

  /**
   * Update displayed function
   * @param func :: New function
   */
  virtual void setFunction(Mantid::API::IFunction_const_sptr func) = 0;

  /**
   * Resize sections table
   * @param rows :: New number of rows
   */
  virtual void setNoOfSectionRows(int rows) = 0;

  /**
   * Updates the row values in the table
   * @param row :: Row in sections table
   * @param values :: New row values
   */
  virtual void setSectionRow(int row, SectionRow values) = 0;

  /**
   * Adds a new section selector
   * @param index :: Index of added section selector, to find it later
   * @param values :: Initial values
   */
  virtual void addSectionSelector(int index, SectionSelector values) = 0;

  /**
   * Deletes section selector at specified index
   * @param index :: Section selector index
   */
  virtual void deleteSectionSelector(int index) = 0;

  /**
   * Update section selector values
   * @param index :: Index of the selector to update
   * @param values :: New values
   */
  virtual void updateSectionSelector(int index, SectionSelector values) = 0;

  /**
   * Pops-up an error box
   * @param message :: Error message to display
   */
  virtual void displayError(const QString &message) = 0;

  /// Links help button to wiki page
  virtual void help() = 0;

  virtual void handleFitRequested() const = 0;

  virtual void handleAddSectionRequested() const = 0;

  virtual void handleRemoveSectionRequested(int row) const = 0;

  virtual void handleSectionRowModified(int row) const = 0;

  virtual void handleSectionSelectorModified(int index) const = 0;

signals:
  /// Fit requested
  void fitRequested();

  /// New section addition requested
  void addSectionRequested();

  /**
   * Section removal requested
   * @param row :: Section row to remove
   */
  void removeSectionRequested(int row);

  /**
   * One of the section rows in the table was modified
   * @param row :: Modified section row
   */
  void sectionRowModified(int row);

  /**
   * One of section selectors has been modified
   * @param index :: Index of modified selector
   */
  void sectionSelectorModified(int index);
};

} // namespace CustomInterfaces
} // namespace MantidQt
