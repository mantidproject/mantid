#ifndef MANTIDQT_CUSTOMINTERFACES_SANSUTILITYDIALOGS_H_
#define MANTIDQT_CUSTOMINTERFACES_SANSUTILITYDIALOGS_H_

//------------------------------
// Includes
//------------------------------
#include "MantidQtAPI/MantidDialog.h"

#include <QDialog>
#include <QStringList>

//-----------------------------
// Qt forward declarations
//----------------------------
class QTreeWidget;
class QComboBox;
class QLineEdit;
class QLabel;

namespace MantidQt
{
namespace CustomInterfaces
{

class SANSPlotDialog : public API::MantidDialog
{
  Q_OBJECT
  
public:
  /// Default constructor
  SANSPlotDialog(QWidget *parent = 0);

  ///Set the list of data sets that are available to plot
  void setAvailableData(const QStringList & workspaces);

signals:
  ///Emits this signal when the code for the plots has been constructed
  void pythonCodeConstructed(const QString&);

private slots:
  /// Add a new 1D plot to the list based on the current options
  void add1DPlot();
  /// Add a new 2D plot to the list
  void add2DPlot();
  /// A combo option is clicked
  void plotOptionClicked(const QString & item_text);
  /// The plot button has been clicked
  void plotButtonClicked();
  /// Respond to delete key
  void deleteKeyPressed();

private:
  /// Create a string containing a python plot command
  QString writePlotCmd(const QString & workspace, const QString & spec_num, bool show_plot);
  /// Check the spectra list
  QString checkSpectraList(const QString & workspace, const QString & speclist);

private:
  /// The tree widget holding the information
  QTreeWidget *m_opt_input;
  /// The available workspaces
  QStringList m_workspaces;
  ///The available data sets
  QComboBox *m_data_sets;
  ///The spectra to plot from the current set
  QLineEdit *m_spec_list;
  ///The plot list
  QComboBox *m_plots;
  ///A label widget
  QLabel *m_info_lbl;
};

}
}

#endif //MANTIDQT_CUSTOMINTERFACES_SANSUTILITYDIALOGS_H_
