#ifndef MANTID_DATAHANDLING_GROUPDETECTORS_H_
#define MANTID_DATAHANDLING_GROUPDETECTORS_H_
/*WIKI* 


This algorithm sums, bin-by-bin, multiple spectra into a single spectra. The errors are summed in quadrature and the algorithm checks that the bin boundaries in X are the same. The new summed spectra are created at the start of the output workspace and have spectra index numbers that start at zero and increase in the order the groups are specified. Each new group takes the spectra numbers from the first input spectrum specified for that group. All detectors from the grouped spectra will be moved (in the [[SpectraDetectorMap]]) to belong to the new spectrum.

Not all spectra in the input workspace have to be copied to a group. If KeepUngroupedSpectra is set to true any spectra not listed will be copied to the output workspace after the groups in order. If KeepUngroupedSpectra is set to false only the spectra selected to be in a group will be used.

To create a single group the list of spectra can be identified using a list of either spectrum numbers, detector IDs or workspace indices. The list should be set against the appropriate property.

An input file allows the specification of many groups. The file must have the following format* (extra space and comments starting with # are allowed) :

 "unused number1"             
 "unused number2"
 "number_of_input_spectra1"
 "input spec1" "input spec2" "input spec3" "input spec4"
 "input spec5 input spec6"
 **    
 "unused number2" 
 "number_of_input_spectra2"
 "input spec1" "input spec2" "input spec3" "input spec4"

<nowiki>*</nowiki> each phrase in "" is replaced by a single integer

<nowiki>**</nowiki> the section of the file that follows is repeated once for each group

Some programs require that "unused number1" is the number of groups specified in the file but Mantid ignores that number and all groups contained in the file are read regardless. "unused number2" is in other implementations the group's spectrum number but in this algorithm it is is ignored and can be any integer (not necessarily the same integer)

 An example of an input file follows:
 2  
 1  
 64  
 1 2 3 4 5 6 7 8 9 10  
 11 12 13 14 15 16 17 18 19 20  
 21 22 23 24 25 26 27 28 29 30  
 31 32 33 34 35 36 37 38 39 40  
 41 42 43 44 45 46 47 48 49 50  
 51 52 53 54 55 56 57 58 59 60  
 61 62 63 64  
 2  
 60
 65 66 67 68 69 70 71 72 73 74  
 75 76 77 78 79 80 81 82 83 84  
 85 86 87 88 89 90 91 92 93 94  
 95 96 97 98 99 100 101 102 103 104  
 105 106 107 108 109 110 111 112 113 114  
 115 116 117 118 119 120 121 122 123 124

In addition the following XML grouping format is also supported
<div style="border:1pt dashed black; background:#f9f9f9;padding: 1em 0;">
<source lang="xml">
<?xml version="1.0" encoding="UTF-8" ?>
<detector-grouping> 
  <group name="fwd1"> <ids val="1-32"/> </group> 
  <group name="bwd1"> <ids val="33,36,38,60-64"/> </group>   

  <group name="fwd2"><detids val="1,2,17,32"/></group> 
  <group name="bwd2"><detids val="33,36,38,60,64"/> </group> 
</detector-grouping>
</source></div>
where <ids> is used to specify spectra IDs and <detids> detector IDs.

== Previous Versions ==

=== version 1 ===
The set of spectra to be grouped can be given as a list of either spectrum numbers, detector IDs or workspace indices. The new, summed spectrum will appear in the workspace at the first workspace index of the pre-grouped spectra (which will be given by the ResultIndex property after execution). The detectors for all the grouped spectra will be moved (in the [[SpectraDetectorMap]]) to belong to the first spectrum. ''A technical note: the workspace indices previously occupied by summed spectra will have their data zeroed and their spectrum number set to a value of -1.''


*WIKI*/

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace DataHandling
{
/** An algorithm for grouping detectors and the spectra associated with them
    into a single DetectorGroup and spectrum.
    This algorithm can only be used on a workspace that has common X bins.

    Required Properties:
    <UL>
    <LI> Workspace - The name of the (input & output) Workspace2D on which to perform the algorithm </LI>
    </UL>

    Optional Properties (Only one of these should be set. Priority to highest listed below if more than one is set.):
    <UL>
    <LI> SpectraList - An ArrayProperty containing a list of spectra to combine </LI>
    <LI> DetectorList - An ArrayProperty containing a list of detector IDs to combine </LI>
    <LI> WorkspaceIndexList - An ArrayProperty containing the workspace indices to combine </LI>
    </UL>

    Output Properties:
    <UL>
    <LI> ResultIndex - The workspace index containing the grouped spectra </LI>
    </UL>

    @author Russell Taylor, Tessella Support Services plc
    @date 17/04/2008

    Copyright &copy; 2008-9 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class DLLExport GroupDetectors : public API::Algorithm
{
public:
  GroupDetectors();
  virtual ~GroupDetectors();

  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "GroupDetectors";};
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1;};
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "DataHandling\\Detectors";}

private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  // Implement abstract Algorithm methods
  void init();
  void exec();
};

} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_GROUPDETECTORS_H_*/
