#ifndef MANTID_DATAHANDLING_LOADSPEC_H_
#define MANTID_DATAHANDLING_LOADSPEC_H_
/*WIKI* 

The LoadSpec algorithm reads in spectra data from a text file and stores it in a Workspace2D as data points. The data in the file must be organized by set of 3 columns (separated by any number of spaces). The first column has to be the X values, the second column the Y values and the third column the error values. 

Here are two examples of such text files that can be loaded with LoadSpec:

''Example 1:''
<pre>
#F norm: REF_M_2000.nxs
#F data: REF_M_2001.nxs
#E 1234567.80
...
#C SCL Version - 1.4.1

#S 1 Spectrum ID ('bank1',(0,127))
#L Lambda_T(Angstroms) Intensity(Counts/A) Sigma(Counts/A)
0.0   0.0   0.0
1.0   5.0   2.0
2.0   10.0  3.0
3.0   15.0  2.0
4.0   20.0  2.5
5.0   25.0  3.2
6.0   30.0  4.2
</pre>
This will create a Workspace2D with 1 spectrum.

''Example 2:''
<pre>
#F norm: REF_M_2000.nxs
#F data: REF_M_2001.nxs
#E 1234567.80
...
#C SCL Version - 1.4.1

#S 1 Spectrum ID ('bank1',(0,127))
#L Lambda_T(Angstroms) Intensity(Counts/A) Sigma(Counts/A)
0.0   0.0   0.0
1.0   5.0   2.0
2.0   10.0  3.0
3.0   15.0  4.0

#S 1 Spectrum ID ('bank1',(1,127))
#L Lambda_T(Angstroms) Intensity(Counts/A) Sigma(Counts/A)
0.0   10.0   0.0
1.0   15.0   2.0
2.0   110.0  3.0
3.0   115.0  4.0

#S 1 Spectrum ID ('bank1',(3,127))
#L Lambda_T(Angstroms) Intensity(Counts/A) Sigma(Counts/A)
0.0   20.0   0.0
1.0   25.0   2.0
2.0   210.0  3.0
3.0   215.0  4.0
</pre>
This text file will create a Workspace2D with 3 spectra.

*WIKI*/

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
  namespace DataHandling
  {
    /**
    Loads a workspace from an ascii file. Spectra must be stored in columns.

    Properties:
    <ul>
    <li>Filename  - the name of the file to read from.</li>
    <li>Workspace - the workspace name that will be created and hold the loaded data.</li>
    <li>Separator - the column separation character: comma (default),tab,space,colon,semi-colon.</li>
    <li>Unit      - the unit to assign to the X axis (default: Energy).</li>
    </ul>

    @author Roman Tolchenov, Tessella plc
    @date 3/07/09

    Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>. 
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class DLLExport LoadSpec : public API::Algorithm
    {
    public:
      LoadSpec();
      ~LoadSpec() {}
      virtual const std::string name() const { return "LoadSpec"; }
      virtual int version() const { return 1; }
      virtual const std::string category() const { return "DataHandling"; }

    private:
      /// Sets documentation strings for this algorithm
      virtual void initDocs();
      void init();
      void exec();

      /// Allowed values for the cache property
      std::vector<std::string> m_seperator_options;
      std::map<std::string,const char*> m_separatormap; ///<a map of seperators
      typedef std::pair<std::string,const char*> separator_pair; ///<serparator pair type def
    };

  } // namespace DataHandling
} // namespace Mantid

#endif  /*  MANTID_DATAHANDLING_LOADSPEC_H_  */
