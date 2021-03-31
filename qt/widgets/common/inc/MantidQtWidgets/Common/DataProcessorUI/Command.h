// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <memory>
#include <vector>

#include <QString>

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {
/** @class Command

Command is an interface which defines the functions any data
processor action needs to support.
*/
class Command {
public:
  Command() : m_children(){};
  virtual ~Command(){};

  virtual void execute() = 0;
  virtual QString name() = 0;
  virtual QString icon() = 0;
  virtual QString tooltip() = 0;
  virtual QString whatsthis() = 0;
  virtual QString shortcut() = 0;
  virtual bool hasChildren() final { return !m_children.empty(); };
  virtual void setChildren(std::vector<std::unique_ptr<Command>> children) final { m_children = std::move(children); }
  virtual std::vector<std::unique_ptr<Command>> &getChildren() final { return m_children; }
  virtual bool isSeparator() final { return name().isEmpty() && icon().isEmpty(); }
  virtual bool modifiesSettings() { return true; };
  virtual bool modifiesRunningProcesses() { return false; }

protected:
  std::vector<std::unique_ptr<Command>> m_children;
};

/// Typedef for a shared pointer to \c ReflSearchModel
using Command_uptr = std::unique_ptr<Command>;
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt