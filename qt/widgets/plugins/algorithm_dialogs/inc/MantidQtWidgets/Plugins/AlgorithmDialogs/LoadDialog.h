// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_CUSTOM_DIALOGS_LOADDIALOG_H
#define MANTIDQT_CUSTOM_DIALOGS_LOADDIALOG_H

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidQtWidgets/Common/AlgorithmDialog.h"
#include "ui_LoadDialog.h"

//------------------------------------------------------------------------------
// Qt Forward declarations
//------------------------------------------------------------------------------
class QVBoxLayout;

namespace MantidQt {
//------------------------------------------------------------------------------
// Mantid Forward declarations
//------------------------------------------------------------------------------
namespace API {
class MWRunFiles;
}

namespace CustomDialogs {

/**
  This class gives specialised dialog for the Load algorithm. It requires that
  the specific
  load algorithm has at least 2 properties with these names:

  <UL>
  <LI>Filename - A text property containing the filename </LI>
  <LI>OutputWorkspace - A text property containing the name of the
  OutputWorkspace </LI>
  </UL>

  There is no UI form as the most of the thing is dynamic.

  @author Martyn Gigg, Tessella plc
  @date 31/01/2011
*/
class PreventLoadRequests;

class LoadDialog : public API::AlgorithmDialog {
  Q_OBJECT
  friend PreventLoadRequests;

public:
  /// Default constructor
  LoadDialog(QWidget *parent = nullptr);

private slots:
  /// Create the widgets and layouts that are dynamic, i.e they depend on
  /// the specific load algorithm
  void createDynamicWidgets();
  /// Override the help button clicked method
  void helpClicked() override;
  /// Suggest a workspace name from the file
  void suggestWSName();
  /// Connect/Disconnect the signal that updates the workspace name with a
  /// suggested value
  void enableNameSuggestion(const bool on = false);
  /// Override accept() slot
  void accept() override;

private:
  /// Initialize the layout
  void initLayout() override;
  /// Save the input history
  void saveInput() override;
  /// Tie static widgets to their properties
  void tieStaticWidgets(const bool readHistory);
  /// Clears all of the widgets from the old layout
  void removeOldInputWidgets(QVBoxLayout *layout);
  /// Create
  void createDynamicLayout();
  /// Create the widgets for a given property
  int createWidgetsForProperty(const Mantid::Kernel::Property *prop,
                               QVBoxLayout *propertyLayout, QWidget *parent);
  /// Ignore requests to load until they are re-enabled.
  void disableLoadRequests();
  /// Accept requests to load until they are disabled.
  void enableLoadRequests();

private:
  /// Form
  Ui::LoadDialog m_form;
  /// The current file
  QString m_currentFiles;
  /// The initial height
  int m_initialHeight;
  /// Flag to indicating if we are populating the dialog
  bool m_populating;
};
} // namespace CustomDialogs
} // namespace MantidQt

#endif // MANTIDQT_CUSTOM_DIALOGS_LOADDIALOG_H
