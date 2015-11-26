#ifndef MANTIDQT_CUSTOMINTERFACES_ALCDATALOADINGPRESENTER_H_
#define MANTIDQT_CUSTOMINTERFACES_ALCDATALOADINGPRESENTER_H_

#include "MantidKernel/System.h"

#include "MantidQtCustomInterfaces/DllConfig.h"
#include "MantidQtCustomInterfaces/Muon/IALCDataLoadingView.h"

#include <QObject>
#include <QFileSystemWatcher>

namespace MantidQt
{
namespace CustomInterfaces
{

  /** ALCDataLoadingPresenter : Presenter for ALC Data Loading step
    
    Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class MANTIDQT_CUSTOMINTERFACES_DLL ALCDataLoadingPresenter : public QObject
  {
    Q_OBJECT

  public:
    ALCDataLoadingPresenter(IALCDataLoadingView* view);

    void initialize();

    /// @return Last loaded data workspace
    MatrixWorkspace_const_sptr loadedData() const { return m_loadedData; }

    /// @return Loaded data as MatrixWorkspace_sptr
    MatrixWorkspace_sptr exportWorkspace();

    /// Sets some data
    void setData (MatrixWorkspace_const_sptr data);

  private slots:
    /// Check file range and call method to load new data
    void handleLoadRequested();

    /// Updates the list of logs and number of periods
    void updateAvailableInfo();

    /// When directory contents change, updates files loaded
    void updateFilesFromDirectory(const QString &path);

  signals:
    /// Signal emitted when data get changed
    void dataChanged();

  private:
    /// Load new data and update the view accordingly
    void load(const std::string &lastFile);

    /// View which the object works with
    IALCDataLoadingView* const m_view;

    /// Last loaded data workspace
    MatrixWorkspace_const_sptr m_loadedData;

    /// Watch a directory for changes
    QFileSystemWatcher m_watcher;
  };


} // namespace CustomInterfaces
} // namespace MantidQt

#endif  /* MANTIDQT_CUSTOMINTERFACES_ALCDATALOADINGPRESENTER_H_ */
