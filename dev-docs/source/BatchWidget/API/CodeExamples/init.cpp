// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/Batch/JobTreeView.h"

using namespace MantidWidgets::Common::Batch;

// Inside the parent view constructor
m_treeView = new JobTreeView(
    {"Heading 1", "Heading 2"}, // The table column headings.
    Cell(""), // The default style and content for the new 'empty' cells.
    this      // The parent QObject
    );
m_treeViewSignals = // JobTreeViewSignalAdapter is also available from C++
    // Constructing a signal adapter with the view implicitly calls subscribe.
    new JobTreeViewSignalAdapter(*m_treeView, this);
m_treeView->appendChildRowOf(RowLocation(), {Cell("Value for Column A"),
                                             Cell("Value for Column B")})
