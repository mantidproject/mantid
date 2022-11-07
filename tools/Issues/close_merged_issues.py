"""Examines all issues assigned to a given milestone and if they are associated with
a merged pull request that is linked to fix the issue then it closes that issue.

This module exists as GitHub only closes issues linked with 'Fixes #1234' if a
pull request was merged to the default branch. During a beta period pull requests
merged to the release branch leaving dangling issues around.

Requires the pygithub module.
"""
import argparse
import logging
import posixpath
import re
import sys
from typing import Optional, Sequence, Mapping

import github

DEFAULT_REPO = 'mantidproject/mantid'
ISSUE_CLOSE_RE = re.compile(
    r'\b(?:close|closes|closed|fix|fixes|fixed|resolve|resolves|resolved)\b\s+#(\d+)',
    flags=re.IGNORECASE)


class _HashableGithubIssue:
    """Thin extension to store an issue as a dict key"""
    def __init__(self, issue: github.Issue.Issue):
        self._issue = issue

    def __hash__(self) -> int:
        return self._issue.number


# Link issues to pull requests
IssueToPRs = Mapping[_HashableGithubIssue, Sequence[github.PullRequest.PullRequest]]


class IssuesToCloseInfo:
    """Simple type to aggreagate summary information"""
    _issues: IssueToPRs = None

    def __init__(self):
        self._issues = dict()

    def add_issue(self, issue: github.Issue.Issue, pull_request: github.PullRequest.PullRequest):
        prs = self._issues.setdefault(_HashableGithubIssue(issue), [])
        prs.append(pull_request)

    def close_all(self):
        """Edits the state of the issues to closed"""
        for issue, prs in self._issues.items():
            issue._issue.edit(state='closed')
            pr_refs = '#' + ',#'.join(map(lambda x: str(x.number), prs))
            issue._issue.create_comment(f'Fixed by {pr_refs}')

    def prettyprint(self, writer):
        lines = [f'{len(self._issues)} open issues are referenced to be closed']
        row_format = '{:^15}|{:^15}'
        lines.append(row_format.format('Issue', 'Pulls'))
        lines.append('-' * 30)
        for issue, prs in self._issues.items():
            formatted_prs = ','.join(map(lambda x: str(x.number), prs))
            lines.append(row_format.format(issue._issue.number, formatted_prs))

        writer('\n'.join(lines))


def enable_console_logging():
    """
    This function sets up a very simple logging configuration (log everything on standard output) that is useful for troubleshooting.
    """
    logging.basicConfig(level=logging.INFO)


def main() -> int:
    """Main cli entry point"""
    enable_console_logging()
    args = parse_arguments()

    github_api = github.Github(args.token)
    repository = github_api.get_repo(args.repo)
    milestone = None
    if args.milestone is not None:
        milestone = find_matching_milestone(repository, args.milestone)
        if milestone is None:
            logging.error(
                f'Unable to find milestone with title "{args.milestone}" in repository "{args.repo}"')
            return 1

    close_issues_with_merged_pull_request(repository, milestone, args.dry_run)
    return 0


def parse_arguments() -> argparse.Namespace:
    """
    Parse options given to program

    :return: The arguments as a argparse object
    """
    parser = argparse.ArgumentParser(description="Close issues fixed by merged pull requests")
    parser.add_argument("--repo",
                        dest="repo",
                        default=DEFAULT_REPO,
                        help="The full org/repo name of a GitHubrepository")
    parser.add_argument("--token", required=True, help="GitHub authorization token")
    parser.add_argument(
        "--milestone", help="An optional milestone on the project. Checks all open issues for this milestone.")
    parser.add_argument(
        "--dry-run", help="If true, just print what would happen but don't do anything", action='store_true')
    return parser.parse_args()


def find_matching_milestone(repository: github.Repository.Repository,
                            milestone_title: str) -> Optional[github.Milestone.Milestone]:
    """Given a milestone title, find the matching milestone
    in the Github repo"""
    for milestone in repository.get_milestones(state='open'):
        if milestone.title == milestone_title:
            return milestone

    return None


def close_issues_with_merged_pull_request(repository: github.Repository.Repository,
                                          milestone: Optional[github.Milestone.Milestone] = None, dry_run: bool = False):
    """
    For each open issue, optionally constrained to a milestone,:
      - check if a pull request is connected to it
      - if the pull request fixes/closes the connected issue then close the issue
    :param repository: A repository object
    :param milestone: An optional  Milestone object
    :param dry_run: If True no action is take but the steps are printed
    """
    if milestone:
        open_issues = repository.get_issues(milestone=milestone, state='open')
        logging.info(f'Milestone "{milestone.title}" has {open_issues.totalCount} open issues')
    else:
        open_issues = repository.get_issues(state='open')
        logging.info(f'Repository has {open_issues.totalCount} open issues')
    logging.info('Checking for merged linked pull requests')
    # Github does not provide an easy link between pull requests & issues
    # We use the timeline on the issue to look for a referenced event and extract the pull_request from the raw_data

    issues_to_close = find_issues_fixed_by_pull_requests(open_issues)
    issues_to_close.prettyprint(logging.info)
    if dry_run:
        logging.info('Dry run requested. No close action taken')
    else:
        logging.info('Closing these issues')
        issues_to_close.close_all()


def find_issues_fixed_by_pull_requests(
        issues: Sequence[github.Issue.Issue]) -> Sequence[github.Issue.Issue]:
    """
    Filter the list of issues and return any fixed/closed by a pull request
    :param issues: A sequence of Issue objects
    :return: True if the issue was close, false otherwise
    """
    fixed_issues = IssuesToCloseInfo()
    for issue in issues:
        prs = get_linked_pull_requests(issue)
        if len(prs) == 0:
            continue
        for pr in prs:
            if is_fixed_by(issue, pr):
                fixed_issues.add_issue(issue, pr)

    return fixed_issues


def get_linked_pull_requests(issue: github.Issue.Issue) -> Sequence[github.PullRequest.PullRequest]:
    """
    Returns linked pull requests if any exist.
    """

    # There is no first-class connection between an issue & a pull request
    # so we use the events timeline to search for a cross-referenced event
    # that contains a link to the pull request
    def get_pr_number(event):
        if event.event != 'cross-referenced':
            return
        raw_data = event.raw_data['source']['issue']
        pull_request = raw_data.get('pull_request', None)
        if pull_request is None:
            return
        return int(posixpath.basename(pull_request['url']))

    pr_numbers = filter(lambda x: x is not None, map(get_pr_number, issue.get_timeline()))
    repository = issue.repository
    return [repository.get_pull(pr_number) for pr_number in pr_numbers]


def is_fixed_by(issue: github.Issue.Issue, pull_request: github.PullRequest.PullRequest) -> bool:
    """
    Returns True if the issue is fixed by the given pull request and that pull request is merged
    :param issue: An Issue object
    :param pull_request: A PullRequest object
    """
    if not pull_request.merged:
        return False

    # We use the body of the pull request and check for any of the keywords defined by GitHub
    # that close an issue
    # https://docs.github.com/en/github/managing-your-work-on-github/linking-a-pull-request-to-an-issue#linking-a-pull-request-to-an-issue-using-a-keyword
    pr_closed_issues = ISSUE_CLOSE_RE.findall(pull_request.body)
    return str(issue.number) in pr_closed_issues


if __name__ == '__main__':
    sys.exit(main())
