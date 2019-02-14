// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#ifndef MANTID_CUSTOMINTERFACES_ITEM_H_
#define MANTID_CUSTOMINTERFACES_ITEM_H_
#include "Common/DllConfig.h"
#include "ItemState.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_ISISREFLECTOMETRY_DLL Item {
public:
  Item();
  Item(Item const &rhs);
  Item &operator=(Item const &rhs);

  State state() const;
  std::string message() const;
  bool requiresProcessing(bool reprocessFailed) const;

  virtual void resetState();
  virtual void renameOutputWorkspace(std::string const &oldName,
                                     std::string const &newName) = 0;

  virtual void algorithmStarted(Mantid::API::IAlgorithm_sptr const algorithm);
  virtual void algorithmComplete(Mantid::API::IAlgorithm_sptr const algorithm);
  virtual void algorithmError(Mantid::API::IAlgorithm_sptr const algorithm,
                              std::string const &msg);

protected:
  ItemState m_itemState;

  void setProgress(double p, std::string const &msg);
  void setStarting();
  void setRunning();
  void setSuccess();
  void setError(std::string const &msg);
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_CUSTOMINTERFACE_ITEM_H_
