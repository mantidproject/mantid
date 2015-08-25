#include "MantidMatrixExtensionRequest.h"
#include "MantidMatrixModel.h"
#include "MantidMatrix.h"

MantidMatrixExtensionRequest::MantidMatrixExtensionRequest() {}

MantidMatrixExtensionRequest::~MantidMatrixExtensionRequest() {}

/**
 * Create a MantidMatrix Tab Extension
 * @param provide the type
 * @returns a MantidMatrixTabeExtension
 */
MantidMatrixTabExtension MantidMatrixExtensionRequest::createMantidMatrixTabExtension(MantidMatrixModel::Type type) {
  MantidMatrixTabExtension extension;

  switch (type) {
    case MantidMatrixModel::DX:
      extension.label = "X Errors";
      extension.type = type;
      return extension;
    default:
      throw std::runtime_error("The requested extension type has not been implemented yet");
  }
}

