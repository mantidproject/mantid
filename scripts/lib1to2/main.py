"""
Entry point for the 1->2 migration script
"""
import messages
import migrate
import gui

import optparse

def main(args):
    """
    Starts the migration process

    @param argv The arguments passed to the script
    """
    parser = optparse.OptionParser(usage="migrate1to2 [options] file|dir")
    parser.add_option("-n", "--nobackups", action="store_true", default=False,
                      help="Don't write backups for modified files. WARNING: This operation cannot be undone, use with care")
    parser.add_option("-g", "--gui", action="store_true", default=False,
                      help="Starts the GUI")

    options, args = parser.parse_args(args)
    backup = not options.nobackups
    files = get_files(args[0])

    if options.gui:
        retcode = gui.start(args, options)
    else:
        retcode = migrate.run(files, backup)

    # Any clean up code
    return retcode

def get_files(path):
    """
    Returns the list of files to work on
    @param path :: A string interpreted as a file or directory
    """
    import os
    if not type(path) == str:
        raise ValueError("Expected string argument specfying file or path. Found %s" % type(path))

    py_files = []
    if os.path.isfile(path):
        py_files.append(path)
    else:
        for root, dirs, files in os.walk(path):
            for filename in files:
                if filename.endswith(".py"):
                    py_files.append(os.path.join(root,filename))
    return py_files
