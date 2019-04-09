// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMDIALOGS_SLICINGALGORITHMDIALOG_H_
#define MANTIDQTCUSTOMDIALOGS_SLICINGALGORITHMDIALOG_H_

//----------------------
// Includes
//----------------------
#include "DllOption.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidQtWidgets/Common/AlgorithmDialog.h"
#include "ui_SlicingAlgorithmDialog.h"

namespace MantidQt {
namespace MantidWidgets {

using PropertyDimensionMap = QMap<QString, QString>;

/*
Class SlicingAlgorithmDialog
Abstract SlicingAlgorithm Dialog geared for MD Slicing type algorithms

This custom dialog provides two advantages over the default custom generated
one.

1) It dynamically creates dimension input controls based on the nature of the
input MD workspace
2) It pre-populates those dimension input controls based on existing values.

*/
class EXPORT_OPT_MANTIDQT_COMMON SlicingAlgorithmDialog
    : public MantidQt::API::AlgorithmDialog {
  Q_OBJECT
public:
  /// Default Constructor
  SlicingAlgorithmDialog(QWidget *parent = nullptr);

  /// Destructor
  ~SlicingAlgorithmDialog() override;

  // Customisation for the VSI
  void customiseLayoutForVsi(std::string initialWorkspace);

  /// Reset the aligned dim values for the VSI
  void resestAlignedDimProperty(size_t index, QString propertyValue);

protected:
  /// view
  Ui::SlicingAlgorithmDialog ui;

  /// Common slice md setup
  void commonSliceMDSetup(const bool /*isSliceMD*/);

  /// Build dimension inputs.
  void buildDimensionInputs(const bool bForceForget = false);

protected slots:

  void onWorkspaceChanged();

  void onAxisAlignedChanged(bool /*unused*/);

  void onBrowse();

  void onMaxFromInput(bool /*unused*/);

  void onRebuildDimensions();

  void onCalculateChanged(bool checked);

private:
  enum History { Remember, Forget };

  enum HistoryChanged { HasChanged, HasNotChanged };

  /// Initialize the layout
  void initLayout() override;

  /// Determine if axis aligned or non-axis aligned is required.
  bool doAxisAligned() const;

  /// Gets the input workspace name provided
  QString getCurrentInputWorkspaceName() const;

  /// Getter for the historical input workspace name
  QString getHistoricalInputWorkspaceName() const;

  /// Gets the output workspace name provided
  QString getCurrentOutputWorkspaceName() const;

  /// Build dimension inputs.
  void makeDimensionInputs(
      const QString &propertyPrefix, QLayout *owningLayout,
      QString (*format)(Mantid::Geometry::IMDDimension_const_sptr),
      History history);

  /// Determine if history should be used.
  History useHistory(const HistoryChanged &criticalChange,
                     const bool bForceForget);

  /// Cleans a given layout.
  void cleanLayoutOfDimensions(QLayout *layout);

  /// Clear out any exisiting dimension widgets.
  void clearExistingDimensions();

  /// Determine if the dimension history has changed.
  HistoryChanged hasDimensionHistoryChanged() const;

  /// Give base classes the opportunity to do any custom overriding.
  virtual void customiseInitLayout() = 0;

  /// Load settings
  void loadSettings();

  /// Save settings
  void saveSettings();

  /// Do auto fill dimension inputs on changes.
  bool doAutoFillDimensions() const;
};

/*
Class SliceMDDialog
Concrete SlicingAlgorithm Dialog geared for SliceMD
*/
class EXPORT_OPT_MANTIDQT_COMMON SliceMDDialog : public SlicingAlgorithmDialog {
  Q_OBJECT
public:
  SliceMDDialog(QWidget *parent = nullptr) : SlicingAlgorithmDialog(parent) {}

  ~SliceMDDialog() override {}

  void customiseInitLayout() override;
};

/*
Class BinMDDialog
Concrete BinMDDialog Dialog geared for BinMD
*/
class EXPORT_OPT_MANTIDQT_COMMON BinMDDialog : public SlicingAlgorithmDialog {
  Q_OBJECT
public:
  BinMDDialog(QWidget *parent = nullptr) : SlicingAlgorithmDialog(parent) {}
  ~BinMDDialog() override {}
  void customiseInitLayout() override;
};
} // namespace MantidWidgets
} // namespace MantidQt

#endif
