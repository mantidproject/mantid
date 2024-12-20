// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/SaveMask.h"

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/ISpectrum.h"
#include "MantidDataObjects/SpecialWorkspace2D.h"

#include <Poco/DOM/AutoPtr.h>
#include <Poco/DOM/DOMWriter.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/Text.h>
#include <Poco/XML/XMLWriter.h>

#include <algorithm>
#include <fstream>
#include <memory>
#include <sstream>

using namespace Mantid::Kernel;
using namespace Mantid::API;

using namespace Poco::XML;

namespace Mantid::DataHandling {

DECLARE_ALGORITHM(SaveMask)

/// Define input parameters
void SaveMask::init() {

  declareProperty(std::make_unique<API::WorkspaceProperty<MatrixWorkspace>>("InputWorkspace", "", Direction::Input),
                  "Workspace to output masking to XML file");
  declareProperty(std::make_unique<FileProperty>("OutputFile", "", FileProperty::Save, ".xml"),
                  "File to save the detectors mask in XML format");
}

/// Main body to execute algorithm
void SaveMask::exec() {
  // 1. Get input
  API::MatrixWorkspace_sptr userInputWS = this->getProperty("InputWorkspace");

  DataObjects::SpecialWorkspace2D_sptr inpWS = std::dynamic_pointer_cast<DataObjects::SpecialWorkspace2D>(userInputWS);
  if (!inpWS) {
    // extract the masking and use that
    Algorithm_sptr emAlg = this->createChildAlgorithm("ExtractMask", 0.0, 0.5, false);
    emAlg->setProperty("InputWorkspace", userInputWS);
    emAlg->setPropertyValue("OutputWorkspace", "tmp");
    emAlg->setLogging(this->isLogging());
    emAlg->execute();
    API::MatrixWorkspace_sptr ws = emAlg->getProperty("OutputWorkspace");
    inpWS = std::dynamic_pointer_cast<DataObjects::SpecialWorkspace2D>(ws);
    if (!inpWS) {
      throw std::runtime_error("Unable to extract masking data using ExtractMask");
    }
  }

  std::string outxmlfilename = this->getPropertyValue("OutputFile");

  // 2. Convert Workspace to ...
  std::vector<detid_t> detid0s;
  for (size_t i = 0; i < inpWS->getNumberHistograms(); i++) {
    if (inpWS->y(i).front() > 0.1) {
      // It is way from 0 but smaller than 1
      const auto &ids = inpWS->getSpectrum(i).getDetectorIDs();
      std::copy(ids.cbegin(), ids.cend(), std::back_inserter(detid0s));
    }
  }

  // d) sort
  g_log.debug() << "Number of detectors to be masked = " << detid0s.size() << '\n';

  // 3. Count workspace to count 1 and 0
  std::vector<detid_t> idx0sts; // starting point of the pair
  std::vector<detid_t> idx0eds; // ending point of pair

  if (!detid0s.empty()) {
    std::sort(detid0s.begin(), detid0s.end());

    detid_t i0st = detid0s[0];
    detid_t i0ed = detid0s[0];

    for (size_t i = 1; i < detid0s.size(); i++) {

      if (detid0s[i] == detid0s[i - 1] + 1) {
        // If it is continuous: record the current one
        i0ed = detid0s[i];
      } else {
        // If skip: restart everything
        // i) record previous result
        idx0sts.emplace_back(i0st);
        idx0eds.emplace_back(i0ed);
        // ii) reset the register
        i0st = detid0s[i];
        i0ed = detid0s[i];
      }

    } // for

    // Complete the registration
    idx0sts.emplace_back(i0st);
    idx0eds.emplace_back(i0ed);

    for (size_t i = 0; i < idx0sts.size(); i++) {
      g_log.information() << "Section " << i << " : " << idx0sts[i] << "  ,  " << idx0eds[i]
                          << " to be masked and recorded.\n";
    }
  } // Only work for detid > 0

  // 4. Write out to XML nodes
  // a) Create document and root node
  AutoPtr<Document> pDoc = new Document;
  AutoPtr<Element> pRoot = pDoc->createElement("detector-masking");
  pDoc->appendChild(pRoot);
  // pRoot->setAttribute("default", "use");

  // b) Append Group
  AutoPtr<Element> pChildGroup = pDoc->createElement("group");
  // pChildGroup->setAttribute("type", "notuse");
  pRoot->appendChild(pChildGroup);

  // c) Append detid
  // c1. Generate text value
  std::stringstream ss;
  for (size_t i = 0; i < idx0sts.size(); i++) {
    size_t ist = idx0sts[i];
    size_t ied = idx0eds[i];

    // a-b or a
    bool writedata = true;
    if (ist < ied) {
      ss << ist << "-" << ied;
    } else if (ist == ied) {
      ss << ist;
    } else {
      writedata = false;
    }
    // add ","
    if (writedata && i < idx0sts.size() - 1) {
      ss << ",";
    }

  } // for
  std::string textvalue = ss.str();
  g_log.debug() << "SaveMask main text:  available section = " << idx0sts.size() << "\n" << textvalue << '\n';

  // c2. Create element
  AutoPtr<Element> pDetid = pDoc->createElement("detids");
  AutoPtr<Text> pText1 = pDoc->createTextNode(textvalue);
  pDetid->appendChild(pText1);
  pChildGroup->appendChild(pDetid);

  // 4. Write
  DOMWriter writer;
  writer.setNewLine("\n");
  writer.setOptions(XMLWriter::PRETTY_PRINT);

  std::ofstream ofs;
  ofs.open(outxmlfilename.c_str(), std::fstream::out);

  ofs << "<?xml version=\"1.0\"?>\n";

  writer.writeNode(std::cout, pDoc);
  writer.writeNode(ofs, pDoc);
  ofs.close();
}

} // namespace Mantid::DataHandling
