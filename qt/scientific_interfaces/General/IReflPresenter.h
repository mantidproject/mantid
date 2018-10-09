// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_IREFLPRESENTER_H
#define MANTID_CUSTOMINTERFACES_IREFLPRESENTER_H

#include <map>
#include <string>

#include "MantidKernel/System.h"

#include <QVariant>

namespace MantidQt {
namespace CustomInterfaces {
/** @class IReflPresenter

IReflPresenter is an interface which defines the functions any reflectometry
interface presenter needs to support.
*/
class IReflPresenter {
public:
  virtual ~IReflPresenter(){};

  enum Flag {
    SaveFlag,
    SaveAsFlag,
    AppendRowFlag,
    PrependRowFlag,
    DeleteRowFlag,
    ProcessFlag,
    ProcessAllFlag,
    GroupRowsFlag,
    OpenTableFlag,
    NewTableFlag,
    TableUpdatedFlag,
    ExpandSelectionFlag,
    OptionsDialogFlag,
    ClearSelectedFlag,
    CopySelectedFlag,
    CutSelectedFlag,
    PasteSelectedFlag,
    SearchFlag,
    TransferFlag,
    ImportTableFlag,
    ExportTableFlag,
    PlotRowFlag,
    PlotGroupFlag,
    ExpandAllGroupsFlag,
    CollapseAllGroupsFlag,
    PauseFlag
  };

  // Tell the presenter something happened
  virtual void notify(IReflPresenter::Flag flag) = 0;
  virtual const std::map<std::string, QVariant> &options() const = 0;
  virtual void setOptions(const std::map<std::string, QVariant> &options) = 0;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif
