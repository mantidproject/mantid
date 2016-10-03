#ifndef MANTIDPLOT_MANTIDMATRIXTABEXTENSION_H
#define MANTIDPLOT_MANTIDMATRIXTABEXTENSION_H

#include "MantidMatrixModel.h"

#include <QString>
#include <QTableView>
#include <QPointer>
/**
 * Holds the information for a new tab.
 */
struct MantidMatrixTabExtension {
  MantidMatrixTabExtension(QString label, QTableView *tableView,
                           MantidMatrixModel *model,
                           MantidMatrixModel::Type type)
      : label(label), tableView(tableView), model(model), type(type) {}
  MantidMatrixTabExtension()
      : label(""), tableView(NULL), model(NULL),
        type(MantidMatrixModel::Type::DX) {}
  QString label;
  QTableView *tableView;
  QPointer<MantidMatrixModel> model;
  MantidMatrixModel::Type type;
};

typedef std::map<MantidMatrixModel::Type, MantidMatrixTabExtension>
    MantidMatrixTabExtensionMap;

#endif
