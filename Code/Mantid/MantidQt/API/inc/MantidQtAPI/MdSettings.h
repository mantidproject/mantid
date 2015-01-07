#ifndef MDSETTINGS_H_
#define MDSETTINGS_H_

#include "DllOption.h"
#include "boost/scoped_ptr.hpp"
#include "MantidQtAPI/MdConstants.h"
#include <string>
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
        void setUserSettingColorMap(std::string colorMap);

        /**
          * Get the UserSetting color map for the vsi.
          * @returns The UserSetting color map for the vsi.
          */
        std::string getUserSettingColorMap();

        /**
          * Get the LastSession color map
          */
        std::string getLastSessionColorMap();

        /** 
          * Set the LastSession color map
          * @param colormap The colormap for the VSI.
          */
        void setLastSessionColorMap(std::string colorMap);

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
          * Set the general Md color map
          * @param colormap The general colormap for VSI and Slice Viewer.
          */
        void setGeneralMdColorMap(std::string colorMap);

        /**
          * Get the general Md color map
          * @returns The general colormap for VSI and Slice Viewer.
          */
        std::string getGeneralMdColorMap();

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
         * Get the list of color maps for the VSI.
         * @returns A list of color maps for the VSI.
         */
        QStringList getVsiColorMaps();

      private:
        boost::scoped_ptr<MdConstants> mdConstants;

        QString vsiGroup;
        QString generalMdGroup;

        QString lblUserSettingColorMap;
        QString lblLastSessionColorMap;
        QString lblUseLastSessionColorMap;
        QString lblGeneralMdColorMap;
        QString lblUseGeneralMdColorMap;
        
        QString lblUserSettingBackgroundColor;
        QString lblLastSessionBackgroundColor;
    };
  }
}

#endif 
