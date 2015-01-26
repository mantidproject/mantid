#ifndef REBINMANAGER_H_
#define REBINMANAGER_H_

#include "MantidVatesSimpleGuiViewWidgets/WidgetDllOption.h"
#include "MantidVatesAPI/ADSWorkspaceProvider.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/Algorithm.h"
#include "MantidQtAPI/AlgorithmDialog.h"
#include "MantidQtMantidWidgets/SlicingAlgorithmDialog.h"

#include <pqPipelineSource.h>
#include <QWidget>


namespace Mantid
{
  namespace Vates
  {
    namespace SimpleGui
    {
      /**
       *
       This class coordinates the rebinning of a workspace and updates the pipeline and view to make the changes of the 
       underlying workspace visible.

       @date 15/01/2015

       Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
      class EXPORT_OPT_MANTIDVATES_SIMPLEGUI_VIEWWIDGETS RebinManager : public QWidget
      {
        Q_OBJECT
        public:
          RebinManager(QWidget* parent = 0);

          ~RebinManager();

          void showDialog(std::string inputWorkspace, std::string outputWorkspace);

        private:
          MantidQt::API::AlgorithmDialog* createDialog(Mantid::API::IAlgorithm_sptr algorithm, std::string inputWorkspace, std::string outputWorkspace);

          void getPresetsForBinMD( std::string inputWorkspace, std::string outputWorkspace, QHash<QString, QString>& presets);

          void setAxisDimensions(MantidQt::MantidWidgets::BinMDDialog* dialog,  std::string inputWorkspace);

          Mantid::API::IMDEventWorkspace_sptr getWorkspace(std::string workspaceName);

          Mantid::API::IAlgorithm_sptr createAlgorithm(const QString& algName, int version);

          Mantid::VATES::ADSWorkspaceProvider<Mantid::API::IMDEventWorkspace> m_adsWorkspaceProvider;

          int m_binMdVersion;
          QString m_binMdName;
          QString m_lblInputWorkspace;
          QString m_lblOutputWorkspace;
          size_t m_binCutOffValue;
      };

    } // SimpleGui
  } // Vates
} // Mantid

#endif 
