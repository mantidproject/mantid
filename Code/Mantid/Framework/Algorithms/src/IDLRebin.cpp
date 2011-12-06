#include "MantidAlgorithms/IDLRebin.h"
#include "MantidKernel/System.h"
#include "MantidAPI/FileProperty.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/EventList.h"
#include <math.h>
#include <iostream>
#include <fstream>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace Algorithms
{

DECLARE_ALGORITHM(IDLRebin)

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  IDLRebin::IDLRebin()
  {
    // TODO Auto-generated constructor stub
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  IDLRebin::~IDLRebin()
  {
    // TODO Auto-generated destructor stub
  }
  
  void IDLRebin::init(){

    declareProperty(new FileProperty("TOFTemplateFile", "", FileProperty::Load, ".dat"),
        "Data file with (tof, y, e) column");

    declareProperty(new WorkspaceProperty<Mantid::DataObjects::EventWorkspace>("InputWorkspace", "", Direction::Input),
        "Input Event Workspace For Rebinning");

    declareProperty(new FileProperty("OutputFileBank1", "", FileProperty::Save, ".dat"),
        "Data file has the rebinned data");

    declareProperty(new FileProperty("OutputFileBank2", "", FileProperty::Save, ".dat"),
        "Data file has the rebinned data");

  }

  void IDLRebin::exec(){

    // 1. Get Inputs
    DataObjects::EventWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
    std::string templatefilename = getPropertyValue("TOFTemplateFile");
    std::string outputbank1filename = getPropertyValue("OutputFileBank1");
    std::string outputbank2filename = getPropertyValue("OutputFileBank2");
    std::vector<std::string> outputfilenames;
    outputfilenames.push_back(outputbank1filename);
    outputfilenames.push_back(outputbank2filename);

    // 2. Read TOF template
    std::ifstream ifs(templatefilename.c_str());
    if (!ifs){
      g_log.error() << "File " << templatefilename << "  cannot be open" << std::endl;
      throw std::invalid_argument("Cannot open file");
    }

    std::vector<double> tofs;
    std::string line;
    double tof, intensity, error;
    while(getline(ifs, line)){
      std::stringstream ss(line);
      ss >> tof >> intensity >> error;
      tofs.push_back(tof);
    }

    g_log.notice() << "Number (TOFs) = " << tofs.size() << " within [" << tofs[0] << ", " << tofs[tofs.size()-1] << "]\n";

    // 3. Rebin
    for (size_t ispec=0; ispec < inputWS->getNumberHistograms(); ispec ++){
      // 3.1 Rebin
      DataObjects::EventList events = inputWS->getEventList(ispec);
      std::vector<double> eventtofs;
      events.getTofs(eventtofs);
      std::vector<double> rebindata(tofs.size());
      this->rebinIDL(tofs, eventtofs, rebindata);

      // 3.2 Output
      g_log.notice() << "Write Spectrum " << ispec << " To File " << outputfilenames[ispec] << std::endl;
      std::ofstream ofs(outputfilenames[ispec].c_str());
      for (size_t i = 0; i < rebindata.size(); i ++){
        ofs << tofs[i] << "\t\t" << rebindata[i] << std::endl;
      }

      ofs.close();

    }

  }

  /*
   */
  void IDLRebin::rebinIDL(const std::vector<double> tofs, const std::vector<double> events, std::vector<double>& rebindata){
    // 1. Init
    g_log.notice() << "Size of RebinnedData = " << rebindata.size() << "  Number(Events) = " << events.size() << std::endl;
    for (size_t i = 0; i < rebindata.size(); i ++){
      rebindata[i] = 0;
    }

    // 2. Rebin
    for (size_t i = 0; i < events.size(); i ++){
      // for (size_t i = 0; i < 20; i ++){
      size_t binindex = locateEventInBin(events[i], tofs);
      rebindata[binindex] += 1;
    }

    return;
  }

  /***
   * IDL  Style...
   */
  size_t IDLRebin::locateEventInBin(double eventtof, std::vector<double> tofs){

    size_t searchlimit = size_t(std::log(double(tofs.size()))/std::log(2))+3;

    // g_log.notice() << "Search count limit = " << searchlimit << std::endl;

    size_t iloc = 0;

    if (eventtof < tofs[0]){
      // Out of lower boundary
      iloc = 0;

    } else if (eventtof > tofs[tofs.size()-1]){
      // Out of upper boundary
      iloc = tofs.size()-1;

    } else {
      // Binary search
      size_t ist = 0;
      size_t ied = tofs.size()-1;
      bool found = false;

      size_t counts = 0;

      while (!found){

        // g_log.debug() << "Step " << counts << ":  " << ist << "   ....  " << ied << std::endl;

        if (ied-ist == 1 || ied-ist == 0){
          // a) End case, tofs[ist] < eventtof < tofs[ied]
          found = true;
          iloc = ist;
          // g_log.debug() << "Locate Event (" << eventtof << ") Between " << tofs[ist] << " , " << tofs[ied] << std::endl;

        } else {
          // b) Binary search
          size_t mid = (ied+ist)/2;
          if (eventtof < tofs[mid]){
            ied = mid;
          } else {
            ist = mid;
          }
        } // if-else

        counts ++;
        if (counts > searchlimit){
          iloc = 0;
          g_log.error() << "Cannot locate Event with TOF = " << eventtof << " ... Exceeds max search limit: " << searchlimit << std::endl;
          g_log.error() << "Final index = " << ist << ", " << ied << "   TOF = " << tofs[ist] << ",  " << tofs[ied] << std::endl;
          throw std::invalid_argument("Debug Stop");
          break;
        }

      } // while
    }

    return iloc;
  }



} // namespace Mantid
} // namespace Algorithms

