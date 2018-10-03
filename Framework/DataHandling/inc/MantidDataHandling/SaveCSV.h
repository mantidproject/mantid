// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_SAVECSV_H_
#define MANTID_DATAHANDLING_SAVECSV_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"

namespace Mantid {
namespace DataHandling {
/** @class SaveCSV SaveCSV.h DataHandling/SaveCSV.h

Saves a 1D or 2D workspace to a CSV file. SaveCSV is an algorithm and as such
inherits from the Algorithm class, via DataHandlingCommand, and overrides
the init() & exec()  methods.

Required Properties:
<UL>
<LI> Filename - The filename of the output CSV file </LI>
<LI> InputWorkspace - The name of a workspace containing the data you want to
save to a CSV file </LI>
</UL>

Optional Properties:
<UL>
<LI> Separator - defaults to "," </LI>
<LI> LineSeparator - defaults to "\n" </LI>
</UL>

The format of the saved ascii CSV file for a 1D worksspace consists of three
columns where the numbers of each row are seperated by the Seperator and
each line by the LineSeperator.

The format of the saved CSV file for a 2D workspace is as follows:

A      0, 200, 400, 600, ..., 50000 <BR>
0     10,   4, 234,  35, ...,    32 <BR>
1      4, 234,   4,   9, ...,    12 <BR>
A      0, 100, 200, 300, ..., 25000 <BR>
2     34,   0,   0,   0, ...,    23

ERRORS<BR>
0    0.1, 3.4, 2.4, 3.5, ...,     2 <BR>
1    3.1, 3.3, 2.5, 3.5, ...,     2 <BR>
2    1.1, 3.3, 2.4,   5, ...,   2.4

where for the matrix above the ERRORS line the first column
shows the content of the numbers on the of the same line; i.e.
'A' is followed by x-axis values (e.g. TOF values) and any number
(e.g. '2') followed by y-axis values (e.g. neutron counts). Multiple
'A' may be present to allow for the a-axis to change. So in
the example above the saved 2D workspace consists of three histograms
(y-axes) where the first two have the same x-axis but the third
histogram has a different x-axis.

The matrix following the ERRORS line lists the errors as recorded
for each histogram.

@author Anders Markvardsen, ISIS, RAL
@date 15/10/2007
*/
class DLLExport SaveCSV : public API::Algorithm {
public:
  /// Default constructor
  SaveCSV();
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "SaveCSV"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Saves a 1D or 2D workspace to a CSV file.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"SaveAscii"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "DataHandling\\Text"; }

private:
  /// Overwrites Algorithm method. Does nothing at present
  void init() override;

  /// Overwrites Algorithm method
  void exec() override;

  /// Saves out x errors
  void saveXerrors(std::ofstream &stream,
                   const Mantid::DataObjects::Workspace2D_sptr workspace,
                   const size_t numberOfHist);

  /// The name of the file used for storing the workspace
  std::string m_filename;

  /// The seperator for the CSV file
  std::string m_separator;

  /// The line seperator for the CSV file
  std::string m_lineSeparator;
};

} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_SAVECSV_H_*/
