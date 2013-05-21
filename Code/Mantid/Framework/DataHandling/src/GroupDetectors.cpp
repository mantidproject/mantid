/*WIKI* 


This algorithm sums, bin-by-bin, multiple spectra into a single spectra. The errors are summed in quadrature and the algorithm checks that the bin boundaries in X are the same. The new summed spectra are created at the start of the output workspace and have spectra index numbers that start at zero and increase in the order the groups are specified. Each new group takes the spectra numbers from the first input spectrum specified for that group. All detectors from the grouped spectra will be moved to belong to the new spectrum.

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

*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/GroupDetectors.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/WorkspaceOpOverloads.h"
#include "MantidKernel/ArrayProperty.h"
#include <set>
#include <numeric>

namespace Mantid
{
namespace DataHandling
{
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(GroupDetectors)

/// Sets documentation strings for this algorithm
void GroupDetectors::initDocs()
{
  this->setWikiSummary("Sums spectra bin-by-bin, equivalent to grouping the data from a set of detectors.  Individual groups can be specified by passing the algorithm a list of spectrum numbers, detector IDs or workspace indices. Many spectra groups can be created in one execution via an input file. ");
  this->setOptionalMessage("Sums spectra bin-by-bin, equivalent to grouping the data from a set of detectors.  Individual groups can be specified by passing the algorithm a list of spectrum numbers, detector IDs or workspace indices. Many spectra groups can be created in one execution via an input file.");
}


using namespace Kernel;
using namespace API;

/// (Empty) Constructor
GroupDetectors::GroupDetectors() {}

/// Destructor
GroupDetectors::~GroupDetectors() {}

void GroupDetectors::init()
{
  declareProperty(new WorkspaceProperty<>("Workspace","",Direction::InOut,
                                          boost::make_shared<CommonBinsValidator>()),
    "The name of the workspace2D on which to perform the algorithm");

  declareProperty(new ArrayProperty<specid_t>("SpectraList"),
    "An array containing a list of the indexes of the spectra to combine\n"
    "(DetectorList and WorkspaceIndexList are ignored if this is set)" );

  declareProperty(new ArrayProperty<detid_t>("DetectorList"),
    "An array of detector ID's (WorkspaceIndexList is ignored if this is\n"
    "set)" );

  declareProperty(new ArrayProperty<size_t>("WorkspaceIndexList"),
    "An array of workspace indices to combine" );

  declareProperty("ResultIndex", -1,
    "The workspace index of the summed spectrum (or -1 on error)",
    Direction::Output);
}

void GroupDetectors::exec()
{
  // Get the input workspace
  const MatrixWorkspace_sptr WS = getProperty("Workspace");

  std::vector<size_t> indexList = getProperty("WorkspaceIndexList");
  std::vector<specid_t> spectraList = getProperty("SpectraList");
  const std::vector<detid_t> detectorList = getProperty("DetectorList");

  // Could create a Validator to replace the below
  if ( indexList.empty() && spectraList.empty() && detectorList.empty() )
  {
    g_log.information(name() +
      ": WorkspaceIndexList, SpectraList, and DetectorList properties are all empty, no grouping done");
    return;
  }

  // Bin boundaries need to be the same, so check if they actually are
  if (!API::WorkspaceHelpers::commonBoundaries(WS))
  {
    g_log.error("Can only group if the histograms have common bin boundaries");
    throw std::runtime_error("Can only group if the histograms have common bin boundaries");
  }

  // If the spectraList property has been set, need to loop over the workspace looking for the
  // appropriate spectra number and adding the indices they are linked to the list to be processed
  if ( ! spectraList.empty() )
  {
    WS->getIndicesFromSpectra(spectraList,indexList);
  }// End dealing with spectraList
  else if ( ! detectorList.empty() )
  {
    // Dealing with DetectorList
    //convert from detectors to workspace indices
    WS->getIndicesFromDetectorIDs(detectorList, indexList);
  }

  if ( indexList.empty() )
  {
      g_log.warning("Nothing to group");
      return;
  }

  const size_t vectorSize = WS->blocksize();

  const specid_t firstIndex = static_cast<specid_t>(indexList[0]);
  ISpectrum * firstSpectrum = WS->getSpectrum(firstIndex);

  setProperty("ResultIndex",firstIndex);

  // loop over the spectra to group
  Progress progress(this, 0.0, 1.0, static_cast<int>(indexList.size()-1));
  for (size_t i = 0; i < indexList.size()-1; ++i)
  {
    // The current spectrum
    const size_t currentIndex = indexList[i+1];
    ISpectrum * spec = WS->getSpectrum(currentIndex);

    // Add the current detector to belong to the first spectrum
    firstSpectrum->addDetectorIDs(spec->getDetectorIDs());

    // Add up all the Y spectra and store the result in the first one
    // Need to keep the next 3 lines inside loop for now until ManagedWorkspace mru-list works properly
    MantidVec &firstY = WS->dataY(firstIndex);
    MantidVec::iterator fYit;
    MantidVec::iterator fEit = firstSpectrum->dataE().begin();
    MantidVec::iterator Yit = spec->dataY().begin();
    MantidVec::iterator Eit = spec->dataE().begin();
    for (fYit = firstY.begin(); fYit != firstY.end(); ++fYit, ++fEit, ++Yit, ++Eit)
    {
      *fYit += *Yit;
      // Assume 'normal' (i.e. Gaussian) combination of errors
      *fEit = sqrt( (*fEit)*(*fEit) + (*Eit)*(*Eit) );
    }

    // Now zero the now redundant spectrum and set its spectraNo to indicate this (using -1)
    // N.B. Deleting spectra would cause issues for ManagedWorkspace2D, hence the the approach taken here
    spec->dataY().assign(vectorSize,0.0);
    spec->dataE().assign(vectorSize,0.0);
    spec->setSpectrumNo(-1);
    spec->clearDetectorIDs();
    progress.report();
  }

}

} // namespace DataHandling
} // namespace Mantid

