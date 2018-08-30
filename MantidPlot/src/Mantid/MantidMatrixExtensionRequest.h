#ifndef MANTIDPLOT_MANTIDMATRIXEXTENSIONREQUEST_H
#define MANTIDPLOT_MANTIDMATRIXEXTENSIONREQUEST_H

#include "IMantidMatrixExtensionHandler.h"
#include "MantidMatrixModel.h"
#include "MantidMatrixTabExtension.h"
#include <memory>

class MantidMatrixExtensionRequest {
public:
  MantidMatrixExtensionRequest();
  ~MantidMatrixExtensionRequest();
  MantidMatrixTabExtension
  createMantidMatrixTabExtension(MantidMatrixModel::Type type);
  void setNumberFormat(MantidMatrixModel::Type type,
                       MantidMatrixTabExtensionMap &extensions,
                       const QChar &format, int precision);
  void setNumberFormatForAll(MantidMatrixTabExtensionMap &extensions,
                             const QChar &format, int precision);
  void recordFormat(MantidMatrixModel::Type type,
                    MantidMatrixTabExtensionMap &extensions,
                    const QChar &format, int precision);
  QChar getFormat(MantidMatrixModel::Type type,
                  MantidMatrixTabExtensionMap &extensions, QChar defaultValue);
  int getPrecision(MantidMatrixModel::Type type,
                   MantidMatrixTabExtensionMap &extensions, int defaultValue);
  int getColumnWidth(MantidMatrixModel::Type type,
                     MantidMatrixTabExtensionMap &extensions, int defaultValue);
  void setColumnWidthForAll(MantidMatrixTabExtensionMap &m_extensions,
                            int width, int numberOfColumns);
  QTableView *getTableView(MantidMatrixModel::Type type,
                           MantidMatrixTabExtensionMap &extensions, int width,
                           QTableView *defaultValue);
  void setColumnWidthPreference(MantidMatrixModel::Type type,
                                MantidMatrixTabExtensionMap &extensions,
                                int width);
  QTableView *getActiveView(MantidMatrixModel::Type type,
                            MantidMatrixTabExtensionMap &extensions,
                            QTableView *defaultValue);
  MantidMatrixModel *getActiveModel(MantidMatrixModel::Type type,
                                    MantidMatrixTabExtensionMap &extensions,
                                    MantidMatrixModel *defaultValue);
  bool tableViewMatchesObject(MantidMatrixTabExtensionMap &extensions,
                              QObject *object);
  int getColumnWidthPreference(MantidMatrixModel::Type type,
                               MantidMatrixTabExtensionMap &extensions,
                               int defaultValue);

private:
  std::unique_ptr<IMantidMatrixExtensionHandler> m_extensionHandler;
};

#endif