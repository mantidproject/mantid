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
#include <string>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {

/** @class Item

    Item is a generic base class providing common operations and state for rows
    and groups in the runs table
 */
class MANTIDQT_ISISREFLECTOMETRY_DLL Item {
public:
  Item();

  virtual bool isGroup() const = 0;
  State state() const;
  std::string message() const;
  virtual bool requiresProcessing(bool reprocessFailed) const;

  virtual void resetState();
  virtual void setSkipped(bool skipped);
  virtual void renameOutputWorkspace(std::string const &oldName,
                                     std::string const &newName) = 0;

  virtual void setOutputNames(std::vector<std::string> const &outputNames) = 0;
  virtual void resetOutputNames() = 0;

  bool complete() const;
  bool success() const;

  void setProgress(double p, std::string const &msg);
  void setStarting();
  void setRunning();
  void setSuccess();
  void setError(std::string const &msg);

protected:
  ItemState m_itemState;
  bool m_skipped;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_CUSTOMINTERFACE_ITEM_H_
