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
  MantidMatrixTabExtension(QString label, QTableView * tableView, MantidMatrixModel *model) : label(label),
                                                                                              tableView(tableView),
                                                                                              model(model) {}
  MantidMatrixTabExtension(): label(""),
                              tableView(NULL),
                              model(NULL) {}
  QString label;
  QTableView *tableView;
  QPointer<MantidMatrixModel> model;
  MantidMatrixModel::Type type;
};

typedef std::map<MantidMatrixModel::Type, MantidMatrixTabExtension> MantidMatrixTabExtensionMap;

#endif
