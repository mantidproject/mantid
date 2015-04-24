#ifndef REBINALGORITHMDIALOGPROVIDER_H_
#define REBINALGORITHMDIALOGPROVIDER_H_

#include "MantidVatesSimpleGuiViewWidgets/WidgetDllOption.h"

#include "MantidVatesAPI/ADSWorkspaceProvider.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/Algorithm.h"
#include "MantidQtAPI/AlgorithmDialog.h"
#include "MantidQtMantidWidgets/SlicingAlgorithmDialog.h"

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
      class EXPORT_OPT_MANTIDVATES_SIMPLEGUI_VIEWWIDGETS RebinAlgorithmDialogProvider
      {
        public:
          RebinAlgorithmDialogProvider(QWidget* parent);

          ~RebinAlgorithmDialogProvider();

          void showDialog(std::string inputWorkspace, std::string outputWorkspace, std::string algorithmType);

           static const size_t BinCutOffValue;

        private:
          MantidQt::API::AlgorithmDialog* createDialog(Mantid::API::IAlgorithm_sptr algorithm, const std::string& inputWorkspace, const std::string& outputWorkspace, const std::string& algorithmType);

          void setAxisDimensions(MantidQt::MantidWidgets::SlicingAlgorithmDialog* dialog,  std::string inputWorkspace);

          Mantid::API::IMDEventWorkspace_sptr getWorkspace(const std::string& workspaceName);

          Mantid::API::IAlgorithm_sptr createAlgorithm(const std::string& algName, int version);

          Mantid::VATES::ADSWorkspaceProvider<Mantid::API::IMDEventWorkspace> m_adsWorkspaceProvider;

          const QString m_lblInputWorkspace;
          const QString m_lblOutputWorkspace;
          QWidget* m_parent;
      };

    } // SimpleGui
  } // Vates
} // Mantid

#endif 
