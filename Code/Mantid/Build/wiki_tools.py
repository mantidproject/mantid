#!/usr/bin/env python
""" A few tools for validating the wiki at mantidproject.org """
import warnings
import optparse
import os
import ConfigParser
import sys
import codecs
import fnmatch
import platform
import re

module_name=os.path.basename(os.path.splitext(__file__)[0])
mantid_initialized = False
mantid_debug = False
python_d_exe = None
# no version identier
noversion = -1
# Direction
InputDirection = "Input"
OutputDirection = "Output"
InOutDirection = "InOut"
NoDirection = "None"
direction_string = [InputDirection, OutputDirection, InOutDirection, NoDirection]

# Holds on to location of sources
file_search_done=False
cpp_files = []
cpp_files_bare = []
python_files = []
python_files_bare = []

# Missing description tag.
missing_description = "INSERT FULL DESCRIPTION HERE"

#======================================================================
def remove_wiki_from_header():
    """One-time method to remove *WIKI* tags from all header files """
    parent_dir = os.path.abspath(os.path.join(os.path.split(__file__)[0], os.path.pardir))
    for root, dirnames, filenames in os.walk(parent_dir):
      for filename in fnmatch.filter(filenames, '*.h'):
        fullfile = os.path.join(root, filename)
        f = open(fullfile,'r')
        lines = f.read().split('\n')
        f.close()
        n = 0
        # The output file
        outlines = []
        while n < len(lines) and (not lines[n].startswith("/*WIKI*") and not lines[n].startswith('"""*WIKI*')):
            outlines.append( lines[n] )
            n += 1
        #Skip all the WIKI lines
        while (n < len(lines)) and not lines[n].startswith("*WIKI*"):
            n += 1
        n += 1
        # Copy the rest
        while n < len(lines):
            outlines.append( lines[n] )
            n += 1
        
        f = open(fullfile,'w')
        f.write('\n'.join(outlines))
        f.close()

#======================================================================
def add_wiki_description(algo, wikidesc):
    """One-time use method that adds a wiki description  in the algo's CPP file under comments tag."""
    wikidesc = wikidesc.split('\n')
    
    source = find_algo_file(algo)
    if source != '':
        if len("".join(wikidesc)) == 0:
            print "No wiki description found to add!!!!"
        
        f = open(source,'r')
        lines = f.read().split('\n')
        f.close()
        
        #What lines are we adding?
        if source.endswith(".py"):
            adding = ['"""*WIKI* ', ''] + wikidesc + ['*WIKI*"""'] 
        else:
            adding = ['/*WIKI* ', ''] + wikidesc + ['*WIKI*/'] 
    
        lines = adding + lines
        
        text = "\n".join(lines)
        f = codecs.open(source, encoding='utf-8', mode='w+')
        f.write(text)
        f.close()

#======================================================================
def get_wiki_description(algo, version):
    tryUseDescriptionFromBinaries = True
    return get_custom_wiki_section(algo, version, "*WIKI*", tryUseDescriptionFromBinaries)

#======================================================================
def get_wiki_usage(algo, version):
    wiki_usage = get_custom_wiki_section(algo, version, "*WIKI_USAGE*")
    wiki_no_sig_usage = get_custom_wiki_section(algo, version, "*WIKI_USAGE_NO_SIGNATURE*")
    
    if wiki_usage:
        return (True, wiki_usage)
    elif wiki_no_sig_usage:
        return (False, wiki_no_sig_usage)
    else:
        return (True, "")
            

#======================================================================
def get_custom_wiki_section(algo, version, tag, tryUseDescriptionFromBinaries=False, verbose=True):
    """ Extract the text between the *WIKI* tags in the .cpp file
    
    @param algo :: name of the algorithm
    @param version :: version, -1 for latest 
    """
    
    desc = ""
    source = find_algo_file(algo, version)
    if source == '' and tryUseDescriptionFromBinaries:
        from mantid.api import AlgorithmManager
        alg = AlgorithmManager.createUnmanaged(algo, version)
        print "Getting algorithm description from binaries."
        return alg.getWikiDescription()
    elif source == '' and not tryUseDescriptionFromBinaries:
        print "Warning: Cannot find source for algorithm"
        return desc
    else:
        return get_wiki_from_source(source, tag, verbose)

