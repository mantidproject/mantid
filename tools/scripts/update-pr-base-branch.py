# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, print_function)

# std library
import argparse
import json
import sys

# 3rd party libs
import requests

# Constants
DEBUG = True
GH_API_URL = "https://api.github.com"
GH_SEARCH_ISSUES_ENDPOINT = "search/issues"
GH_PULLS_ENDPOINT = "repos/{}/pulls"
DEFAULT_REPO = "mantidproject/mantid"


def info(msg):
    """
    Display an informational message
    :param msg: A string to display
    """
    print(msg)


def debug(msg):
    """
    Display an debug message if debugging is enabled
    :param msg: A string to display
    """
    if DEBUG:
        info(msg)


def die(msg):
    """
    Display an error and exit
    :param msg: A string to display
    """
    info(msg)
    sys.exit(1)


class PullRequest(object):
    """
    Thin wrapper around a pull request
    """

    def __init__(self, repo, id):
        """
        :param repo: The repository holding the pull request
        :param id: The pull request ID
        """
        self.repo = repo
        self.id = id

    def __str__(self):
        return "{}: #{}".format(self.repo, self.id)

    def __repr__(self):
        return str(self)


def parse_arguments(argv):
    """
    Parse options given to program

    :param argv: The list of command line arguments
    :return: The arguments as a argparse object
    """
    parser = argparse.ArgumentParser(
        description="Update the base branch of pull requests")
    parser.add_argument("--repo",
                        dest="repo",
                        default=DEFAULT_REPO,
                        help="The full org/repo name of a GitHubrepository")
    parser.add_argument("--token",
                        required=True,
                        help="GitHub authorization token")
    parser.add_argument(
        "milestone",
        help=
        "A milestone on the project. Updates all open PRs against this milestone"
    )
    parser.add_argument("newbase",
                        help="The new base branch of the pull requests")
    return parser.parse_args()


def get_open_pull_request_ids(repo, milestone):
    """
    Find the ids of all of the open pull requests for the milestone
    :param repo:
    :param milestone: A milestone on the
    :return:
    """
    info("Fetching IDs of all open pull requests")

    issues_search_url = GH_API_URL + "/" + GH_SEARCH_ISSUES_ENDPOINT
    search_string = "repo:{} is:pr is:open milestone:\"{}\"".format(
        repo, milestone)
    debug("Sending query '{}' to '{}'".format(search_string,
                                              issues_search_url))
    response = requests.get(issues_search_url, params={"q": search_string})
    response.raise_for_status()

    response_json = response.json()
    debug("Raw response:\n    " + response.text)
    milestone_prs = []
    for pr in response_json["items"]:
        milestone_prs.append(PullRequest(repo, int(pr["number"])))

    debug("Found {} open pull requests: {}".format(len(milestone_prs),
                                                   str(milestone_prs)))
    return milestone_prs


def update_base_branch(prs, newbase, token):
    """
    Updates the base branch of each PR to newbas
    :param prs: A list of PullRequest objects
    :param newbase: A string giving the new base for each pull request
    :param token: An authorization token for GitHub
    """
    for pr in prs:
        update_pr_base_branch(pr, newbase, token)


def update_pr_base_branch(pr, newbase, token):
    """
    Update the base branch of the pull request on the given repo
    :param pr: A single PullRequst object
    :param newbase: The new base branch

    """
    info("Updating base branch of #{} to {}".format(pr.id, newbase))
    pull_url = "{}/{}/{}".format(GH_API_URL, GH_PULLS_ENDPOINT.format(pr.repo),
                                 pr.id)
    headers = {"Authorization": "token {}".format(token)}
    data = {"base": newbase}

    debug("Sending data '{}' to {}".format(data, pull_url))
    response = requests.post(pull_url, headers=headers, data=json.dumps(data))
    response.raise_for_status()


def main():
    args = parse_arguments(sys.argv)
    open_pr_ids = get_open_pull_request_ids(args.repo, args.milestone)
    update_base_branch(open_pr_ids, args.newbase, args.token)


if __name__ == '__main__':
    main()
