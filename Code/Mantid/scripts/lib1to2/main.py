"""
Entry point for the 1->2 migration script
"""
import messages
import migrate
import gui

import optparse


def main(args):
    """Starts the migration process
    
        @param argv The arguments passed to the script
    """
    parser = optparse.OptionParser(usage="migrate1to2 [options] file|dir")
    parser.add_option("-n", "--nobackups", action="store_true", default=False,
                      help="Don't write backups for modified files. WARNING: This operation cannot be undone, use with care")
    parser.add_option("-g", "--gui", action="store_true", default=False,
                      help="Starts the GUI")
                      
    options, args = parser.parse_args(args)
    backup = not options.nobackups
    if options.gui:
        retcode = gui.start(args, options)
    else:
        retcode = migrate.run(args, backup)
        
    # Any clean up code
    return retcode