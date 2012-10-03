#ifndef MANTIDQTCUSTOMDIALOGS_LOAD_INSTRUMENT_DIALOG_H_
#define MANTIDQTCUSTOMDIALOGS_LOAD_INSTRUMENT_DIALOG_H_

//----------------------
// Includes
//----------------------
#include "ui_LoadInstrumentDialog.h"
#include "MantidQtAPI/AlgorithmDialog.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/Algorithm.h"

namespace MantidQt
{
namespace CustomDialogs
{

/*
Class SlicingAlgorithmDialog

*/
class LoadInstrumentDialog : public MantidQt::API::AlgorithmDialog
{
  Q_OBJECT
public:

   /// Default Constructor
  LoadInstrumentDialog(QWidget *parent = 0);

  /// Destructor
  ~LoadInstrumentDialog();

protected:

  /// view
  Ui::Dialog ui; 
  
protected slots:

  void onBrowse();
  void accept();

private:
  /// Initialize the layout
  virtual void initLayout();

};
}
}

#endif
