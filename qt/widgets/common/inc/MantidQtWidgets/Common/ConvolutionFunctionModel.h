// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDWIDGETS_CONVOLUTIONFUNCTIONMODEL_H_
#define MANTIDWIDGETS_CONVOLUTIONFUNCTIONMODEL_H_

#include "DllOption.h"
#include "FunctionModel.h"

#include <boost/optional.hpp>

namespace MantidQt {
namespace MantidWidgets {

using namespace Mantid::API;

class EXPORT_OPT_MANTIDQT_COMMON ConvolutionFunctionModel
    : public FunctionModel {
public:
  void setFunction(IFunction_sptr) override;
  void setModel(const std::string &background, const std::string &workspace,
                int workspaceIndex, const std::string &peaks, bool hasDeltaFunction);
  boost::optional<QString> backgroundPrefix() const {
    return m_backgroundPrefix;
  }
  boost::optional<QString> convolutionPrefix() const {
    return m_convolutionPrefix;
  }
  boost::optional<QString> deltaFunctionPrefix() const {
    return m_deltaFunctionPrefix;
  }
  boost::optional<QStringList> peakPrefixes() const { return m_peakPrefixes; }
  std::string resolutionWorkspace() const { return m_resolutionWorkspace; }
  int resolutionWorkspaceIndex() const { return m_resolutionWorkspaceIndex; }

private:
  void findComponentPrefixes();
  void findConvolutionPrefixes(const IFunction_sptr& fun);
  boost::optional<QString> m_backgroundPrefix;
  boost::optional<QString> m_convolutionPrefix;
  boost::optional<QString> m_deltaFunctionPrefix;
  boost::optional<QStringList> m_peakPrefixes;
  std::string m_resolutionWorkspace;
  int m_resolutionWorkspaceIndex;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif // MANTIDWIDGETS_CONVOLUTIONFUNCTIONMODEL_H_