def get_fitfunc_summary(name, verbose=True):
    source = find_fitfunc_file(name)
    if len(source) <= 0:
        print "Warning: Cannot find source for fitfunction '%s'" % name
        return ""
    return get_wiki_from_source(source, "*WIKI*", verbose)

def get_wiki_from_source(source, tag, verbose=True):
        f = open(source,'r')
        lines = f.read().split('\n')
        #print lines
        f.close()
        
        print source
        desc = ""
        try:
            # Start and end location markers.
            start_tag_cpp = "/" + tag 
            start_tag_python = '"""%s' % tag
            end_tag_cpp = tag + "/"
            end_tag_python = '%s"""' % tag
            
            # Find the start and end lines for the wiki section in the source.
            start_index = 0
            end_index = 0
            for line_index in range(0, len(lines)):
                line = lines[line_index]
                if line.lstrip().startswith(start_tag_cpp) or line.lstrip().startswith(start_tag_python):
                    start_index = line_index + 1
                    continue
                if line.lstrip().startswith(end_tag_cpp) or line.lstrip().startswith(end_tag_python):
                    end_index = line_index
                    break
            
            # Concatinate across the range.
            for line_index in range(start_index, end_index):
                desc += lines[line_index] + "\n"

            if verbose:
                if start_index == end_index:
                    print "No  '%s' section in source '%s'." % (tag, source)
                else:
                    print "Getting '%s' section from source '%s'." % (tag, source)
        
        except IndexError:
            print "No '%s' section in source '%s'." % (tag, source)
        return desc        
        
#======================================================================
def create_function_signature(alg, algo_name):
    """
    create the function signature for the algorithm.
    """
    import mantid.simpleapi
    from mantid.api import IWorkspaceProperty
    from mantid.simpleapi import _get_function_spec

    _alg = getattr(mantid.simpleapi, algo_name)
    prototype =  algo_name + _get_function_spec(_alg)
    
    # Replace every nth column with a newline.
    nth = 4
    commacount = 0
    prototype_reformated = ""
    for char in prototype:
        if char == ',':
            commacount += 1
            if (commacount % nth == 0):
                prototype_reformated += ",\n  "
            else:
                prototype_reformated += char
        else: 
           prototype_reformated += char
           
    # Strip out the version.
    prototype_reformated = prototype_reformated.replace(",[Version]", "")
    
    # Add the output properties
    props = alg.getProperties()
    allreturns = []
    # Loop through all the properties looking for output properties
    for prop in props:
        if (direction_string[prop.direction] == OutputDirection):
            allreturns.append(prop.name)
                
    lhs = ""
    comments = ""
    if not allreturns:
        pass
    elif (len(allreturns) == 1): 
        lhs =   allreturns[0] + " = "
    else :
        lhs = "result = "
        comments = "\n "
        comments += "\n # -------------------------------------------------- \n"
        comments += " # result is a tuple containing\n"
        comments += " # (" + ",".join(allreturns ) + ")\n"
        comments += " # To access individual outputs use result[i], where i is the index of the required output.\n"
        
    return lhs + prototype_reformated + comments

def filter_blacklist_directories(dirnames):
    blacklist = ['MantidPlot', 'MantidQt']
    filtered = dirnames
    for banneddir in blacklist:
        if banneddir in dirnames:
            filtered.remove(banneddir)
    return filtered

#======================================================================
def intialize_files():
    """ Get path to every header file """
    global file_matches
    parent_dir = os.path.abspath(os.path.join(os.path.split(__file__)[0], os.path.pardir))
    file_matches = []
    for root, dirnames, filenames in os.walk(parent_dir):
      # Filter out mantidplot from the file search. There are a few file in MantidPlot we don't want to accidently search, such as FFT.
      dirnames = filter_blacklist_directories(dirnames)
      for filename in fnmatch.filter(filenames, '*.cpp'):
          fullfile = os.path.join(root, filename)
          cpp_files.append(fullfile)
          cpp_files_bare.append( os.path.split(fullfile)[1] )
      for filename in fnmatch.filter(filenames, '*.py'):
          fullfile = os.path.join(root, filename)
          python_files.append(fullfile)
          python_files_bare.append( os.path.split(fullfile)[1] )
    


