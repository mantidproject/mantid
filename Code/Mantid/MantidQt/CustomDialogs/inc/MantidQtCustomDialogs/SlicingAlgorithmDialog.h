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

class SlicingAlgorithmDialog : public MantidQt::API::AlgorithmDialog
{
  Q_OBJECT
public:

   /// Default Constructor
  SlicingAlgorithmDialog(QWidget *parent = 0);

  /// Destructor
  ~SlicingAlgorithmDialog();

protected:
  /// The algorithm for processing chunks
  Mantid::API::Algorithm_sptr m_slicingAlgorithm;

protected slots:

  void onWorkspaceChanged();

  void onAxisAlignedChanged(bool);

  void onBrowse();

  void onMaxFromInput(bool);

  void accept();

private:

  /// Initialize the layout
  virtual void initLayout();

  /// Get the property documentation
  QString getPropertyDocumentation(const QString& propertyName);

  /// Helper to apply documentation to a widget.
  void applyDocToWidget(QWidget* widget, const QString& propertyName);

  /// Helper to apply same documentation to many widgets.
  void applyDocToWidgets(QWidget* widget1, QWidget* widget2, const QString& propertyName);

  /// Determine if axis aligned or non-axis aligned is required.
  bool doAxisAligned() const;

  /// Gets the input workspace name provided
  QString getInputWorkspaceName() const;

  /// Gets the output workspace name provided
  QString getOutputWorkspaceName() const;

  /// Build dimension inputs.
  void buildDimensionInputs();

  /// Build dimension inputs.
  void buildDimensionInputs(const QString& propertyPrefix, QLayout* owningLayout, QString(*format)(Mantid::Geometry::IMDDimension_const_sptr));

  /// Cleans a given layout.
  void cleanLayoutOfDimensions(QLayout* layout);

  /// Clear out any exisiting dimension widgets.
  void clearExistingDimensions();

  /// Extract the dimension information provided by the user.
  PropertyDimensionMap extractDimensionInputs() const;

  /// view
  Ui::Dialog ui; 
};

class SliceMDDialog : public SlicingAlgorithmDialog
{
    Q_OBJECT
public:
  SliceMDDialog(QWidget* parent=NULL) : SlicingAlgorithmDialog(parent)
  {
  }
};

class BinMDDialog : public SlicingAlgorithmDialog
{
    Q_OBJECT
public:
  BinMDDialog(QWidget* parent=NULL) : SlicingAlgorithmDialog(parent)
  {
  }
};

}
}

#endif