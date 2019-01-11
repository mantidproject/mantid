// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_API_GENERICDIALOG_H_
#define MANTIDQT_API_GENERICDIALOG_H_

//----------------------------------
// Includes
//----------------------------------
#include "AlgorithmDialog.h"

#include "MantidKernel/Property.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidQtWidgets/Common/AlgorithmPropertiesWidget.h"
#include <QHash>
#include <QVariant>

//----------------------------------
// Forward declarations
//----------------------------------
class QSignalMapper;
class QGridLayout;
class QLineEdit;

namespace MantidQt {

namespace API {

/**
    This class gives a basic dialog that is not tailored to a particular
    algorithm.

    @date 24/02/2009
*/
class EXPORT_OPT_MANTIDQT_COMMON GenericDialog : public AlgorithmDialog {

  Q_OBJECT

public:
  // Constructor
  GenericDialog(QWidget *parent = nullptr);
  // Destructor
  ~GenericDialog() override;

protected slots:
  void accept() override;

private:
  void initLayout() override;

  void parseInput() override;

  /// Widget containing all the PropertyWidgets
  AlgorithmPropertiesWidget *m_algoPropertiesWidget;
};
} // namespace API
} // namespace MantidQt

#endif // MANTIDQT_API_GENERICDIALOG_H_
