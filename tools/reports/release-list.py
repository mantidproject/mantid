# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
import datetime
import os
import requests
import subprocess
import sys


class Release:
    def __init__(self, name, datestamp):
        name = name.replace("Iteration ", "Iteration")
        name = name.replace("Release ", "v")
        if name.count(".") == 1:
            name = name + ".0"
        self.name = name

        self.date = datestamp.strftime("%Y-%m-%d")

    def __str__(self):
        return "%s,%s" % (self.date, self.name)

    def __eq__(self, other):
        return self.name == other.name and self.date == other.date

    def __hash__(self):
        return hash(str(self))


def getTags(repolocation):
    arg_tags = ["git", "log", "--tags", "--simplify-by-decoration", '--pretty="format:%ai %d"']
    sub = subprocess.Popen(arg_tags, stdout=subprocess.PIPE, close_fds=True, cwd=repolocation)
    tags = [line.decode("utf-8").strip() for line in sub.stdout]
    tags = [tag for tag in tags if "tag:" in tag]
    result = []
    for tag in tags:
        tag = tag.split()
        datestamp = "%sT%s%s" % tuple(tag[:3])
        datestamp = datestamp.split("format:")[1]

        # grab out the timezone
        tzd = 0
        if "+" in datestamp:
            (datestamp, tzd) = datestamp.split("+")
            tzd = int(0.01 * float(tzd))
        elif datestamp.count("-") == 3:
            tzd = "-" + datestamp.split("-")[-1]
            datestamp = datestamp.replace(tzd, "")
            tzd = int(0.01 * float(tzd))
        tzd = datetime.timedelta(hours=tzd)

        # parse the date and correct for timezone
        datestamp = datetime.datetime.strptime(datestamp, "%Y-%m-%dT%H:%M:%S")
        datestamp -= tzd

        tag = " ".join(tag[3:])[:-1]
        tag = tag.replace("(", "")
        tag = tag.replace(")", "")
        tag = [item.replace("tag: ", "") for item in tag.split(", ") if "tag:" in item]
        for item in tag:
            result.append(Release(item, datestamp))

    return result


def parseMilestones(json_doc):
    milestones = []
    for milestone in json_doc:
        name = milestone["title"]
        datestamp = datetime.datetime.strptime(milestone["due_on"], "%Y-%m-%dT%H:%M:%SZ")
        milestones.append(Release(name, datestamp))
    return milestones


def getMilestones(endpoint, oauth=None):
    # print('Getting list of all milestones from Github.')
    milestones = []

    req_params = {"state": "all"}
    if oauth is not None:
        req_params["access_token"] = oauth
    req = requests.get("%smilestones" % endpoint, params=req_params)
    status_code = req.status_code
    if status_code == 403:
        print("status:", status_code)
        print(req.json()["message"])
        sys.exit(-1)

    try:
        json_doc = req.json()
    except TypeError:
        json_doc = req.json
    milestones.extend(parseMilestones(json_doc))

    while status_code == 200:
        # we have more pages
        try:
            req = requests.get(req.links["next"]["url"])
            if status_code == 403:
                print("status:", status_code)
                print(json_doc()["message"])
                sys.exit(-1)
            milestones.extend(parseMilestones(req.json()))
        except:
            status_code = 0

    return milestones


def writeFile(releases):
    csvreleases = open("releases.csv", "w")
    csvreleases.write("date,milestone\n")

    for release in releases:
        csvreleases.write(str(release) + "\n")

    csvreleases.close()


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("must supply repository location")
        print("usage: %s <git repo> [oauth]" % sys.argv[0])
        sys.exit(-1)

    repolocation = sys.argv[1]
    if not os.path.isdir(repolocation):
        print("ERROR: Specified repository location is not a directory.")
        sys.exit(-1)

    if len(sys.argv) > 2:
        oauth = sys.argv[2]
    else:
        oauth = None

    releases = getTags(repolocation)

    today = datetime.datetime.now().strftime("%Y-%m-%d")

    milestones = getMilestones("https://api.github.com/repos/mantidproject/mantid/", oauth)
    for milestone in milestones:
        # only unique milestones
        if milestone not in releases:
            # see if they have a different date
            found = False
            for i, release in enumerate(releases):
                if release.name == milestone.name:
                    found = True
                    if milestone.date < release.date:
                        releases[i] = milestone
                    break
            # only if they exist in the past
            if not found:
                if milestone.date <= today:
                    releases.append(milestone)

    releases = list(set(releases))
    releases = sorted(releases, key=lambda release: release.date)

    writeFile(releases)
