#ifndef MANTID_CURVEFITTING_PLOTPEAKBULOGVALUE_H_
#define MANTID_CURVEFITTING_PLOTPEAKBULOGVALUE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
  namespace CurveFitting
  {
    /**

    Takes a workspace group and fits the same spectrum in all workspaces with 
    the same function. The output parameters are saved in a workspace ready
    for plotting against the specified log value.

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the WorkspaceGroup to take as input </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the result </LI>
    <LI> WorkspaceIndex - The index of the spectrum to fit.</LI>
    <LI> Optimization - A list property defining how to set initial guesses for 
          the parameters. Value 'Individual' ... </LI>
    <LI> Function - The fitting function. </LI>
    <LI> LogValue - The log value to plot against. </LI>
    <LI> Parameters - A list (comma separated) of parameters for plotting. </LI>
    </UL>


    @author Roman Tolchenov, Tessella plc
    @date 01/06/2010

    Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    class DLLExport PlotPeakByLogValue : public API::Algorithm
    {
      /** Structure to identify data for fitting
        */
      struct InputData
      {
        /// Constructor
        InputData(const std::string& nam,int ix,int s,int p,double st=0,double en=0)
          :name(nam),
          i(ix),
          spec(s),
          period(p),
          start(st),
          end(en)
        {}
        /// Copy constructor
        InputData(const InputData& data)
          :name(data.name),
          i(data.i),
          spec(data.spec),
          period(data.period),
          ws(data.ws),
          start(data.start),
          end(data.end)
        {
          indx.assign(data.indx.begin(),data.indx.end());
        }
        std::string name; ///< Name of a workspace or file
        int i;            ///< Workspace index of the spectra to fit
        int spec;         ///< Spectrum number to fit
        int period;       ///< Period, needed if a file contains several periods
        double start;     ///< starting axis value
        double end;       ///< ending axis value
        API::MatrixWorkspace_sptr ws; ///< shared pointer to the workspace
        std::vector<int> indx;  ///< a list of ws indices to fit if i and spec < 0
      };
    public:
      /// Default constructor
      PlotPeakByLogValue() : API::Algorithm() {};
      /// Destructor
      virtual ~PlotPeakByLogValue() {};
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "PlotPeakByLogValue";}
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 1;}
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "General";}

    private:
      // Overridden Algorithm methods
      void init();
      void exec();

      /// Get a workspace
      InputData getWorkspace(const InputData& data);
      /// Create a list of input workspace names
      std::vector<InputData> makeNames()const;
    };

  } // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_PLOTPEAKBULOGVALUE_H_*/
