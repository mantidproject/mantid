// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "LoadCanSAS1D.h"
#include "MantidDataObjects/Workspace2D.h"
#include <Poco/DOM/Element.h>
#include <Poco/DOM/Node.h>
//----------------------------------------------------------------------

namespace Poco {
namespace XML {
class Element;
}
} // namespace Poco

namespace Mantid {
namespace DataHandling {
/** @class LoadCanSAS1D2  DataHandling/LoadCanSAS1D2.h

This algorithm loads 1 CanSAS1d xml file into a workspace.
It implements the CanSAS - version 1.1 standard
(http://www.cansas.org/svn/1dwg/tags/v1.1/cansas1d.xsd).

The main difference between the CanSAS1D version 1.0 (implemented at
version 1 of this algorithm (LoadCanSAS1D) and the version 1.1 is
that the later version introduced the element SAStransmission_spectrum.

This means that right now, a file may have the reduced data, as well as
some spectra related to the transmission. In order not to break the
signature proposed on LoadCanSAS1D version 1.0, a new Property will be
introduced:
  - LoadTransmission - boolean flag with default False.

If the user let the LoadTransmissionData false, than the signature will be:
  - OutputWs = LoadCanSAS1D(filename)

If the user set the LoadTransmission, than, it will receive the output in this
way:
  - OutputWs, TransWs, TransCanWs = LoadCanSAS1D(filename,LoadTransmission)
The values of TransWs and TransCanWs may be None if the related data was not
found
at filename.

@author Gesner Passos, Rutherford Appleton Laboratory
@date 12/04/2013
*/
class DLLExport LoadCanSAS1D2 : public LoadCanSAS1D {
public:
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 2; }
  const std::vector<std::string> seeAlso() const override { return {"SaveCanSAS1D"}; }

protected:
  /// Overwrites Algorithm method. Extend to create the LoadTransmission flag.
  void init() override;

  void exec() override;
  /// Extends the LoadEntry to deal with the possibility of Transmission data.
  API::MatrixWorkspace_sptr loadEntry(Poco::XML::Node *const workspaceData, std::string &runName) override;
  /// Add new method to deal with loading the transmission related data.
  API::MatrixWorkspace_sptr loadTransEntry(Poco::XML::Node *const workspaceData, std::string &runName,
                                           std::string trans_name);

  std::vector<API::MatrixWorkspace_sptr> trans_gp, trans_can_gp;

private:
  void processTransmission(std::vector<API::MatrixWorkspace_sptr> &trans_gp, const std::string &name,
                           const std::string &output_name);
};
} // namespace DataHandling
} // namespace Mantid