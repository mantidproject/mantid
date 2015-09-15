#!/usr/bin/env python
VERSION = "1.0"

BANNED = ["__init__.py"]
DEF_PYTHON = "/usr/bin/env python"
DEF_XMLRUNNER = None
import os
import sys

class DriverGenerator:
    def __init__(self, directory, python=DEF_PYTHON,
                 xmlrunnerloc=DEF_XMLRUNNER, withXmlRunner=True):
        self.__directory = os.path.normpath(directory)
        self.__python=python
        self.__xmlrunnerloc = xmlrunnerloc
        self.__withxmlrunner = withXmlRunner
        self.classes = []
        self.__init_tests()

    def __init_tests(self):
        self.__test_package = os.path.split(self.__directory)[-1]

        files = os.listdir(self.__directory)
        files = [ item for item in files if item.endswith(".py")]
        files = [ item for item in files if not (item in BANNED)]

        classes = [item.split(".")[0] for item in files]
        
        self.classes = classes
        
        self.__tests = []
        self.__tests.extend(classes)

    def write(self, filename):
        handle = open(filename, 'w')

        # header bit for just having unittest
        handle.write("#!%s\n" % self.__python)
        handle.write("import unittest\n")
        handle.write("import sys")
        handle.write("\n")

        # import the various tests
        handle.write("# define the unit tests\n")
        for test in self.__tests:
            handle.write("from %s.%s import *\n" % \
                             (self.__test_package, test))
        handle.write("\n")

        # write the main
        handle.write("if __name__ == \"__main__\":\n")
        
        # --- Handle the --help-tests entry by printing out all test classes and tests ----------
        handle.write("""
    # ---- List the available tests (same as cxxtest) ----
    if len(sys.argv) > 1:
        if sys.argv[1] == '--help-tests':
            print "--------------------------------------------------------"
""")
        for classname in self.classes:
            modulefile = os.path.join(self.__directory, classname + ".py")
            for line in open(modulefile, 'r'):
                line = line.strip()
                n = line.find('(')
                if line.startswith("def test") and n > 4:
                    testname = line[4:n]
                    handle.write("            print '%s %s'\n" % (classname, testname))
        handle.write("            sys.exit()\n\n\n")
        
        # ----------- Continue with the runner code ------------
        handle.write("    # setup the xml runner\n")
        if self.__xmlrunnerloc is not None:
            handle.write("    import sys\n")
            handle.write("    sys.path.append(\"%s\")\n" % \
                          self.__xmlrunnerloc)
        handle.write("    import xmlrunner\n")
        handle.write("\n")
        handle.write("    unittest.main(")
        if self.__withxmlrunner:
            handle.write("testRunner=xmlrunner.XMLTestRunner(output='Testing')")
        handle.write(")\n")

        # cleanup
        handle.close()
        
        # Make file executable
        import stat
        os.chmod(filename, 0755)

if __name__ == "__main__":
    import optparse

    info=[]
    parser = optparse.OptionParser("usage: %prog [options]",
                                   None, optparse.Option, VERSION, 'error',
                                   " ".join(info))
    parser.add_option("-o", "--output", dest="output",
                      help="File to write to")
    parser.add_option("-d", "--dir", dest="directory",
                      help="Directory containing the suite tests to collect")
    parser.add_option("", "--xmlrunner", dest="xmlrunner",
                      help="Location of the xml runner")
    parser.add_option("", "--python", dest="python",
                      help="Location of python executable")
    parser.set_defaults(xmlrunner=DEF_XMLRUNNER, python=DEF_PYTHON)
    
    (options, args) = parser.parse_args()
    if len(args) > 0:
        parser.error("does not accept command line arguments without flags \"%s\"" % (", ".join(args)))

    

    generator = DriverGenerator(options.directory,
                                python=options.python,
                                xmlrunnerloc=options.xmlrunner)
    generator.write(options.output)
    
