from base import AlgorithmBaseDirective
from os import path, walk
import mantid

class SourceLinkError(Exception):
    def __init__(self, value):
        self.value = value
    def __str__(self):
        return str(self.value)
        
class SourceLinkDirective(AlgorithmBaseDirective):
    """
    Obtains the github links to the .cpp and .h or .py files depending on the name and version.
    
    Example directive usage:
    
    Default Usage:
    .. sourcelink::
    
    Overriding the filename for searching:
    .. sourcelink::
        :filename: SINQTranspose3D
        
    suppressing sanity checks:
    .. sourcelink::
        :sanity_checks: 0    
        
    specifying specific files 
    (paths should use / and start from but not include the Mantid directory):
    .. sourcelink::
         :h: Framework/ICat/inc/MantidICat/CatalogSearch.h
         :cpp: Framework/ICat/src/CatalogSearch.cpp
         
    suppressing a specific file type (None is case insensitive):
    .. sourcelink::
      :filename: FilterEventsByLogValuePreNexus
      :py: None
    """

    required_arguments, optional_arguments = 0, 0
    option_spec = {"filename":str,"sanity_checks":int,"cpp":str, "h":str, "py":str}

    #IMPORTANT: keys must match the option spec above 
    # - apart from filename and sanity_checks
    file_types = {"h":"C++ header", "cpp":"C++ source","py":"Python"}
    file_lookup = {} 
    
    mantid_directory_cache=None

    def execute(self):
        """
        Called by Sphinx when the ..sourcelink:: directive is encountered.
        """
        file_paths = {}
        error_string = ""
        sanity_checks = self.options.get("sanity_checks", 1)
        file_name = self.options.get("filename", None)
        if file_name is None:
            #build a sensible default
            file_name = self.algorithm_name()
            if (self.algorithm_version() != 1) and (self.algorithm_version() is not None):
                file_name += str(self.algorithm_version())
                
        
        for extension in self.file_types.keys():
            file_paths[extension] = self.options.get(extension, None)
            if file_paths[extension] is None:
                try:
                    file_paths[extension] = self.find_source_file(file_name,extension)
                except SourceLinkError as err:
                    error_string += str(err) + "\n"
            elif file_paths[extension].lower() == "none":
                # the users has specifically chosen to suppress this - set it to a "proper" None
                #but do not search for this file
                file_paths[extension] = None
            else:
                #prepend the base framework directory
                file_paths[extension] = path.join(self.get_mantid_directory(),file_paths[extension])
                if not path.exists(file_paths[extension]):
                    error_string +="Cannot find " + extension + " file at " + file_paths[extension] + "\n"

        #throw accumulated errors now if you have any
        if error_string != "":
            raise SourceLinkError(error_string)                
                    
        try:
            self.output_to_page(file_paths,file_name,sanity_checks);
        except SourceLinkError as err:
            error_string += str(err) + "\n"
                    
        if error_string != "":
            raise SourceLinkError(error_string)                

        return []

    def find_source_file(self, file_name, extension):
        """
        Searches the source code for a matching filename with the correct extension
        """
        # parse the source tree if it has not already been done
        if len(self.file_lookup) == 0:
            self.parse_source_tree(file_name,extension)
            
        try:
            path_list = self.file_lookup[file_name][extension]
            if len(path_list) == 1:
                return path_list[0]
            else:
                suggested_path = "os_agnostic_path_to_file_from_Code/Mantid"
                if len(path_list) > 1:
                    suggested_path = path_list[0]
                    #harmonize slashes
                    suggested_path = suggested_path.replace("\\","/")
                    #remove everything before and including the Mantid directory
                    strip_off_token = "Code/Mantid/"
                    index = suggested_path.find(strip_off_token)
                    if index != -1:
                        suggested_path = suggested_path[index+len(strip_off_token):]
                raise SourceLinkError("Found multiple possibilities for " + file_name + "." + extension + "\n" + 
                "Possible matches" +  str(path_list) + "\n" + 
                "Specify one using the " + extension + " option\n" + 
                "e.g. \n" + 
                ".. sourcelink:\n" + 
                "      :" + extension + ": " + suggested_path)
                
            return self.file_lookup[file_name][extension]
        except KeyError:
            #value is not present
            return None
            
    def get_mantid_directory(self):
        """
        returns the Code\Mantid directory
        """
        if self.mantid_directory_cache is None:
            env = self.state.document.settings.env
            dir = env.srcdir #= C:\Mantid\Code\Mantid\docs\source
            #go up the path until dir point to the "Mantid" directory
            (head,tail) = path.split(dir)
            while tail != "Mantid":
                dir = head
                (head,tail) = path.split(head)
                if (head == ""):
                    raise IOError ("Could not find the 'Mantid' directory in " + env.srcdir)

            self.mantid_directory_cache = dir
        return self.mantid_directory_cache
    
    def parse_source_tree(self, file_name, extension):
        """
        Fills the file_lookup dictionary after parsing the source code
        """        
        for dirName, subdirList, fileList in walk(self.get_mantid_directory()):
            for fname in fileList:
                (baseName, fileExtension) = path.splitext(fname)            
                #strip the dot from the extension
                fileExtension = fileExtension[1:] 
                #build the data object that is e.g.
                #file_lookup["Rebin2"]["cpp"] = ['C:\Mantid\Code\Mantid\Framework\Algorithms\src\Rebin2.cpp','possible other location']
                if fileExtension in self.file_types.keys():
                    if baseName not in self.file_lookup.keys():
                        self.file_lookup[baseName] = {}
                    if fileExtension not in self.file_lookup[baseName].keys():
                        self.file_lookup[baseName][fileExtension] = []
                    self.file_lookup[baseName][fileExtension].append(path.join(dirName,fname))
        return
        
    def output_to_page(self, file_paths,file_name,sanity_checks):
        """
        Outputs the sourcelinks and heading to the rst page
        and performs some sanity checks
        """
        valid_ext_list = []
        
        self.add_rst(self.make_header("Source"))
        for extension, filepath in file_paths.iteritems():
            if filepath is not None:
                self.output_path_to_page(filepath,extension)
                valid_ext_list.append(extension)
                
                    
        #do some sanity checks - unless suppressed
        if sanity_checks > 0:
            suggested_path = "os_agnostic_path_to_file_from_Code/Mantid"
            if len(valid_ext_list) == 0:
                raise SourceLinkError("No file possibilities for " + file_name + " have been found\n" + 
                    "Please specify a better one using the :filename: opiton or use the " + str(self.file_types.keys()) + " options\n" + 
                    "e.g. \n" + 
                    ".. sourcelink:\n" + 
                    "      :" + self.file_types.keys()[0] + ": " + suggested_path + "\n "+
                    "or \n" + 
                    ".. sourcelink:\n" + 
                    "      :filename: " + file_name) 
            #if the have a cpp we should also have a h
            if ("cpp" in valid_ext_list) or ("h" in valid_ext_list):
                if ("cpp" not in valid_ext_list) or ("h" not in valid_ext_list):
                    raise SourceLinkError("Only one of .h and .cpp found for " + file_name + "\n" + 
                    "valid files found for " + str(valid_ext_list) + "\n" + 
                    "Please specify the missing one using an " + str(self.file_types.keys()) + " option\n" + 
                    "e.g. \n" + 
                    ".. sourcelink:\n" + 
                    "      :" + self.file_types.keys()[0] + ": " + suggested_path) 
        return
        
    def output_path_to_page(self, filepath, extension):
        """
        Outputs the source link for a file to the rst page
        """
        dirName,fName = path.split(filepath)
        self.add_rst(self.file_types[extension] + ": `" + fName + " <" + self.convert_path_to_github_url(filepath) + ">`_\n\n")
        return
        
        
    def convert_path_to_github_url(self, file_path):
        """
        Converts a file path to the github url for that same file
        """
        #example path C:\Mantid\Code\Mantid/Framework/Algorithms/inc/MantidAlgorithms/MergeRuns.h
        #example url  https://github.com/mantidproject/mantid/blob/master/Code/Mantid/Framework/Algorithms/inc/MantidAlgorithms/MergeRuns.h
        
        url = file_path
        #harmonize slashes
        url = url.replace("\\","/")
        #remove everything before Code
        index = url.find("Code")
        if index != -1:
            url = url[index:]
        else:
            raise SourceLinkError ("Could not find the 'Code' directory in " + url)
        #prepend the github part
        url = "https://github.com/mantidproject/mantid/blob/" + mantid.kernel.revision_full() + "/" + url
        return url
    
def setup(app):
    """
    Setup the directives when the extension is activated

    Args:
      app: The main Sphinx application object
    """
    app.add_directive('sourcelink', SourceLinkDirective)
