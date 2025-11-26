# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
from itertools import filterfalse
import os
import re
import subprocess

# The structure of a release tag
RELEASE_TAG_RE = re.compile(r"^v?\d+.\d+(.\d)?$")

# Used to ensure a Git author does not appear in any of the DOIs.  This is NOT
# to be used in the case where a Git user has multiple accounts; a translation
# entry would suffice in such an instance.
_blocklist = [
    "unknown",
    # people
    "Utkarsh Ayachit",
    "Thomas Brooks",
    "Chris Kerr",
    "Erik B Knudsen",
    "Bartomeu Llopis",
    "Thomas Mueller",
    "Daniel Pajerowski",
    "Luz Paz",
    "David Voneshen",
    "Marie Yao",
]

# The allowlist is used for sponsors / contributors who should be included,
# but who are not listed as authors on Git.  These names will be shown in the
# "main" DOI only.
allowlist = [
    "Cottrell, Stephen",
    "Dillow, David",
    "Hagen, Mark",
    "Hillier, Adrian",
    "Heller, William",
    "Howells, Spencer",
    "McGreevy, Robert",
    "Pascal, Manuel",
    "Perring, Toby",
    "Pratt, Francis",
    "Proffen, Thomas",
    "Radaelli, Paolo",
    "Taylor, Jon",
    "Granroth, Garrett",
]


def run_from_script_dir(func):
    """Decorator that changes the working directory to the directory of this
    script for the duration of the decorated function.  Basically it means we
    can be sure that calls to "git tag" and "git log" still work, even if this
    script is called from outside the Git tree.
    """

    def change_dir_wrapper(*args, **kwargs):
        cwd = os.getcwd()
        os.chdir(os.path.dirname(os.path.abspath(__file__)))
        result = func(*args, **kwargs)
        os.chdir(cwd)
        return result

    return change_dir_wrapper


@run_from_script_dir
def _get_all_release_git_tags():
    """
    Returns a list of all the tags in the tree.
    """
    proc = subprocess.run(["git", "tag", "--sort=version:refname"], stdout=subprocess.PIPE, encoding="utf-8")
    all_tags = proc.stdout.replace('"', "").split("\n")
    return list(filter(lambda tag: RELEASE_TAG_RE.match(tag) is not None, all_tags))


def _clean_up_author_list(author_list):
    """Apply translations and blocklist, and get rid of duplicates."""
    # Get rid of count by splitting on tab - remove whitespace
    result = [author.split("\t")[-1].strip() for author in author_list]
    # Get rid of email adddress - bot addresses are <bot> in gitmailmap
    result = [author.split("<")[0].strip() for author in result if "<bot>" not in author]
    # Remove empty authors
    result = [author for author in result if bool(author)]

    # Remove any names that are on the blocklist.
    result = set(filterfalse(_blocklist.__contains__, result))

    # Return the unique list of translated names sorted by last name
    return sorted(set(result), key=lambda value: value.split()[-1])


@run_from_script_dir
def _authors_from_tag_info(tag_info):
    """Given some tag/commit information, will return the corresponding Git
    authors.

    This assumes that you have the `.gitmailmap` file and it is configured correctly.
    Contact members of the TWG for the file.
    """
    # -n sort by number of commits
    # -s suppress commit descriptions
    # -e show email
    # --group=author show counts by author rather than committer
    # --no-merges ignore merge commits
    args = ["git", "shortlog", "-nse", "--group=author", "--no-merges", tag_info]
    proc = subprocess.run(args, stdout=subprocess.PIPE, encoding="utf-8")
    authors = proc.stdout.replace('"', "").split("\n")
    return _clean_up_author_list(authors)


def find_tag(version_str):
    """Return the Git tag, if it actually exists, for a given version"."""
    tag_title = "v" + version_str
    if tag_title in _get_all_release_git_tags():
        return tag_title
    else:
        raise RuntimeError("Cannot find expected git tag '{0}'".format(tag_title))


def get_previous_tag(tag):
    """Given an existing git tag, will return the tag that is found before it."""
    all_tags = _get_all_release_git_tags()
    if tag not in all_tags:
        return None
    return all_tags[all_tags.index(tag) - 1]


def get_major_minor_patch(version_str):
    """Return the major, minor & patch revision numbers as integers"""
    version_components = version_str.split(".")
    if len(version_components) != 3:
        raise RuntimeError("Invalid format for version string. Expected X.Y.Z")
    return map(int, version_components)


def get_shortened_version_string(version_str):
    """
    We use the convention whereby the patch number is ignored if it is zero,
    i.e. "3.0.0" becomes "3.0".
    """
    major, minor, patch = get_major_minor_patch(version_str)
    if patch == 0:
        return "{0}.{1}".format(major, minor)
    else:
        return "{0}.{1}.{2}".format(major, minor, patch)


def get_version_from_git_tag(tag):
    """
    Given a tag from Git, extract the major, minor and patch version
    numbers.
    """
    short_regexp = r"^v(\d+).(\d+)(-|$)"
    long_regexp = r"^v(\d+).(\d+).(\d+)(-|$)"

    if re.match(short_regexp, tag):
        match_text = re.match(short_regexp, tag).group(0)
        a, b = [int(x) for x in re.findall(r"\d+", match_text)]
        c = 0
    elif re.match(long_regexp, tag):
        match_text = re.match(long_regexp, tag).group(0)
        (
            a,
            b,
            c,
        ) = [int(x) for x in re.findall(r"\d+", match_text)]
    else:
        raise RuntimeError('Unable to parse version information from "' + tag + '"')
    return "{0}.{1}.{2}".format(a, b, c)


def authors_up_to_git_tag(tag):
    """
    Get a list of all authors who have made a commit, up to and including
    the given tag.
    """
    return _authors_from_tag_info(tag)


def authors_under_git_tag(tag):
    """Get a list of all authors who have made a commit, up to and including
    the given tag, but after the tag of the previous version.  I.e. if given
    "2, 6, 1" as inputs, then only commits between the tags "v2.6.0" and
    "v2.6.1" will be included.
    """
    all_tags = _get_all_release_git_tags()

    previous_tag = all_tags[all_tags.index(tag) - 1]

    return _authors_from_tag_info(previous_tag + ".." + tag)


if __name__ == "__main__":
    """This should normally be run as a library for doi.py, but for testing/development a command line was added."""
    import argparse

    parser = argparse.ArgumentParser(
        description="Generate list of authors for a release", epilog="This requires having a .gitmailmap file from a TWG member"
    )
    parser.add_argument("version", type=str, help='Version of Mantid whose DOI is to be created/updated in the form "major.minor.patch"')
    parser.add_argument("--full", action="store_true", help="Get full list of authors back to start of project")

    # process inputs
    args = parser.parse_args()
    git_tag = find_tag(args.version)

    # generate list of authors
    if args.full:
        authors = authors_up_to_git_tag(git_tag)
    else:
        authors = authors_under_git_tag(git_tag)

    # print out the results
    print("For", git_tag)
    for author in authors:
        print(author)
