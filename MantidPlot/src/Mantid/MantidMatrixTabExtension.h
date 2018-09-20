#ifndef MANTIDPLOT_MANTIDMATRIXTABEXTENSION_H
#define MANTIDPLOT_MANTIDMATRIXTABEXTENSION_H

#include "MantidMatrixModel.h"

#include <QPointer>
#include <QString>
#include <QTableView>
/**
 * Holds the information for a new tab.
 */
struct MantidMatrixTabExtension {
  MantidMatrixTabExtension(QString label, QTableView *tableView,
                           MantidMatrixModel *model,
                           MantidMatrixModel::Type type)
      : label(label), tableView(tableView), model(model), type(type) {}
  MantidMatrixTabExtension()
      : label(""), tableView(), model(nullptr),
        type(MantidMatrixModel::Type::DX) {}
  QString label;
  std::unique_ptr<QTableView> tableView;
  QPointer<MantidMatrixModel> model;
  MantidMatrixModel::Type type;
};

using MantidMatrixTabExtensionMap =
    std::map<MantidMatrixModel::Type, MantidMatrixTabExtension>;
#endif
