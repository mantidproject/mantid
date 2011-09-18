#ifndef MANTID_CURVEFITTING_PLOTPEAKBULOGVALUE_H_
#define MANTID_CURVEFITTING_PLOTPEAKBULOGVALUE_H_
/*WIKI* 


This algorithm fits a series of spectra with the same function. Each spectrum is fit independently and the result is a table of fitting parameters unique for each spectrum. The sources for the spectra are defined in the Input property. The Input property expects a list of spectra identifiers separated by semicolons (;). An identifier is itself a comma-separated list of values. The first value is the name of the source. It can be either a workspace name or a name of a file (RAW or Nexus). If it is a name of a [[WorkspaceGroup]] all its members will be included in the fit. The second value selects a spectrum within the workspace or file. It is an integer number with a prefix defining the meaning of the number: "sp" for a spectrum number, "i" for a workspace index, or "v" for a range of values on the numeric axis associated with the workspace index. For example, sp12, i125, v0.5:2.3. If the data source is a file only the spectrum number option is accepted. The third value of the spectrum identifier is optional period number. It is used if the input file contains multiperiod data. In case of workspaces this third parameter is ignored. This are examples of  Input property

  "test1,i2; MUSR00015189.nxs,sp3; MUSR00015190.nxs,sp3; MUSR00015191.nxs,sp3"
  "test2,v1.1:3.2"
  "test3,v" - fit all spectra in workspace test3

Internally PlotPeakByLogValue uses [[Fit]] algorithm to perform fitting and the following properties have the same meaning as in [[Fit]]: Function, StartX, EndX, Minimizer, CostFunction. Property FitType defines the way of setting initial values. If it is set to "Sequential" every next fit starts with parameters returned by the previous fit. If set to "Individual" each fit starts with the same initial values defined in the Function property. 

LogValue property specifies a log value to be included into the output. If this property is empty the values of axis 1 will be used instead. Setting this property to "SourceName" makes the first column of the output table contain the names of the data sources (files or workspaces).

===Output workspace format===

The output workspace is a table in which rows correspond to the spectra in the order they (spectra) appear in the Input property. The first column of the table has the log values. It is followed by pairs of columns with parameter values and fitting errors. If a parameter was fixed or tied the error will be zero. Here is an example of the output workspace:

[[File:PlotPeakByLogValue_Output.PNG]]

In this example a group of three Matrix workspaces were fitted with a [[Gaussian]] on a linear background.


*WIKI*/

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
          start(data.start),
          end(data.end),
          ws(data.ws)
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
      /// Sets documentation strings for this algorithm
      virtual void initDocs();
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
