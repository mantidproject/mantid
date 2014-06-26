#ifndef MANTIDQTCUSTOMDIALOGS_SLICINGALGORITHMDIALOG_H_
#define MANTIDQTCUSTOMDIALOGS_SLICINGALGORITHMDIALOG_H_

//----------------------
// Includes
//----------------------
#include "ui_SlicingAlgorithmDialog.h"
#include "MantidQtAPI/AlgorithmDialog.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/Algorithm.h"

namespace MantidQt
{
namespace CustomDialogs
{

typedef QMap<QString, QString> PropertyDimensionMap;

/*
Class SlicingAlgorithmDialog
Abstract SlicingAlgorithm Dialog geared for MD Slicing type algorithms

This custom dialog provides two advantages over the default custom generated one.

1) It dynamically creates dimension input controls based on the nature of the input MD workspace
2) It pre-populates those dimension input controls based on existing values.

*/
class SlicingAlgorithmDialog : public MantidQt::API::AlgorithmDialog
{
  Q_OBJECT
public:

   /// Default Constructor
  SlicingAlgorithmDialog(QWidget *parent = 0);

  /// Destructor
  ~SlicingAlgorithmDialog();

protected:

  /// view
  Ui::SlicingAlgorithmDialog ui; 

  /// Common slice md setup
  void commonSliceMDSetup(const bool);

protected slots:

  void onWorkspaceChanged();

  void onAxisAlignedChanged(bool);

  void onBrowse();

  void onMaxFromInput(bool);

  void onRebuildDimensions();
  
  void onCalculateChanged(bool checked);

private:

  enum History{Remember, Forget};

  enum HistoryChanged{HasChanged, HasNotChanged};

  /// Initialize the layout
  virtual void initLayout();

  /// Determine if axis aligned or non-axis aligned is required.
  bool doAxisAligned() const;

  /// Gets the input workspace name provided
  QString getCurrentInputWorkspaceName() const;

  /// Getter for the historical input workspace name
  QString getHistoricalInputWorkspaceName() const;

  /// Gets the output workspace name provided
  QString getCurrentOutputWorkspaceName() const;

  /// Build dimension inputs.
  void buildDimensionInputs(const bool bForceForget=false);

  /// Build dimension inputs.
  void makeDimensionInputs(const QString& propertyPrefix, QLayout* owningLayout, QString(*format)(Mantid::Geometry::IMDDimension_const_sptr), History history);

  /// Determine if history should be used.
  History useHistory(const HistoryChanged& criticalChange, const bool bForceForget);

  /// Cleans a given layout.
  void cleanLayoutOfDimensions(QLayout* layout);

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
class SliceMDDialog : public SlicingAlgorithmDialog
{
    Q_OBJECT
public:
  SliceMDDialog(QWidget* parent=NULL) : SlicingAlgorithmDialog(parent)
  {
  }
  ~SliceMDDialog(){}

  void customiseInitLayout();
};

/*
Class BinMDDialog
Concrete BinMDDialog Dialog geared for BinMD
*/
class BinMDDialog : public SlicingAlgorithmDialog
{
    Q_OBJECT
public:
  BinMDDialog(QWidget* parent=NULL) : SlicingAlgorithmDialog(parent)
  {
  }
  ~BinMDDialog(){}
  void customiseInitLayout();
};

}
}

#endif
