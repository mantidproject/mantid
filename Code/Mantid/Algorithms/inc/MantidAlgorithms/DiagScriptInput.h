#ifndef MANTID_ALGORITHM_DIAGSCRIPTINPUT_H_
#define MANTID_ALGORITHM_DIAGSCRIPTINPUT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
  namespace Algorithms
  {
    using namespace API;
/** 
    This algorithm declares and validates the properties used by the
    find bad detectors script, findbaddetectors.py

    Required Properties:
    <UL>
    <LI> WBVanadium1 - Input white beam vanadium workspace name </LI>
    <LI> Variation - The number of times the median value of change between white beam vanadium runs is acceptable </LI>
    </UL>

    Optional Properties:
    <UL>
    <LI> OutputFile - A filename to which to write the list of dead detector UDETs </LI>
    <LI> SignificanceTest - Don't identify spectra as bad if they are within this number of standard deviations from the median </LI>
    <LI> HighAbsolute - Spectra whose total number of counts are above or equal to this value will be marked bad </LI>
    <LI> LowAbsolute - Spectra whose total number of counts are below or equal to this value will be marked bad </LI>
    <LI> HighMedian - Detectors corresponding to spectra with total counts more than this number of the median will be marked bad </LI>
    <LI> LowMedian - Detectors corresponding to spectra with total counts less than this proportion of the median number of counts will be labelled as reading badly </LI>
    <LI> WBVanadium2 - Name of a matching second white beam vanadium run from the same instrument </LI>
    <LI> Experimental - An experimental run that would be used in the background test to find bad detectors </LI>
    <LI> RemoveZero - Identify histograms and their detectors that contain no counts in the background region </LI>
    <LI> MaskExper - Write to the experiment workspace filling all spectra identified with bad detectors with zeros and write to the detector mask map </LI>
    <LI> BackgroundAccept - Accept spectra whose white beam normalised background count is no more than this factor of the median count and identify the others as bad</LI>
    <LI> RangeLower - Marks the start of background region.  It is non-inclusive, the bin that contains this x value wont be used </LI>
    <LI> RangeUpper - Marks the end of background region.  It is non-inclusive, the bin that contains this x value wont be used </LI>
    </UL>

    @author Steve D Williams, ISIS Facility Rutherford Appleton Laboratory
    @date 18/08/2009

    Copyright &copy; 2009 STFC Rutherford Appleton Laboratories

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>    
*/
    class DLLExport DiagScriptInput : public Algorithm
    {
    public:
      /// Default constructor only runs the base class constructor
      DiagScriptInput() : Algorithm()                          //call the base class constructor
      {};
      /// Destructor
      virtual ~DiagScriptInput() {};
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "DiagScriptInput";}
      /// Algorithm's version for identification overriding a virtual method
      virtual const int version() const { return (1);}
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "Diagnostics";}

    protected:
      // Overridden Algorithm methods
      void init();
      void exec();
   };

  } // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_DIAGSCRIPTINPUT_H_*/
