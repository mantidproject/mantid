#ifndef MANTIDPLOT_MANTIDMATRIXEXTENSIONREQUEST_H
#define MANTIDPLOT_MANTIDMATRIXEXTENSIONREQUEST_H

#include "MantidMatrixModel.h"

class MantidMatrixExtensionRequest {
public:
  MantidMatrixExtensionRequest();
  ~MantidMatrixExtensionRequest();
  MantidMatrixTabExtension createMantidMatrixTabExtension(MantidMatrixModel::Type type);
};

#endif