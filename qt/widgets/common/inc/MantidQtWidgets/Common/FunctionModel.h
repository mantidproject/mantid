// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDWIDGETS_FUNCTIONMODEL_H_
#define MANTIDWIDGETS_FUNCTIONMODEL_H_

#include "DllOption.h"

#include "MantidAPI/IFunction_fwd.h"

#include <QString>

namespace MantidQt {
namespace MantidWidgets {

class EXPORT_OPT_MANTIDQT_COMMON FunctionModel {
public:
  explicit FunctionModel(const QString &funStr = "", int nDomains = 0);
  bool isMultiDomain() const;
  int getNumberDomains() const;
  int currentDomainIndex() const;
  void setCurrentDomainIndex(int);
  Mantid::API::IFunction_sptr getSingleFunction(int index) const;
  Mantid::API::IFunction_sptr getCurrentFunction() const;
  Mantid::API::IFunction_sptr getGlobalFunction() const;
private:
  void checkIndex(int) const;
  Mantid::API::CompositeFunction_sptr m_function;
  size_t m_currentDomainIndex;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif // MANTIDWIDGETS_FUNCTIONMODEL_H_