#======================================================================
def find_algo_file(algo, version=-1):
    """Find the files for a given algorithm (and version)"""
    global file_search_done
    if not file_search_done:
        intialize_files()
        file_search_done=True

    source = ''
    filename = algo
    if version > 1:
        filename += '%d' % version
    cpp = filename + ".cpp"
    pyfile = filename + ".py"
    if cpp in cpp_files_bare:
        n = cpp_files_bare.index(cpp, )
        source = cpp_files[n]
    elif pyfile in python_files_bare:
        n = python_files_bare.index(pyfile, )
        source = python_files[n]
    return source

#======================================================================
def find_fitfunc_file(name):
    """Find the files for a given algorithm (and version)"""
    global file_search_done
    if not file_search_done:
        intialize_files()
        file_search_done=True

    source = ''
    filename = name
    cpp = filename + ".cpp"
    pyfile = filename + ".py"
    if cpp in cpp_files_bare:
        candidates = []
        total = cpp_files_bare.count(cpp)
        index = -1
        while len(candidates) < total:
            index = cpp_files_bare.index(cpp, index+1)
            candidates.append(cpp_files[index])
        source = candidates[0]
        if total > 1:
            print candidates
            for filename in candidates:
                if "CurveFitting" in filename:
                    source = filename
                    break
    elif pyfile in python_files_bare:
        n = python_files_bare.index(pyfile, )
        source = python_files[n]
    return source

#======================================================================
def initialize_wiki(args):
    global site
    exec_path=sys.executable
    if sys.executable.endswith('python_d.exe'):
        msg="wiki_maker must be called with standard python.exe not the debug python_d.exe" + \
            "You can still use a debug build of Mantid with wiki_maker, the use of python_d is handled internally"
        raise RuntimeError(msg)

    import mwclient
    # Init site object
    print "Connecting to site mantidproject.org"
    site = mwclient.Site('www.mantidproject.org', path='/')
    # Login, if specified
    if hasattr(args, 'username'):
        print "Logging in as %s" % args.username
        site.login(args.username, args.password) 
    
    
#======================================================================
def flag_if_build_is_debug(mantidpath):
    """
    Check if the given build is a debug build of Mantid
    """
    global mantid_debug
    global python_d_exe
    if platform.system() != 'Windows':
        return

    kernel_path=os.path.join(mantidpath,"mantid","kernel")
    if os.path.exists(os.path.join(kernel_path,"_kernel_d.pyd")):
        mantid_debug=True
        #Try & find python_d exe to use.
        exec_path=sys.executable.replace(".exe","_d.exe")
        if os.path.exists(exec_path):
           python_d_exe = exec_path
        else:
            raise RuntimeError("No python_d.exe found next to python.exe at %s" %(sys.executable))
        
    elif os.path.exists(os.path.join(kernel_path,"_kernel.pyd")):
        mantid_debug=False
    else:
        raise RuntimeError("Unable to locate Mantid python libraries.")

#======================================================================
def get_algorithm_to_version_lookup():
    """
        Returns a dictionary of all algorithm names along with a list of their versions.
        If necessary done via a subprocess
    """
    global mantid_debug
    if not mantid_debug:
        lookup=do_get_algorithm_to_version_lookup()
    else:
        import subprocess
        marker='@@@@@@@@@@@WIKI_TOOLS_END_OF_FUNCTION_CALL_STDOUT@@@@@@@@@@@@@@'
        code="import %s;out=%s.do_get_algorithm_to_version_lookup(as_str=True);print '%s';print out" % (module_name,module_name,marker)
        sp_output = subprocess.check_output([python_d_exe,"-c",code])
        try:
            fn_prints,fn_retval = sp_output.split(marker)
            # Reconstruct dictionary
            lookup = {}
            all_lines = fn_retval.strip().splitlines()
            for line in all_lines:
                columns = line.split()
                # First is name remaining are version numbers
                versions = []
                for v in columns[1:]:
                    versions.append(int(v))
                lookup[columns[0]] = versions
        except ValueError:
            raise RuntimeError("Error in processing output from subprocess. Most likely a bug in wiki_tools")

    return lookup
    
