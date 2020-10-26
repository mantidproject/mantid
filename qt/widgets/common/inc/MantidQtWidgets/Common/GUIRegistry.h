// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "IGUILauncher.h"
#include "MantidKernel/Instantiator.h"
#include "MantidKernel/SingletonHolder.h"

#include <map>
#include <string>
#include <QString>
#include <QVector>
#include <QMap>
#include <QMenu>

namespace MantidQt {
namespace API {

using namespace Mantid::Kernel;

using LauncherRegistry =
    std::map<std::string, std::map<std::string, IGUILauncher_sptr>>;

/** GUIRegistry : Manages the list of subscribed plugin GUIs.
 */
class EXPORT_OPT_MANTIDQT_COMMON GUIRegistryImpl {
public:
  std::map<std::string, std::vector<std::string>> getNames() const {
    std::map<std::string, std::vector<std::string>> interfaces;
    for (const auto &guis : m_registry) {
      for (const auto &gui : guis.second) {
        interfaces[gui.first].emplace_back(gui.first);
      }
    }
    return interfaces;
  }
  QMenu* getMenu() const {
      QMenu* menu = new QMenu();
      return menu;
  }
  const IGUILauncher &getGUI(const std::string &category,
                             const std::string &name) const {
    if (checkExists(category, name)) {
      return *m_registry.at(category).at(name);
    } else {
      throw std::runtime_error(
          "GUI with the given name does not exist under the given category");
    }
  }

  void subscribe(const IGUILauncher_sptr &gui) {
    const auto category = gui->category().toStdString();
    const auto name = gui->name().toStdString();
    if (checkExists(category, name)) {
      throw std::runtime_error(
          "GUI with the same name is already registered under the same "
          "category.");
    }
    m_registry[category][name] = gui;
  }

  template <class C> void subscribe() {
    std::unique_ptr<AbstractInstantiator<IGUILauncher>> instantiator =
        std::make_unique<Instantiator<C, IGUILauncher>>();
    return this->subscribe(std::move(instantiator));
  }

private:
  template <class T>
  void subscribe(std::unique_ptr<AbstractInstantiator<T>> instantiator) {
    static_assert(std::is_base_of<IGUILauncher, T>::value);
    IGUILauncher_sptr gui = instantiator->createInstance();
    subscribe(gui);
  }

  bool checkExists(const std::string &category, const std::string &name) const {
    if (m_registry.count(category) != 0) {
      return m_registry.at(category).count(name) != 0;
    } else {
      return false;
    }
  }
  LauncherRegistry
      m_registry; // holds the GUI launchers with unique names under given
                  // category and unique category names
};

using GUIRegistry = Mantid::Kernel::SingletonHolder<GUIRegistryImpl>;

#define DECLARE_GUI(classname)                                                 \
  namespace {                                                                  \
  RegistrationHelper register_gui_##classname(                                 \
      ((Mantid::QtAPI::GUIRegistry::Instance().subscribe<classname>()), 0));   \
  }

} // namespace API
} // namespace MantidQt
