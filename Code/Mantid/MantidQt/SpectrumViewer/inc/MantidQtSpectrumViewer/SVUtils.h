#ifndef  SV_UTILS_H
#define  SV_UTILS_H

#include <QVector>

#include "MantidQtSpectrumViewer/DllOptionSV.h"

/**
    @class SVUtils

    This class has static methods that do various basic calculations
    needed by other parts of the SpectrumView package.

    @author Dennis Mikkelson
    @date   2012-04-03

    Copyright © 2012 ORNL, STFC Rutherford Appleton Laboratories

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

    Code Documentation is available at
                 <http://doxygen.mantidproject.org>
 */

namespace MantidQt
{
namespace SpectrumView
{

class EXPORT_OPT_MANTIDQT_SPECTRUMVIEWER SVUtils
{
  public:
    /// Get a formatted string representation of a double
    static void Format( int            width,
                        int            precision,
                        double         value,
                        std::string  & str );

    /// push a name, value pair onto a vector of strings
    static void PushNameValue( const std::string               & name,
                                     int                         width,
                                     int                         precision,
                                     double                      value,
                                     std::vector<std::string>  & list );

    /// find a non-degenerate interval containing all the specified values
    static bool FindValidInterval( const QVector<double>  & values,
                                         double           & min,
                                         double           & max );

    /// Adjust min and max so that min is strictly less than max
    static bool FindValidInterval( double           & min,
                                   double           & max );

    /// Adjust min and max so they can be used to form a log scale
    static bool FindValidLogInterval( double  & min, double  & max );

    /// Find the number of steps from min to max on a linear or log scale
    static int NumSteps( double min, double max, double step );

    /// Find point new_val that is spaced between new_min and new_max in the
    /// same proportion as val is between min and max. Return false if
    /// new_val is outside [new_min,new_max].
    static bool Interpolate( double   min,
                             double   max,
                             double   val,
                             double   newMin,
                             double   newMax,
                             double & newVal );

    /// Find the value in [new_min,new_max] on a logarithmic scale that
    /// would correspond to the point val on a linear scale on [min,max].
    static bool LogInterpolate( double   min,
                                double   max,
                                double   val,
                                double   newMin,
                                double   newMax,
                                double & newVal );

    /// adjust the values defining a subinterval to match the boundaries of
    /// the global data. (Currently only for uniformly spaced bins.)
    static bool CalculateInterval( double   globalMin,
                                   double   globalMax,
                                   size_t   globalSteps,
                                   size_t & firstIndex,
                                   double & min,
                                   double & max,
                                   size_t & steps );
};

} // namespace SpectrumView
} // namespace MantidQt

#endif   // SV_UTILS_H
