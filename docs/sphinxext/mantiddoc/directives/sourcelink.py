# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
import mantid
from .base import AlgorithmBaseDirective
from pathlib import Path, PurePosixPath


class SourceLinkError(Exception):
    def __init__(self, value):
        self.value = value

    def __str__(self):
        return str(self.value)


class SourceLinkDirective(AlgorithmBaseDirective):
    """
    Obtains the github links to the .cpp and .h or .py files depending on the
    name and version.

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
      :filename: FindEPP
      :py: None
    """

    required_arguments, optional_arguments = 0, 0
    option_spec = {"filename": str, "sanity_checks": int, "cpp": str, "h": str, "py": str}

    # IMPORTANT: keys must match the option spec above
    # - apart from filename and sanity_checks
    file_types = {"h": "C++ header", "cpp": "C++ source", "py": "Python"}
    file_lookup = {}

    git_cache = {}

    # will be filled in
    __source_root = None

    def execute(self):
        """
        Called by Sphinx when the .. sourcelink:: directive is encountered.
        """
        file_paths = {}
        error_string = ""
        sanity_checks = self.options.get("sanity_checks", 1)
        file_name = self.options.get("filename", None)

        if file_name is None:
            # build a sensible default
            file_name = self.algorithm_name()
            if (self.algorithm_version() != 1) and (self.algorithm_version() is not None):
                file_name += str(self.algorithm_version())

        for extension in self.file_types.keys():
            file_paths[extension] = self.options.get(extension, None)
            if file_paths[extension] is None:
                try:
                    fname = self.find_source_file(file_name, extension)
                    file_paths[extension] = fname if fname is not None else None
                except SourceLinkError as err:
                    error_string += str(err) + "\n"
            elif file_paths[extension].lower() == "none":
                # the users has specifically chosen to suppress this - set it to a "proper" None
                # but do not search for this file
                file_paths[extension] = None
            else:
                # prepend the base framework directory
                fname = self.source_root.joinpath(file_paths[extension])
                file_paths[extension] = fname
                if not os.path.exists(file_paths[extension]):
                    error_string += "Cannot find {} file at {}\n".format(extension, file_paths[extension])

        # throw accumulated errors now if you have any
        if error_string != "":
            raise SourceLinkError(error_string)

        self.output_to_page(file_paths, file_name, sanity_checks)

        return []

    def find_source_file(self, file_name, extension):
        """
        Searches the source code for a matching filename with the correct extension
        """
        # parse the source tree if it has not already been done
        if not self.file_lookup:
            self.parse_source_tree()

        try:
            path_list = self.file_lookup[file_name][extension]
            if len(path_list) == 1:
                return path_list[0]
            else:
                suggested_path = "os_agnostic_path_to_file_from_source_root"
                if len(path_list) > 1:
                    suggested_path = path_list[0].relative_to(self.source_root)
                raise SourceLinkError(
                    "Found multiple possibilities for "
                    + file_name
                    + "."
                    + extension
                    + "\n"
                    + "Possible matches"
                    + str(path_list)
                    + "\n"
                    + "Specify one using the "
                    + extension
                    + " option\n"
                    + "e.g. \n"
                    + ".. sourcelink:\n"
                    + "      :"
                    + extension
                    + ": "
                    + suggested_path
                )

            return self.file_lookup[file_name][extension]
        except KeyError:
            # value is not present
            return None

    @property
    def source_root(self):
        """
        returns the root source directory
        """
        if self.__source_root is None:
            env = self.state.document.settings.env
            # assume root is two levels up
            direc = Path(env.srcdir, "..", "..").resolve()  # = C:\Mantid\Code\Mantid\docs\source
            self.__source_root = PurePosixPath(direc)  # pylint: disable=protected-access

        return self.__source_root

    def parse_source_tree(self):
        """
        Fills the file_lookup dictionary after parsing the source code
        """
        env = self.state.document.settings.env
        builddir = Path(env.doctreedir, "..", "..")  # there should be a better setting option
        builddir = builddir.resolve()

        for dir_name, _, file_list in os.walk(self.source_root):
            if builddir in Path(dir_name).resolve().parents:
                continue  # don't check or add to the cache
            for fname in file_list:
                (base_name, file_extensions) = os.path.splitext(fname)
                # strip the dot from the extension
                file_extensions = file_extensions[1:]
                # build the data object that is e.g.
                # file_lookup["Rebin2"]["cpp"] = ['C:\Mantid\Code\Mantid\Framework\Algorithms\src\Rebin2.cpp','possible other location']
                if file_extensions in self.file_types.keys():
                    if base_name not in self.file_lookup.keys():
                        self.file_lookup[base_name] = {}
                    if file_extensions not in self.file_lookup[base_name].keys():
                        self.file_lookup[base_name][file_extensions] = []
                    self.file_lookup[base_name][file_extensions].append(PurePosixPath(dir_name, fname))

    def output_to_page(self, file_paths, file_name, sanity_checks):
        """
        Outputs the sourcelinks and heading to the rst page
        and performs some sanity checks
        """
        valid_ext_list = []

        self.add_rst(self.make_header("Source"))
        for extension, filepath in file_paths.items():
            if filepath is not None:
                self.output_path_to_page(filepath, extension)
                valid_ext_list.append(extension)

        # do some sanity checks - unless suppressed
        if sanity_checks > 0:
            suggested_path = "os_agnostic_path_to_file_from_Code/Mantid"
            if not valid_ext_list:
                raise SourceLinkError(
                    "No file possibilities for "
                    + file_name
                    + " have been found\n"
                    + "Please specify a better one using the :filename: option or use the "
                    + str(list(self.file_types.keys()))
                    + " options\n"
                    + "e.g. \n"
                    + ".. sourcelink:\n"
                    + "      :"
                    + list(self.file_types.keys())[0]
                    + ": "
                    + suggested_path
                    + "\n "
                    + "or \n"
                    + ".. sourcelink:\n"
                    + "      :filename: "
                    + file_name
                )

            # if the have a cpp we should also have a h
            if ("cpp" in valid_ext_list) ^ ("h" in valid_ext_list):
                raise SourceLinkError(
                    "Only one of .h and .cpp found for "
                    + file_name
                    + "\n"
                    + "valid files found for "
                    + str(valid_ext_list)
                    + "\n"
                    + "Please specify the missing one using an "
                    + str(list(self.file_types.keys()))
                    + " option\n"
                    + "e.g. \n"
                    + ".. sourcelink:\n"
                    + "      :"
                    + list(self.file_types.keys())[0]
                    + ": "
                    + suggested_path
                )

    def output_path_to_page(self, filepath, extension):
        """
        Outputs the source link for a file to the rst page
        """
        self.add_rst("{}: `{} <{}>`_\n\n".format(self.file_types[extension], str(filepath.name), self.convert_path_to_github_url(filepath)))

    def convert_path_to_github_url(self, file_path):
        """
        Converts a file path to the github url for that same file

        example path Framework/Algorithms/inc/MantidAlgorithms/MergeRuns.h
        example url  https://github.com/mantidproject/mantid/blob/master/Framework/Algorithms/inc/MantidAlgorithms/MergeRuns.h
        """
        url = str(file_path)
        # remove the directory path
        url = url.replace(str(self.source_root), "")
        # harmonize slashes
        url = url.replace("\\", "/")
        # prepend the github part
        if not url.startswith("/"):
            url = "/" + url
        url = "https://github.com/mantidproject/mantid/blob/" + mantid.kernel.revision_full() + url
        return url


def setup(app):
    """
    Setup the directives when the extension is activated

    Args:
      app: The main Sphinx application object
    """
    app.add_directive("sourcelink", SourceLinkDirective)
