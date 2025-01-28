// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidQtWidgets/Common/AlgorithmDialog.h"
#include "ui_FitDialog.h"

#include <QSpinBox>

//------------------------------------------------------------------------------
// Qt Forward declarations
//------------------------------------------------------------------------------
class QVBoxLayout;

namespace MantidQt {
//------------------------------------------------------------------------------
// Mantid Forward declarations
//------------------------------------------------------------------------------
namespace MantidWidgets {
class FileFinderWidget;
}

namespace CustomDialogs {

//------------------------------------------------------------------------------
// Local Forward declarations
//------------------------------------------------------------------------------
class InputWorkspaceWidget;
class DynamicPropertiesWidget;

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
class FitDialog final : public API::AlgorithmDialog {
  Q_OBJECT

public:
  /// Default constructor
  FitDialog(QWidget *parent = nullptr);

private slots:
  /// Override the help button clicked method
  // void helpClicked();
  void workspaceChanged(const QString & /*unused*/);
  void functionChanged();
  /// Create InputWorkspaceWidgets and populate the tabs of the tab widget
  void createInputWorkspaceWidgets();
  void domainTypeChanged();

private:
  /// Initialize the layout
  void initLayout() override;
  /// Save the input history
  void saveInput() override;
  void parseInput() override;
  /// Tie static widgets to their properties
  void tieStaticWidgets(const bool readHistory);
  /// Get the domain type: Simple, Sequential, or Parallel
  int getDomainType() const;
  /// Get the domain type: Simple, Sequential, or Parallel
  QString getDomainTypeString() const;

  /// Get allowed values for a property
  QStringList getAllowedPropertyValues(const QString &propName) const;
  /// Set i-th workspace name
  void setWorkspaceName(int i, const QString &wsName);

  /// Is the function MD?
  bool isMD() const;

private:
  /// Form
  Ui::FitDialog m_form;
  QList<QWidget *> m_tabs;

  friend class InputWorkspaceWidget;
};

/**
 * Widget for inputting workspace information.
 */
class InputWorkspaceWidget : public QWidget {
  Q_OBJECT
public:
  /// Constructor
  InputWorkspaceWidget(FitDialog *parent, int domainIndex = 0);
  /// Return property value stored in history
  QString getStoredPropertyValue(const QString &propName) const { return m_fitDialog->getPreviousValue(propName); }
  /// Get allowed values for a property
  QStringList getAllowedPropertyValues(const QString &propName) const {
    return m_fitDialog->getAllowedPropertyValues(propName);
  }
  /// Get workspace name
  QString getWorkspaceName() const;
  /// Set workspace name
  void setWorkspaceName(const QString &wsName);
  /// Return the domain index
  int getDomainIndex() const { return m_domainIndex; }
  /// Set a property
  void setPropertyValue(const QString &propName, const QString &propValue);
  /// Set all workspace properties
  void setProperties();
  /// Get the domain type: Simple, Sequential, or Parallel
  int getDomainType() const { return m_fitDialog->getDomainType(); }
protected slots:
  /// Set the dynamic properties
  void setDynamicProperties();

protected:
  /// Is ws name set?
  bool isWSNameSet() const;
  /// Is the workspace MW?
  bool isMatrixWorkspace() const;
  /// Is the workspace MD?
  bool isMDWorkspace() const;
  /// is current workspace supported by Fit?
  bool isWorkspaceSupported() const;

  /// Parent FitDialog
  FitDialog *m_fitDialog;
  /// In multidomain fitting it is index of domain created from this workspace
  /// In single domain case == 0
  int m_domainIndex;
  /// Name of the property for the input workspace
  QString m_wsPropName;
  /// Workspace name widget
  QComboBox *m_workspaceName;
  /// Dynamic propeties widget
  DynamicPropertiesWidget *m_dynamicProperties;

  /// The main layout
  QVBoxLayout *m_layout;
};

/**
 * Base class for input workspace's dynamic properties widget
 */
class DynamicPropertiesWidget : public QWidget {
public:
  /// Constructor
  DynamicPropertiesWidget(InputWorkspaceWidget *parent) : QWidget(parent), m_wsWidget(parent) {}
  /// Initialize the child widgets with stored and allowed values
  virtual void init() = 0;
  /// Set all workspace properties
  virtual void setProperties() = 0;

protected:
  /// Parent InputWorkspaceWidget
  InputWorkspaceWidget *m_wsWidget;
};

/**
 * Widgets to set properties for a MatrixWorkspace: WorkspaceIndex, StartX, EndX
 */
class MWPropertiesWidget : public DynamicPropertiesWidget {
public:
  MWPropertiesWidget(InputWorkspaceWidget *parent);
  /// Initialize the child widgets with stored and allowed values
  void init() override;
  /// Set all workspace properties
  void setProperties() override;

private:
  QSpinBox *m_workspaceIndex;
  QLineEdit *m_startX;
  QLineEdit *m_endX;
  QSpinBox *m_maxSize;
};

/**
 * Widgets to set properties for a IMDWorkspace: MaxSize
 */
class MDPropertiesWidget : public DynamicPropertiesWidget {
public:
  MDPropertiesWidget(InputWorkspaceWidget *parent);
  /// Initialize the child widgets with stored and allowed values
  void init() override {}
  /// Set all workspace properties
  void setProperties() override;

private:
  QSpinBox *m_maxSize;
};
} // namespace CustomDialogs
} // namespace MantidQt