def do_get_algorithm_to_version_lookup(as_str=False):
    """
        Returns a list of all algorithm names and versions as a dictionary
        @param as_str If True, then the dictionary is transformed to a string 
                      of one algorithm per line: "name 1 2 3 ..." with as many columns as versions afterward
    """
    import mantid
    from mantid.api import AlgorithmFactory
    algs = AlgorithmFactory.getRegisteredAlgorithms(True)
    if not as_str:
        return algs
        
    out=""
    for name, versions in algs.iteritems():
        out += name + " "
        for version in versions:
            out += str(version) + " "
        out += "\n"
    return out

#======================================================================
def get_all_algorithm_names():
    """Returns a list of all algorithm names"""
    alg_lookup = get_algorithm_to_version_lookup()
    return alg_lookup.keys() 

#======================================================================
def find_misnamed_algos():
    """Prints out algos that have mismatched class names and algorithm names"""
    algos = get_all_algorithms()
    print "\n--- The following algorithms have misnamed class files."
    print   "--- No file matching their algorithm name could be found.\n"
    for algo in algos:
        source = find_algo_file(algo, -1)
        if source=='':
            print "%s was NOT FOUND" % (algo, )
            
#======================================================================
def find_section_text(lines, section, go_to_end=False, section2=""):
    """ Search WIKI text to find a section text there """
    if len(lines) == 0:
        return ""
    n = 0
    for line in lines:
        line_mod = line.replace(" ", "")
        if line_mod.startswith("==%s" % section) \
            or (section2 != "" and line_mod.startswith("==%s" % section2)):
            # Section started
            n += 1
            doc = ""
            # collect the documents till next section or the end 
            newline = lines[n]
            while (go_to_end or not newline.strip().startswith('==')) \
                  and not newline.strip().startswith('[[Category'):
                doc += newline + '\n'
                n += 1
                if n < len(lines):
                    newline = lines[n]
                else:
                    break
            return doc
        n += 1
            
    return ""
    
    

#======================================================================
def find_property_doc(lines, propname):
    """ Search WIKI text to find a properties' documentation there """
    if len(lines) == 0:
        return ""
    n = 0
    for line in lines:
        if line.strip() == "|" + propname:
            doc = lines[n+4].strip()
            if len(doc)>1:
                doc = doc[1:]
            return doc
        n += 1
            
    return ""
    
#======================================================================
def validate_wiki(args, algos):
    """ Look for any missing wiki pages / inconsistencies """
    missing = []
    
    for algo in algos:
        print "\n================= %s ===================" % algo
        
        # Check wiki page and download contents
        page = site.Pages[algo]
        wiki = page.edit()
        lines = []
        if len(wiki) == 0:
            print "- Algorithm wiki page for %s is MISSING!" % algo
            missing.append(algo)
        else:
            lines = wiki.split('\n')
            
        source = find_algo_file(algo)
        if source=='':
            print "- Source file not found."

        # Validate the algorithm itself
        alg = mtd.createAlgorithm(algo)
        summary = alg._ProxyObject__obj.getWikiSummary()
        if len(summary) == 0: 
            print "- Summary is missing (in the code)."
            wikidoc = find_section_text(lines, "Summary", go_to_end=False, section2="")
            if args.show_missing: print wikidoc
            
#        # One-time code to add wiki desc to CPP file
#        desc = find_section_text(lines, "Description", True, "Introduction")
#        # Fallback to the code one
#        if len(desc) == 0: 
#            desc = alg._ProxyObject__obj.getWikiDescription()
#        add_wiki_description(algo, desc)
            
        props = alg._ProxyObject__obj.getProperties()
        for prop in props:
            if len(prop.documentation) == 0:
                print "- Property %s has no documentation/tooltip (in the code)." % prop.name,
                wikidoc = find_property_doc(lines, prop.name)
                if len(wikidoc) > 0:
                    print "Found in wiki"
                    if args.show_missing: print "   \"%s\"" % wikidoc
                else:
                    print "Not found in wiki."
            
    print "\n\n"
    print "Algorithms with missing wiki page:\n", " ".join(missing) 


#======================================================================
def find_orphan_wiki():
    """ Look for pages on the wiki that do NOT correspond to an algorithm """
    category = site.Pages['Category:Algorithms']
    algos = get_all_algorithms()
            
    for page in category:
        algo_name = page.name
        if not (algo_name.startswith("Category:")):
            if not (algo_name in algos) :
                print "Algorithm %s was not found." % algo_name

