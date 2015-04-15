Leeroy
======

This document describes the implementation of the automated pull request Jenkins builder [leeroy](http://github.com/jfrazelle/leeroy).
Leeroy is written in Go and we are using a particular fork found [here](http://github.com/rosswhitfield/leeroy).

The user documents for the Mantid Leeroy can be found at [The automated build process](http://www.mantidproject.org/The_automated_build_process).

Setup
-----

Leeroy runs on builds.mantidproject.org on port _5000_ under the user _leeroy_, iptables were modified to allow this.

Github webhooks will need to manually be added. Add `http://builds.mantidproject.org:5000/notification/github` to be notified of pull request events.

There is a cron job to restart leeroy if it crashes (have yet to see this happen) or if the build server is restarted. There is a `start_leeroy.sh` in `/home/leeroy/` which will start leeroy with the correct arguments, which the cron job calls.

The config file for leeroy can be found at /home/leeroy/config.json and is copied at the bottom of this document with jenkins and github tokens removed. Leeroy uses the _mantid-builder_ account to access Jenkins and Github via API tokens.

Adding a new job to Leeroy
--------------------------

To setup a new Jenkins jobs to work with Leeroy there are three additional thing needed over the standard settings for a job in Jenkins.

   * Add a Job Notifications with `URL` set to `http://localhost:5000/notification/jenkins`
   * Add 5 string parameters; `GIT_BASE_REPO`, `GIT_HEAD_REPO`, `GIT_SHA1`, `GITHUB_URL` and `PR`.
   * Under Source Code Management set Repository `URL` to `git://github.com/$GIT_HEAD_REPO.git`, Refspec (under Advanced) to `+refs/pull/$PR/merge:pull-$PR-merged` and Branch Specifier to `pull-$PR-merged`.
   * Add the build to `config.json` and restart Leeroy.


Other things
------------

Using the `Refspec` in the job config will mean that the pull request branch will be merged into the target before it is built. If it can't be merged cleanly the pull request will fail.

Leeroy can be manually triggered outside of the Jenkins Rebuild by running for example
```shell
curl -u USER:PASS builds.mantidproject.org:5000/build/retry -d '{"context":"cppcheck","repo":"mantidproject/mantid","number":415}'
```
which will start the cppcheck build on pull request 415

A cron jobs runs `/home/leeroy/cron_leeroy.sh` every 10 minutes to pick up missed builds, which executes the following type command.
```shell
curl -u USER:PASS builds.mantidproject.org:5000/build/cron -d '{"context":"cppcheck","repo":"mantidproject/mantid"}'
```

Config file (`config.json`)
-----------
```json
{
    "jenkins": {
        "username": "mantid-builder",
        "token": "",
        "base_url": "http://builds.mantidproject.org"
    },
    "build_commits": "last",
    "github_token": "",
    "builds": [
        {
            "github_repo": "mantidproject/mantid",
            "jenkins_job_name": "pull_requests-rhel7",
            "context": "RHEL7"
        },
        {
            "github_repo": "mantidproject/mantid",
            "jenkins_job_name": "pull_requests-rhel6",
            "context": "RHEL6"
        },
        {
            "github_repo": "mantidproject/mantid",
            "jenkins_job_name": "pull_requests-ubuntu",
            "context": "Ubuntu"
        },
        {
            "github_repo": "mantidproject/mantid",
            "jenkins_job_name": "pull_requests-win7",
            "context": "Windows"
        },
        {
            "github_repo": "mantidproject/mantid",
            "jenkins_job_name": "pull_requests-osx",
            "context": "OSX"
        },
        {
            "github_repo": "mantidproject/mantid",
            "jenkins_job_name": "pull_requests-cppcheck",
            "context": "cppcheck"
        }
    ],
    "user": "USER",
    "pass": "PASS"
}
```
