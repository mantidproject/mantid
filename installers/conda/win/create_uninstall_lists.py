# Generate files containing uninstaller commands to be included in the Windows NSIS script.
# Creates Delete commands for all files contained within the package directory, and a RMDir command for all directories.
# Two arguments are required:
# --package-dir is the location of the directory containing all files to be installed/uninstalled by NSIS
# --output-dir is where the output files (uninstall_files.nsh and uninstall_dirs.nsh) will be placed.

import os


def write_uninstall_file_commands(package_dir, file_name):
    with open(file_name, 'w') as output_file:
        for path, subdirs, files in os.walk(package_dir):
            for name in files:
                delete_command = f'Delete "{os.path.join(path, name)}"'
                delete_command = delete_command.replace(package_dir, r"$INSTDIR")
                output_file.write(delete_command + "\n")


def write_uninstall_dirs_commands(package_dir, file_name):
    with open(file_name, 'w') as output_file:
        # Walk backwards over directories so that we can delete the inner directories first
        for path, subdirs, _ in sorted(os.walk(package_dir), reverse=True):
            for name in subdirs:
                delete_command = f'RMDir "{os.path.join(path, name)}"'
                delete_command = delete_command.replace(package_dir, r"$INSTDIR")
                output_file.write(delete_command + "\n")


if __name__ == '__main__':
    from argparse import ArgumentParser
    parser = ArgumentParser(description="Generate uninstaller delete commands for NSIS script")
    parser.add_argument('--package_dir', required=True)
    parser.add_argument('--output_dir', required=True)
    args = parser.parse_args()

    output_file_name = args.output_dir + r'\uninstall_files.nsh'
    write_uninstall_file_commands(args.package_dir, output_file_name)
    output_file_name_dirs = args.output_dir + r'\uninstall_dirs.nsh'
    write_uninstall_dirs_commands(args.package_dir, output_file_name_dirs)
