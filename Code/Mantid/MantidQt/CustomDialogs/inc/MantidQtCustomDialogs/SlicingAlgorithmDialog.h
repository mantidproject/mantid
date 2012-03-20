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

private:

  /// Initialize the layout
  virtual void initLayout();

  /// Build dimension info from the current input workspace.
  void buildFromCurrentInput();

  /// Clear out any exisiting dimension widgets.
  void clearExistingDimensions();

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