#======================================================================
def find_latest_alg_version(algo_name):
    """
        Calls into mantid to find the latest version of the given algorithm
        via subprocess call to mantid if necessary
        @param algo_name The name of the algorithm
    """
    if not mantid_debug:  
        return do_find_latest_alg_version(algo_name)
    else:
        import subprocess
        code="import %s;print %s.do_find_latest_alg_version('%s')" % (module_name,module_name,algo_name)
        output = subprocess.check_output([python_d_exe,"-c",code])
        # Output will contain possible warnings,notices etc. Just want the last character
        return int(output.strip()[-1])
    
def do_find_latest_alg_version(algo_name):
    """
        Calls into mantid to find the latest version of the given algorithm
        @param algo_name The name of the algorithm
    """
    import mantid
    from mantid.api import AlgorithmManager
    return AlgorithmManager.createUnmanaged(algo_name, noversion).version()
    
#======================================================================

def make_wiki(algo_name, version, latest_version):
    """ Return wiki text for a given algorithm via subprocess call to mantid
    if necessary, this allows it to work in debug on Windows.
    @param algo_name :: name of the algorithm (bare)
    @param version :: version requested
    @param latest_version :: the latest algorithm 
    """
    if not mantid_debug:  
        output=do_make_wiki(algo_name,version,latest_version)
    else:
        import subprocess
        marker='@@@@@@@@@@@WIKI_TOOLS_END_OF_FUNCTION_CALL_STDOUT@@@@@@@@@@@@@@'
        code="import %s;import mantid;out=%s.do_make_wiki('%s',%d,%d);print '%s';print out" % (module_name,module_name,algo_name,version,latest_version,marker)
        sp_output = subprocess.check_output([python_d_exe,"-c",code])
        try:
            fn_prints,fn_retval = sp_output.split(marker)
            output=fn_retval
        except ValueError:
            raise RuntimeError("Error in processing output from subprocess. Most likely a bug in wiki_tools")
    return output
    
#======================================================================
def do_make_wiki(algo_name, version, latest_version):
    """ Return wiki text for a given algorithm
    @param algo_name :: name of the algorithm (bare)
    @param version :: version requested
    @param latest_version :: the latest algorithm 
    """ 
    
    external_image = "http://download.mantidproject.org/algorithm_screenshots/ScreenShotImages/%s_dlg.png" % algo_name  
    out = "<anchor url='%s'><img width=400px align='right' src='%s' style='position:relative; z-index:1000; padding-left:5px;'></anchor>\n\n" % (external_image, external_image)  
    
    # Deprecated algorithms: Simply return the deprecation message
    print "Creating... ", algo_name, version
    import mantid
    from mantid.api import AlgorithmManager,DeprecatedAlgorithmChecker
    # Deprecated algorithms: Simply returnd the deprecation message
    print "Creating... ", algo_name, version
    deprec_check = DeprecatedAlgorithmChecker(algo_name,version)
    deprec = deprec_check.isDeprecated()
    if len(deprec) != 0:
        out += "== Deprecated ==\n\n"
        deprecstr = deprec
        deprecstr = deprecstr.replace(". Use ", ". Use [[")
        deprecstr = deprecstr.replace(" instead.", "]] instead.")
        out += deprecstr 
        out += "\n\n"
    
    alg = AlgorithmManager.createUnmanaged(algo_name, version)
    alg.initialize()
    
    if (latest_version > 1):
        out += "Note: This page refers to version %d of %s. \n\n"% (version, algo_name)
            
    
    out += "== Summary ==\n\n"
    out += alg.getWikiSummary().replace("\n", " ") + "\n\n"
    # Fetch the custom usage wiki section.
    include_signature, custom_usage = get_wiki_usage(algo_name, version)
    out += "\n\n== Usage ==\n\n"
    if include_signature:
        out += " " + create_function_signature(alg, algo_name) + "\n\n" 
    out += "<br clear=all>\n\n" 
    out += custom_usage

    # If there is an alias put it in
    alias = alg.alias().strip()
    if len(alias) > 0:
        out += ("== Alias ==\n\n")
        out += "This algorithm is also called '%s'\n\n" % alias

    # Table of properties
    out += "== Properties ==\n\n"
    
    out += """{| border="1" cellpadding="5" cellspacing="0" 
!Order\n!Name\n!Direction\n!Type\n!Default\n!Description
|-\n"""

    # Do all the properties
    props = alg.getProperties()
    propnum = 1
    last_group = ""
    for prop in props:
        group = prop.getGroup
        if (group != last_group):
            out += make_group_header_line(group)
            last_group = group
        out += make_property_table_line(propnum, prop)
        propnum += 1
        
        
    # Close the table
    out += "|}\n\n"


    out += "== Description ==\n"
    out += "\n"
    desc = ""
    try:
        desc = get_wiki_description(algo_name,version)
    except IndexError:
        pass
    if (desc == ""):
      out +=  missing_description + "\n"
      print "Warning: missing wiki description for %s! Placeholder inserted instead." % algo_name
    else:
      out += desc + "\n"
    out += "\n"
    out += "[[Category:Algorithms]]\n"
    
    # All other categories
    categories = alg.categories()
    for categ in categories:
        n = categ.find("\\")
        if (n>0):
            # Category is "first\second"
            first = categ[0:n]
            second = categ[n+1:]
            out += "[[Category:" + first + "]]\n"
            out += "[[Category:" + second + "]]\n"
        else:
            out += "[[Category:" + categ + "]]\n"

    # Point to the right source ffiles
    if version > 1:
        out +=  "{{AlgorithmLinks|%s%d}}\n" % (algo_name, version)
    else:
        out +=  "{{AlgorithmLinks|%s}}\n" % (algo_name)

    return out

