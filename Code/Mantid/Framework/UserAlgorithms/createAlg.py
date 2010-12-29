#####################################################################################
#                                                                                   #
# This is a simple script to create header (.h) and source (.cpp) stubs for a new   #
# algorithm for use in Mantid.                                                      #
#                                                                                   #
# Usage: python createAlg.py [AlgorithmName] [Category (optional)]                  #
#                                                                                   #
# Author: Russell Taylor, Tessella Support Services plc, 12/09/2008                 #
#####################################################################################

import sys
import string

args = len(sys.argv)
if args == 1:
    print "\nMissing arguments"
    sys.exit("Usage: python createAlg.py [AlgorithmName] [Category (optional)]\n")

name = sys.argv[1]
if args > 2:
  cat = sys.argv[2]
else:
  cat = "UserAlgorithms"
  
hfilename = name + ".h"
cppfilename = name + ".cpp"

h = open(hfilename, "w")
h.write("#ifndef " + string.upper(name) + "_H_\n"
        "#define " + string.upper(name) + "_H_\n\n"
        "#include \"MantidAPI/Algorithm.h\"\n\n"
        "class " + name + " : public Mantid::API::Algorithm\n"
        "{\n"
        "public:\n"
        "  /// (Empty) Constructor\n"
        "  " + name + "() : Mantid::API::Algorithm() {}\n"
        "  /// Virtual destructor\n"
        "  virtual ~" + name +"() {}\n"
        "  /// Algorithm's name\n"
        "  virtual const std::string name() const { return \"" + name + "\"; }\n"
        "  /// Algorithm's version\n"
        "  virtual const int version() const { return (1); }\n"
        "  /// Algorithm's category for identification\n"
        "  virtual const std::string category() const { return \"" + cat + "\"; }\n\n"
        "private:\n"
        "  /// Initialisation code\n"
        "  void init();\n"
        "  ///Execution code\n"
        "  void exec();\n\n"
        "};\n\n"
        "#endif /*" + string.upper(name) + "_H_*/\n"
        )
h.close()

c = open(cppfilename, "w")
c.write("#include \"" + hfilename + "\"\n\n"
        "// Register the algorithm into the AlgorithmFactory\n"
        "DECLARE_ALGORITHM(" + name + ")\n\n"
        "using namespace Mantid::Kernel;\n"
        "using namespace Mantid::API;\n"
        "using namespace Mantid::Geometry;\n\n"
        "// A reference to the logger is provided by the base class, it is called g_log.\n"
        "// It is used to print out information, warning and error messages\n\n"
        "void " + name + "::init()\n{\n"
        "  // Put your initialisation code (e.g. declaring properties) here...\n\n"
        "  // Virtually all algorithms will want an input and an output workspace as properties.\n"
        "  // Here are the lines for this, so just uncomment them:\n"
        "  //   declareProperty(new WorkspaceProperty<>(\"InputWorkspace\",\"\",Direction::Input));\n"
        "  //   declareProperty(new WorkspaceProperty<>(\"OutputWorkspace\",\"\",Direction::Output));\n\n"
        "}\n\n"
        "void " + name + "::exec()\n{\n"
        "  // Put the algorithm execution code here... \n\n"
        "  // The first thing to do will almost certainly be to retrieve the input workspace.\n"
        "  // Here's the line for that - just uncomment it:\n"
        "  //   MatrixWorkspace_sptr inputWorkspace = getProperty(\"InputWorkspace\");\n\n"
        "}\n\n"
        )
c.close()

print "\nSuccessfully created " + hfilename + " & " + cppfilename + " files.\n"
