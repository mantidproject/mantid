#ifndef MDSETTINGS_H_
#define MDSETTINGS_H_

#include "DllOption.h"
#include "boost/scoped_ptr.hpp"
#include "MantidQtAPI/MdConstants.h"
#include <QColor>
#include <QString>
#include <QStringList>

namespace MantidQt
{
  namespace API
  {
    /**
      *
      This class is for reading and persisting MD properties.

      @date 19/12/2014

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

    class EXPORT_OPT_MANTIDQT_API MdSettings
    {
      public:

        MdSettings();

        ~MdSettings();

        /**
         * Set the UserSetting color map for the vsi.
         *@param colorMap UserSetting colormap for the vsi
         */
        void setUserSettingColorMap(QString colorMap);

        /**
          * Get the UserSetting color map for the vsi.
          * @returns The UserSetting color map for the vsi.
          */
        QString getUserSettingColorMap();

        /**
          * Get the LastSession color map
          */
        QString getLastSessionColorMap();

        /** 
          * Set the LastSession color map
          * @param colormap The colormap for the VSI.
          */
        void setLastSessionColorMap(QString colorMap);

        /**
          * Get the background color for the user setting.
          * @returns The background color.
          */
        QColor getUserSettingBackgroundColor();

        /**
         * Get the default background color.
         * @returns The default background color.
         */
        QColor getDefaultBackgroundColor();

        /**
          * Set the background color for the user setting.
          * @param backgroundColor The background color.
          */
        void setUserSettingBackgroundColor(QColor backgroundColor);

        /**
          * Get the background color for the last session.
          * @returns The background color.
          */
        QColor getLastSessionBackgroundColor();

        /**
          * Set the background color for the user setting.
          * @param backgroundColor The background color.
          */
        void setLastSessionBackgroundColor(QColor backgroundColor);

        /** 
          * Set the general MD color map
          * @param colorMapName The name of the general color map.
          * @param colorMapFile The file name of the general color map.
          */
        void setGeneralMdColorMap(QString colorMapName, QString colorMapFile);

        /**
          * Get the general MD color map file
          * @returns The file path to the general md color map .map file.
          */
        QString getGeneralMdColorMapFile();

        /**
         * Get the general MD color map name
         * @returns The name of the general Md color map.
         */
        QString getGeneralMdColorMapName();

        /**
          *  Set the flag if general color map is desired or not.
          * @param flag If a general color map is desired or not.
          */
        void setUsageGeneralMdColorMap(bool flag);

        /**
          * Get the flag if the general color map is desired or not.
          * @returns Is a general color map desired?
          */
        bool getUsageGeneralMdColorMap();

        /**
          * Set the flag which indicates if the last active color map is supposed to be used.
          * @param flag If the last active color map is supposed to be used or not.
          */
        void setUsageLastSession(bool flag);

        /**
          * Get the flag which indicates if the last active color map is supposed to be used.
          * @returns Is the last active color map to be used?
          */
        bool getUsageLastSession();

        /**
         * Get user setting for the initial view.
         * @returns The initial view
         */
        QString getUserSettingInitialView();

        /**
         * Set the user setting for the initial view.
         * @param initialView The selected initial view.
         */
        void setUserSettingIntialView(QString initialView);

      private:
        boost::scoped_ptr<MdConstants> mdConstants;

        QString vsiGroup;
        QString generalMdGroup;
        QString sliceViewerGroup;

        QString lblUserSettingColorMap;
        QString lblLastSessionColorMap;
        QString lblUseLastSessionColorMap;
        QString lblGeneralMdColorMap;
        QString lblUseGeneralMdColorMap;
        QString lblGeneralMdColorMapName;

        QString lblSliceViewerColorMap;

        QString lblUserSettingBackgroundColor;
        QString lblLastSessionBackgroundColor;

        QString lblUserSettingInitialView;
    };
  }
}

#endif 
