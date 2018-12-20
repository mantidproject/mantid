/* Simple command line tool to parse a set of ARIEL instrument definition files
   and output the information in Mantid's XML instrument definition format.

   To run: "ArielToMantidXML [path] [instrument]" where [path] is the path (relative
   or absolute) to the set of ARIEL files and [instrument] is the name of the
   instrument as given by the name of the 'root' ARIEL file.

   Note that this will NOT produce a complete XML file. In particular, the detector
   IDs (UDETs) will be missing and need to be entered manually. There is also currently
   no 'shape' provided for the detectors, although there is at least some information
   on this in the ARIEL files.
*/

#include <iostream>
#include <string>
#include "Component.h"
#include "XMLWriter.h"

// Declare global variable for path
std::string G_PATH;

int main(int argc, const char* argv[])
{
  // Check correct arguments have been passed
  if ( argc != 3 )
  {
    std::cout << "To run: ArielToMantidXML [path] [instrument], e.g. ArielToMantidXML ../ GEM" << std::endl;
    exit(EXIT_FAILURE);
  }

  // First argument should be the path
  G_PATH = argv[1];
  // Append / to path if not already provided in argument
  if (G_PATH.compare(G_PATH.size()-1,1,"/") && G_PATH.compare(G_PATH.size()-1,1,"\\") ) G_PATH.append("/");
  const std::string instrumentName(argv[2]);

  // Create the 'top' component
  Component instrument(instrumentName, instrumentName);
  // Traverse the tree to build up the full structure
  instrument.findChildren();

  // Now write out the XML file
  XMLWriter writer(instrumentName, &instrument);
  writer.writeDetectors();

  std::cout << Component::counter << " detectors found" << std::endl;
}
