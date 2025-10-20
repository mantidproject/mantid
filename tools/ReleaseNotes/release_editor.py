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


DIRECTIVE = ".. amalgamate:: "
GIT_REPO = "mantidproject/mantid"


def getReleaseRoot() -> pathlib.Path:
    program_path = pathlib.Path(__file__).resolve()
    script_dir = program_path / "../../../docs/source/release/"
    return script_dir.resolve()


def fixReleaseName(name):
    if not name.startswith("v"):
        name = "v" + name
    return name


def createFileLocation(releaseNo) -> pathlib.Path:
    return getReleaseRoot() / releaseNo


def updateFile(originalFile, replaceText, textToReplace):
    # opens the upper level release note e.g. diffraction.rst
    with open(originalFile, mode="r") as mainRST:
        rstToRead = mainRST.read()
        mainRST.close()

    # replaces the amalgamate directive with the notes compiled for that heading
    newdata = rstToRead.replace(textToReplace, replaceText)

    with open(originalFile, mode="w") as mainRST:
        mainRST.write(newdata)
        mainRST.close()


def createFileName(fileName, pathDirectory, includeStatement) -> pathlib.Path:
    newFileName = fileName.replace(includeStatement, "")
    newFileName = newFileName.replace("\n", "")
    newFileName = pathDirectory / newFileName
    return newFileName


def addReleaseNotesToMain(pathDirectory, git_token):
    # iterates through files in a directory
    for file in pathDirectory.glob("*.rst"):
        with open(file) as fileToEdit:
            # iterate through each line in the upper level release note file e.g. diffraction.rst
            for line in fileToEdit:
                # finds the amalgamate directive to replace
                if line.startswith(DIRECTIVE):
                    fileName = createFileName(line, pathDirectory, DIRECTIVE)
                    releaseNotes = collateNotes(fileName, git_token)
                    originalFile = mainDirectory / file
                    updateFile(originalFile, releaseNotes, line)


def getReleaseNoteDirectories(path, directoryList):
    # return nothing if path is a file
    if os.path.isfile(path):
        return ()

    # add dir to directorylist if it contains .rst files
    if len([f for f in os.listdir(path) if f.endswith(".rst")]) > 0:
        path = pathlib.Path(path)
        directoryList.append(path)

    for d in os.listdir(path):
        new_path = os.path.join(path, d)
        if os.path.isdir(new_path):
            getReleaseNoteDirectories(new_path, directoryList)

    return directoryList


def checkContainsReleaseNote(path):
    releaseNoteList = []
    getReleaseNoteDirectories(path, releaseNoteList)
    releaseNoteList.pop(0)
    return releaseNoteList


def get_pr_number_and_link(release_note_path, git_token):
    commit_hash = get_file_creation_commit(release_note_path)
    headers = {"Authorization": f"token {git_token}", "Accept": "application/vnd.github.groot-preview+json"}
    url = f"https://api.github.com/repos/{GIT_REPO}/commits/{commit_hash}/pulls"
    response = requests.get(url, headers=headers)
    if response.status_code == 200:
        pulls = response.json()
        if pulls:
            pr = pulls[0]
            return pr["number"], pr["html_url"]

    return "", ""


def get_file_creation_commit(release_note_path):
    result = subprocess.run(
        ["git", "log", "--diff-filter=A", "--pretty=format:%H", "--", release_note_path], capture_output=True, text=True
    )

    return result.stdout.split("\n")[-1].strip()


# This is the method for iterating through release notes in a folder and collating the notes into one object
def collateNotes(path, git_token):
    combinedRst = ""
    for file in path.glob("*.rst"):
        pr_number, pr_link = get_pr_number_and_link(file, git_token)
        pr_text = ""
        if pr_number and pr_link:
            pr_text = f"(`#{pr_number} <{pr_link}>`_)"
        with open(file) as f:
            for line in f:
                if line.startswith("- "):
                    line = f"- {pr_text} {line[2:]}"
                combinedRst += line

    return combinedRst


# Moves the release note files that have been copied to top level file into 'Used' folders
def moveFiles(listOfDirectories):
    for path in listOfDirectories:
        usedDir = path / "Used"
        usedDir.mkdir(parents=True, exist_ok=True)
        for file in path.glob("*.rst"):
            used_folder = "Used"
            file.rename(path / used_folder / file.name)


if __name__ == "__main__":
    from argparse import ArgumentParser

    parser = ArgumentParser(description="Generate generic release pages")
    parser.add_argument("--release", required=True)
    parser.add_argument("--git_token", required=True)
    args = parser.parse_args()

    args.release = fixReleaseName(args.release)
    mainDirectory = createFileLocation(args.release)
    directoriesUsed = checkContainsReleaseNote(mainDirectory)

    addReleaseNotesToMain(mainDirectory, args.git_token)
    moveFiles(directoriesUsed)
