#ifndef MDCONSTANTS_H_
#define MDCONSTANTS_H_

#include "DllOption.h"
#include <QString>
#include <QColor>
#include <QStringList>

namespace MantidQt
{
  namespace API
  {
    /**
      *
      This class is a collection of constants and keys used for the VSI.

      @date 6/1/2015

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

    class EXPORT_OPT_MANTIDQT_API MdConstants
    {
      public:

        MdConstants();

        ~MdConstants();

        /**
         * Initialize constants which are required to store and persist MD settings.
         */
        void initializeSettingsConstants();

        /**
         * Initialize constants which are required for the view
         */
        void initializeViewConstants();

        QString getGeneralMdColorMap() const;

        QColor getDefaultBackgroundColor() const;

        QStringList getVsiColorMaps() const;

        QString getStandardView() const;

        QString getMultiSliceView() const;

        QString getThreeSliceView() const;

        QString getSplatterPlotView() const;

        QString getTechniqueDependence() const;
        
        double getColorScaleStandardMax();

        QStringList getAllInitialViews() const;
        
        double getLogScaleDefaultValue();

      private:
        QString m_generalMdColorMap;
        QColor m_defaultBackgroundColor;
        QStringList m_vsiColorMaps;
        QString m_standardView;
        QString m_multiSliceView;
        QString m_threeSliceView;
        QString m_splatterPlotView;
        QString m_techniqueDependence;

        const double m_colorScaleStandardMax;
        const double m_logScaleDefaultValue;
    };
  }
}

#endif 
