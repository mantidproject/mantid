# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +


from argparse import ArgumentParser
from release_editor import fix_release_name, create_file_location, gather_release_note_directories


def parse_args():
    parser = ArgumentParser(description="Locate unused release notes")
    parser.add_argument("--release", required=True)
    return parser.parse_args()


def check_for_unused_files(dirs):
    unused_dirs = []
    for p in dirs:
        if p.name != "Used":
            unused_dirs.append(p)
    return unused_dirs


def find_files(dirs):
    file_paths = []
    for p in dirs:
        file_paths += list(p.glob("*.rst"))
    return file_paths


def print_paths(dirs):
    if not dirs:
        print("No unused release notes!")
    else:
        paths = find_files(dirs)
        print(f"{len(paths)} unused release notes found!")
        for p in paths:
            print(str(p))


if __name__ == "__main__":
    args = parse_args()
    args.release = fix_release_name(args.release)
    main_release_dir = create_file_location(args.release)
    release_note_dirs = gather_release_note_directories(main_release_dir)
    unused_note_dirs = check_for_unused_files(release_note_dirs)
    print_paths(unused_note_dirs)
