# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import os
import pathlib
import subprocess
import requests

from typing import List, Tuple


DIRECTIVE = ".. amalgamate:: "
GIT_REPO = "mantidproject/mantid"


def get_release_root() -> pathlib.Path:
    program_path = pathlib.Path(__file__).resolve()
    script_dir = program_path / "../../../docs/source/release/"
    return script_dir.resolve()


def fix_release_name(name: str) -> str:
    if not name.startswith("v"):
        name = "v" + name
    return name


def create_file_location(release_version: str) -> pathlib.Path:
    return get_release_root() / release_version


def update_file(file: pathlib.Path, release_note_text: str, amalgamate_line: str) -> None:
    # opens the upper level release note e.g. diffraction.rst
    with open(file, mode="r") as f:
        rst_text = f.read()

    # replaces the amalgamate directive with the notes compiled for that heading
    text_with_replacements = rst_text.replace(amalgamate_line, release_note_text)

    with open(file, mode="w") as f:
        f.write(text_with_replacements)


def create_release_note_directory_path(amalgamate_line: str, amalgamate_directive: str, release_notes_root: pathlib.Path) -> pathlib.Path:
    release_note_dir = amalgamate_line.replace(amalgamate_directive, "").strip()
    return release_notes_root / release_note_dir


def add_release_notes_to_main_pages(release_notes_root: pathlib.Path, git_token: str) -> None:
    # iterates through files in a directory
    for file in release_notes_root.glob("*.rst"):
        with open(file) as f:
            # iterate through each line in the upper level release note file e.g. diffraction.rst
            for line in f:
                # finds the amalgamate directive to replace
                if line.startswith(DIRECTIVE):
                    release_note_sub_dir = create_release_note_directory_path(line, DIRECTIVE, release_notes_root)
                    release_notes_text = collate_release_notes(release_note_sub_dir, git_token)
                    update_file(release_notes_root / file, release_notes_text, line)


def append_release_note_directories(path: pathlib.Path, directory_list: List[pathlib.Path]):
    if not os.path.isdir(path):
        return

    # add dir to directorylist if it contains .rst files
    if len([f for f in os.listdir(path) if f.endswith(".rst")]) > 0:
        directory_list.append(path)

    for d in os.listdir(path):
        append_release_note_directories(path / d, directory_list)


def gather_release_note_directories(path: pathlib.Path) -> List[pathlib.Path]:
    release_note_directories = []
    append_release_note_directories(path, release_note_directories)
    release_note_directories.pop(0)
    return release_note_directories


def get_pr_number_and_link(release_note_path: pathlib.Path, git_token: str) -> Tuple[str, str]:
    commit_hashes = get_file_creation_commits(release_note_path)
    for commit_hash in commit_hashes:
        headers = {"Authorization": f"token {git_token}", "Accept": "application/vnd.github.groot-preview+json"}
        url = f"https://api.github.com/repos/{GIT_REPO}/commits/{commit_hash}/pulls"
        response = requests.get(url, headers=headers, timeout=10)
        if response.status_code != 200:
            continue

        pr = response.json()[0]
        if pr["base"]["ref"] == "main":
            return pr["number"], pr["html_url"]

    return "", ""


def get_file_creation_commits(release_note_path: pathlib.Path) -> List[str]:
    result = subprocess.run(
        ["git", "log", "--diff-filter=A", "--pretty=format:%H", "--", release_note_path], capture_output=True, text=True
    )
    return result.stdout.split("\n")


# This is the method for iterating through release notes in a folder and collating the notes into one object
def collate_release_notes(release_note_dir: pathlib.Path, git_token: str) -> str:
    combined_notes = ""
    for release_note_file in release_note_dir.glob("*.rst"):
        pr_number, pr_link = get_pr_number_and_link(release_note_file, git_token)
        pr_text = ""
        if pr_number and pr_link:
            pr_text = f"(`#{pr_number} <{pr_link}>`_)"
        with open(release_note_file) as f:
            for line in f:
                if line.startswith("- "):
                    line = f"- {pr_text} {line[2:]}"
                combined_notes += line

    return combined_notes


# Moves the release note files that have been copied to top level file into 'Used' folders
def move_files_to_used(release_note_directories: List[pathlib.Path]) -> None:
    for path in release_note_directories:
        used_dir = path / "Used"
        used_dir.mkdir(parents=True, exist_ok=True)
        for file in path.glob("*.rst"):
            file.rename(used_dir / file.name)


if __name__ == "__main__":
    from argparse import ArgumentParser

    parser = ArgumentParser(description="Generate generic release pages")
    parser.add_argument("--release", required=True)
    parser.add_argument("--git_token", required=True)
    args = parser.parse_args()

    release_notes_root_dir = create_file_location(fix_release_name(args.release))
    directories_with_release_notes = gather_release_note_directories(release_notes_root_dir)

    add_release_notes_to_main_pages(release_notes_root_dir, args.git_token)
    move_files_to_used(directories_with_release_notes)