#======================================================================
def make_property_table_line(propnum, p):
    """ Make one line of the property table
    
    Args:
        propnum :: number of the prop
        p :: Property object
    Returns:
        string to add to the wiki
    """    
    from mantid.api import IWorkspaceProperty
    
    out = ""
    # The property number
    out += "|" + str(propnum) + "\n"
    # Name of the property
    out += "|" + p.name + "\n"

    out += "|" + direction_string[p.direction] + "\n"
    # Type (as string) wrap an IWorkspaceProperty in a link.
    if isinstance(p, IWorkspaceProperty): 
        out += "|[[" + str(p.type) + "]]\n"
    else:
        out += "|" + str(p.type) + "\n"
       
    if (direction_string[p.direction] == OutputDirection) and (not isinstance(p, IWorkspaceProperty)):
      out += "|\n" # Nothing to show under the default section for an output properties that are not workspace properties.
    elif (p.isValid == ""): #Nothing was set, but it's still valid = NOT  mandatory
      defaultstr = create_property_default_string(p)
      out += "| " + defaultstr + "\n"
    else:
      out += "|Mandatory\n"
      
    # Documentation
    out += "|" + p.documentation.replace("\n", "<br />") + "\n"
    # End of table line
    out += "|-\n"
    return out
    

#======================================================================
def make_group_header_line(group):
    """ Make a group header line for the property table
    
     Args:
        group :: name of the group
    Returns:
        string to add to the wiki
    """
    if group=="":
        return "|colspan=6 align=center|   \n|-\n"
    else:
        return "|colspan=6 align=center|'''%s'''\n|-\n" % group

#======================================================================  
def create_property_default_string(prop):
    """ Create a default string 
    
     Args:
        default. The property default value.
    Returns:
        string to add to the wiki property table default section.
    """
    # Convert to int, then float, then any string
    
    default = prop.getDefault
    defaultstr = ""
    try:
        val = int(default)
        if (val >= 2147483647):
            defaultstr = "Optional"
        else:
            defaultstr = str(val)
    except:
        try:
            val = float(default)
            if (val >= 1e+307):
                defaultstr = "Optional"
            else:
                defaultstr = str(val)
        except:
            # Fall-back default for anything
            defaultstr = str(default)
            
    # Replace the ugly default values with "optional"
    if (defaultstr == "8.9884656743115785e+307") or \
       (defaultstr == "1.7976931348623157e+308") or \
       (defaultstr == "2147483647"):
        defaultstr = "Optional"
        
    if str(prop.type) == "boolean":
        if defaultstr == "1": defaultstr = "True" 
        else: defaultstr = "False"
    return defaultstr
