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
#include "MantidAPI/Algorithm.h"
#include "MantidDataHandling/DllConfig.h"
#include "SaveCanSAS1D.h"
#include <fstream>

namespace Poco {
namespace XML {
class Document;
class Element;
class Text;
} // namespace XML
} // namespace Poco

namespace Mantid {
namespace DataHandling {
/** @class SaveCanSAS1D2  DataHandling/SaveCanSAS1D2.h

This algorithm saves  workspace into CanSAS1d format. This is an xml format
except
the \<Idata\>, \<\/Idata\> tags and all data in between must be one line, which
necesitates
the files be written iostream functions outside xml libraries.

The second version of CanSAS1D implements the version 1.1, whose schema is found
at
http://www.cansas.org/formats/1.1/cansas1d.xsd. See the tutorial for more
infomation
about: http://www.cansas.org/svn/1dwg/trunk/doc/cansas-1d-1_1-manual.pdf.

The first version of SaveCanSAS1D implemented the version 1.0 of CanSAS.
The main difference among them is the definition of the SASRoot and the
introduction of a new element called SAStransmission_spectrum, which allows
to record the Spectrum of the transmission for the sample and can.

So, the SaveCanSAS1D2 will extend SaveCanSAS1D in the following:

 - Introduction of 2 new (optional) workspace properties:
   - Transmission - The workspace for of the transmission
   - TransmissionCan - The workspace for the transmission can
 - Extension of the SaveCanSAS1D2::init method in order to introduce these
workspaces properties.
 - Overide the SaveCanSAS1D2::createSASRootElement to conform the new header.
 - Introduction of the method to deal with the new element:
SaveCanSAS1D2::createSASTransElement.
 - Override the SaveCanSAS1D2::exec method to introduce this new element when
apropriated.
 - Override the SaveCanSAS1D2::writeHeader method to introduce set the correct
stylesheet

@author Gesner Passos, Rutherford Appleton Laboratory
@date 11/04/2013
*/
class MANTID_DATAHANDLING_DLL SaveCanSAS1D2 : public SaveCanSAS1D {
public:
  int version() const override { return 2; }
  const std::vector<std::string> seeAlso() const override { return {"LoadCanSAS1D", "SaveNXcanSAS"}; }

protected:
  /// Extends the SaveCanSAS1D init method
  void init() override;
  /// Overwrites Algorithm method
  void exec() override;

  /// Create the SASRoot element
  void createSASRootElement(std::string &rootElem) override;

  /// this method creates SAStransmission_spectrum element
  void createSASTransElement(std::string &sasTrans, const std::string &name);

  /// this method creates SASProcess element
  void createSASProcessElement(std::string &sasProcess);

  /// Overwrites writeHeader method
  void writeHeader(const std::string &fileName) override;

  /// points to the workspace that will be written to file
  API::MatrixWorkspace_const_sptr m_trans_ws, m_transcan_ws;
};
} // namespace DataHandling
} // namespace Mantid
