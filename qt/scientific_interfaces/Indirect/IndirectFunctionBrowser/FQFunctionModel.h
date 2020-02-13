// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#ifndef MANTID_FQFUNCTIONMODEL_H
#define MANTID_FQFUNCTIONMODEL_H

#include "DllConfig.h"
#include "IFQFitObserver.h"
#include "IndexTypes.h"
#include "MantidAPI/IFunction_fwd.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidQtWidgets/Common/FunctionModel.h"
#include "ParameterEstimation.h"

#include <QMap>
#include <boost/optional.hpp>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

using namespace Mantid::API;
using namespace MantidWidgets;

class MANTIDQT_INDIRECT_DLL FQFunctionModel : public FunctionModel {
public:
  FQFunctionModel();
  void setFunction(IFunction_sptr fun) override;

  void setFitType(const QString &name);
  QString getFitType();
  void removeFitType();
  void setDataType(DataType dataType);
  QStringList getFunctionList();
  int getEnumIndex();
  void setGlobal(const QString &name, bool isGlobal);

  void
  updateParameterEstimationData(DataForParameterEstimationCollection &&data);

private:
  QString m_fitType;
  DataForParameterEstimationCollection m_estimationData;
  QMap<QString, IFunction_sptr> m_functionStore;
  QMap<QString, QStringList> m_globalParameterStore;
  std::string m_resolutionName;
  TableDatasetIndex m_resolutionIndex;
  DataType m_dataType;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTID_FQFUNCTIONMODEL_H
