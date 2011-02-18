#include <iostream>
#include <iomanip>
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/Workspace.h"
#include "MantidDataObjects/Workspace1D.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/Memory.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/Strings.h"
#include <set>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;

/* Application to make a wiki page text from an algorithm */

//--------------------------------------------------------------------------
void printHelp()
{
  std::cout
  << "WikiMaker: Utility to make Mantid wiki pages.\n"
  << "---------------------------------------------\n"
  << "USAGE: WikiMaker ALGORITHM_NAME [outputfile]\n"
  << "\n"
  << "Please specify an algorithm name!\n"
  ;
}


//--------------------------------------------------------------------------
/** Make a wiki text from the properties of an algorithm.
  *
 * @param alg :: pointer to the algorithm.
 */
std::string makeWikiText(IAlgorithm * alg)
{
  // To pipe out the string
  std::ostringstream out;
  out << "== Summary ==\n\n";
  out << Strings::replace( alg->getOptionalMessage(), "\n", " ") << "\n\n";
  out << "== Properties ==\n\n";

  out << "{| border=\"1\" cellpadding=\"5\" cellspacing=\"0\" \n"
      << "!Order\n!Name\n!Direction\n!Type\n!Default\n!Description\n"
      << "|-\n";

  int propNum = 1;
  const std::vector< Property* >& props = alg->getProperties();
  std::vector< Property* >::const_iterator it;
  for (it = props.begin(); it != props.end(); it++)
  {
    const Property * p = *it;
    if (p)
    {
      // The property number
      out << "|" << propNum << "\n";
      // Name of the property
      out << "|" << p->name() << "\n";
      // Direction
      out << "|" << Kernel::Direction::asText(p->direction()) << "\n";
      // Type
      out << "|" << p->type() << "\n";

      // Default?
      if (p->isValid()=="") //Nothing was set, but it's still valid = NOT mandatory
      {
        out << "|" << p->getDefault() << "\n";
      }
      else
      {
        out << "|Mandatory\n";
      }
      // Documentation
      out << "|" << Strings::replace( p->documentation(), "\n", " ") << "\n";
      // End of table line
      out << "|-\n";
      propNum++;
    }
  }

  //Close the table
  out << "|}\n\n";

  out << "== Description ==\n";
  out << "\n";
  out << "INSERT FULL DESCRIPTION HERE\n";
  out << "\n";
  out << "[[Category:Algorithms]]\n";
  out << "[[Category:" << alg->category() << "]]\n";
  out << "{{AlgorithmLinks|" << alg->name() << "}}\n";

  return out.str();
}



//--------------------------------------------------------------------------
int main(int argc, char *argv[])
{
  // Must specift the algorigthm
  if (argc < 2)
  {
    printHelp();
    exit(1);
  }
  std::string algName(argv[1]);
  // Empty name?
  if (algName == "")
  {
    printHelp();
    exit(2);
  }
  // Optional output filename
  std::string filename = "";
  if (argc >= 2)
    filename = argv[2];

  // Get the algorithm
  FrameworkManagerImpl& fm = FrameworkManager::Instance();
  IAlgorithm *alg;
  // Create it; will throw if not found
  alg = fm.createAlgorithm(algName);

  // Make it!
  std::string res = makeWikiText(alg);

  // Output to screen
  std::cout << "\n\n" << res;

  if (filename != "")
  {
    std::ofstream myfile;
    myfile.open(filename.c_str());
    myfile << res;
    myfile.close();
    std::cout << "\n\n... Written to: " << filename << "\n\n";
  }


  exit(0);
}
