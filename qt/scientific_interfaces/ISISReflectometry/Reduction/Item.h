// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "Common/DllConfig.h"
#include "ItemState.h"
#include <string>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

/** @class Item

    Item is a generic base class providing common operations and state for rows
    and groups in the runs table
 */
class MANTIDQT_ISISREFLECTOMETRY_DLL Item {
public:
  using ItemCountFunction = int (Item::*)() const;

  Item();
  virtual ~Item() = default;

  virtual bool isGroup() const = 0;
  virtual bool isPreview() const = 0;

  State state() const;
  void setState(State state);
  std::string message() const;
  virtual bool requiresProcessing(bool reprocessFailed) const;

  virtual void resetState(bool resetChildren = true);
  virtual void setSkipped(bool skipped);
  virtual void renameOutputWorkspace(std::string const &oldName, std::string const &newName) = 0;

  virtual void setOutputNames(std::vector<std::string> const &outputNames) = 0;
  virtual void resetOutputs(){};

  virtual int totalItems() const = 0;
  virtual int completedItems() const = 0;

  virtual void setStarting();
  virtual void setRunning();
  virtual void setSuccess();
  virtual void setError(std::string const &msg);

  bool complete() const;
  bool success() const;

  void setProgress(double p, std::string const &msg);

protected:
  ItemState m_itemState;
  bool m_skipped;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
