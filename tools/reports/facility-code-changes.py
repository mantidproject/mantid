# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
import datetime
import subprocess
import csv
import argparse
import os
import time

__author__ = "Stuart Campbell"


def generate_file_changes_data(year_start, year_end):
    current_year = int(datetime.datetime.now().strftime("%Y"))
    current_month = int(datetime.datetime.now().strftime("%m"))

    print("Generating git file change data...")

    for year in range(year_start, year_end + 1):
        for month in range(1, 13):
            # Don't go past the current month
            if current_year == year:
                if month > current_month:
                    continue
            since = "--since='{0}-{1}-1'".format(str(year), str(month))
            until = "--before='{0}-{1}-{2}'".format(str(year), str(month), str(days_in_month[month - 1]))

            date_key = str(year) + "-{0:02d}".format(month)

            f = open("facility-file-changes-{0}.stdout".format(date_key), "w", buffering=0)
            arg_changes = ["git", "log", '--pretty=format:"%aE"', "--shortstat", since, until]
            subprocess.Popen(arg_changes, stdout=f, stderr=subprocess.PIPE, cwd=repolocation)
            f.flush()
            os.fsync(f.fileno())
            f.close()


def generate_commit_data(year_start, year_end):
    current_year = int(datetime.datetime.now().strftime("%Y"))
    current_month = int(datetime.datetime.now().strftime("%m"))

    print("Generating git commit data...")

    for year in range(year_start, year_end + 1):
        for month in range(1, 13):
            # Don't go past the current month
            if current_year == year:
                if month > current_month:
                    continue

            since = "--since='{0}-{1}-1'".format(str(year), str(month))
            until = "--before='{0}-{1}-{2}'".format(str(year), str(month), str(days_in_month[month - 1]))

            date_key = str(year) + "-{0:02d}".format(month)

            f = open("facility-commits-{0}.stdout".format(date_key), "w", buffering=0)
            args_commits = ["git", "log", '--pretty=format:"%aE"', since, until]
            subprocess.Popen(args_commits, stdout=f, stderr=subprocess.PIPE, cwd=repolocation)
            f.flush()
            os.fsync(f.fileno())
            f.close()


def _ensure_is_directory(filepath):
    if not os.path.isdir(filepath):
        print("ERROR: Specified repository location is not a directory.")
        exit()


def _assign_change_to_facility(domains, changes, year, facility_dict, date_key, increment):
    found = False
    for domain in domains.keys():
        if domain in changes:
            # ORNL didn't join until 2009
            if domains[domain] == "ORNL" and int(year) < 2009:
                domain = "stfc.ac.uk"
            facility_dict[date_key][domains[domain]] != increment
            found = True
    return found, facility_dict


if __name__ == "__main__":
    print("Generating some random metrics...\n")

    lines_removed_as_negative = False

    # data = {}
    facility_commits = {}
    facility_changed = {}
    facility_added = {}
    facility_removed = {}

    parser = argparse.ArgumentParser()
    parser.add_argument("repository", type=str, help="Location of git repo")
    args = parser.parse_args()
    repolocation = args.repository

    _ensure_is_directory(repolocation)

    organisations = ["STFC", "ORNL", "ESS", "ILL", "PSI", "ANSTO", "KITWARE", "JUELICH", "OTHERS"]

    domains = {
        "stfc.ac.uk": "STFC",
        "clrc.ac.uk": "STFC",
        "tessella.com": "STFC",
        "ornl.gov": "ORNL",
        "sns.gov": "ORNL",
        "esss.se": "ESS",
        "ill.fr": "ILL",
        "ill.eu": "ILL",
        "psi.ch": "PSI",
        "ansto.gov.au": "ANSTO",
        "ansto": "ANSTO",
        "mantidproject.org": "OTHERS",
        "MichaelWedel@users.noreply.github.com": "PSI",
        "stuart.i.campbell@gmail.com": "ORNL",
        "uwstout.edu": "ORNL",
        "kitware.com": "KITWARE",
        "juelich.de": "JUELICH",
        "ian.bush@tessella.com": "STFC",
        "dan@dan-nixon.com": "STFC",
        "peterfpeterson@gmail.com": "ORNL",
        "stuart@stuartcampbell.me": "ORNL",
        "harry@exec64.co.uk": "STFC",
        "martyn.gigg@gmail.com": "STFC",
        "raquelalvarezbanos@users.noreply.github.com": "STFC",
        "torben.nielsen@nbi.dk": "ESS",
        "borreguero@gmail.com": "ORNL",
        "raquel.alvarez.banos@gmail.com": "STFC",
        "anton.piccardo-selg@tessella.com": "STFC",
        "rosswhitfield@users.noreply.github.com": "ORNL",
        "mareuternh@gmail.com": "ORNL",
        "quantumsteve@gmail.com": "ORNL",
        "ricleal@gmail.com": "ORNL",
        "jawrainey@gmail.com": "STFC",
        "xingxingyao@gmail.com": "ORNL",
        "owen@laptop-ubuntu": "STFC",
        "picatess@users.noreply.github.com": "STFC",
        "Janik@Janik": "ORNL",
        "debdepba@dasganma.tk": "OTHERS",
        "matd10@yahoo.com": "OTHERS",
        "diegomon93@gmail.com": "OTHERS",
        "mgt110@ic.ac.uk": "OTHERS",
        "granrothge@users.noreply.github.com": "ORNL",
        "tom.g.r.brooks@gmail.com": "STFC",
        "ross.whitfield@gmail.com": "ORNL",
        "MikeHart85@users.noreply.github.com": "STFC",
    }

    days_in_month = [31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31]

    csvcommits = open("facility-commits.csv", "w")
    csvchanged = open("facility-changed-files.csv", "w")
    csvadded = open("facility-added-lines.csv", "w")
    csvremoved = open("facility-removed-lines.csv", "w")

    field_names = ["date"] + organisations

    commits_writer = csv.DictWriter(csvcommits, fieldnames=field_names)
    commits_writer.writeheader()

    changed_writer = csv.DictWriter(csvchanged, fieldnames=field_names)
    changed_writer.writeheader()

    added_writer = csv.DictWriter(csvadded, fieldnames=field_names)
    added_writer.writeheader()

    removed_writer = csv.DictWriter(csvremoved, fieldnames=field_names)
    removed_writer.writeheader()

    current_year = int(datetime.datetime.now().strftime("%Y"))
    current_month = int(datetime.datetime.now().strftime("%m"))

    year_start = 2007
    # year_end = year_start
    year_end = current_year

    generate_commit_data(year_start, year_end)
    generate_file_changes_data(year_start, year_end)

    time.sleep(20)

    for year in range(year_start, year_end + 1):
        print("------{0}------".format(str(year)))
        for month in range(1, 13):
            email_changes = ""
            email_commits = ""
            # Don't go past the current month
            if current_year == year and month > current_month:
                continue

            print("Getting stats for {0}-{1:02d}".format(str(year), month))
            since = "--since='{0}-{1}-1'".format(str(year), str(month))
            until = "--before='{0}-{1}-{2}'".format(str(year), str(month), str(days_in_month[month - 1]))

            date_key = str(year) + "-{0:02d}".format(month)

            facility_commits[date_key] = {}
            facility_changed[date_key] = {}
            facility_added[date_key] = {}
            facility_removed[date_key] = {}

            freading = open("facility-file-changes-{0}.stdout".format(date_key), "r", buffering=0)

            # initialize facility counters
            for org in organisations:
                facility_commits[date_key][org] = 0
                facility_changed[date_key][org] = 0
                facility_added[date_key][org] = 0
                facility_removed[date_key][org] = 0

            for line in freading:
                changed = 0
                added = 0
                removed = 0

                # Is the line blank (or None)
                if line is None or len(line.strip().replace("\n", "")) == 0:
                    # print("BLANK:'{0}'".format(str(line)))
                    continue

                # Is the line an email address ?
                if "@" in line:
                    email_changes = line.strip()
                    # print("EMAIL:'{0}".format(str(line)))
                    continue

                for item in line.split(","):
                    if "files changed" in item:
                        changed = item.strip().split(" ")[0]
                        # print("FILES CHANGED:{0}".format(changed))
                    elif "insertions(+)" in item:
                        added = item.strip().split(" ")[0]
                        # print ("INSERTIONS:{0}".format(added))
                    elif "deletions(-)" in item:
                        removed = item.strip().split(" ")[0]
                        # print ("DELETIONS:{0}".format(removed))

                (facility_changed, found_changed) = _assign_change_to_facility(
                    domains=domains,
                    changes=email_changes,
                    year=year,
                    date_key=date_key,
                    facility_dict=facility_changed,
                    increment=int(changed),
                )
                (facility_added, found_added) = _assign_change_to_facility(
                    domains=domains,
                    changes=email_changes,
                    year=year,
                    date_key=date_key,
                    facility_dict=facility_added,
                    increment=int(added),
                )
                (facility_removed, found_removed) = _assign_change_to_facility(
                    domains=domains,
                    changes=email_changes,
                    year=year,
                    date_key=date_key,
                    facility_dict=facility_removed,
                    increment=int(removed),
                )
                found = found_changed or found_added or found_removed
                # Print out the email address if it didn't match anything
                if not found:
                    print("Email ({0}) couldn't be matched to a facility!".format(str(email_changes)))

            freading.close()

            commits = 0

            f2reading = open("facility-commits-{0}.stdout".format(date_key), "r", buffering=0)

            for line in f2reading:
                email_commits = line.replace('"', "").strip()
                (found, facility_commits) = _assign_change_to_facility(
                    domains=domains, changes=email_commits, year=year, facility_dict=facility_commits, date_key=date_key, increment=1
                )
                if not found:
                    print("Email for commits ({0}) couldn't be matched to a facility!".format(str(email_commits)))

            f2reading.close()

            commits_datarow = {"date": date_key + "-01"}
            changed_datarow = {"date": date_key + "-01"}
            added_datarow = {"date": date_key + "-01"}
            removed_datarow = {"date": date_key + "-01"}

            # Now actually store the values
            for org in organisations:
                commits_datarow[org] = facility_commits[date_key][org]
                changed_datarow[org] = facility_changed[date_key][org]
                added_datarow[org] = facility_added[date_key][org]
                if lines_removed_as_negative:
                    removed_datarow[org] = -1 * (facility_removed[date_key][org])
                else:
                    removed_datarow[org] = facility_removed[date_key][org]

                # print("In {0}, {1} has {2} commits".format(date_key, org, facility_commits[date_key][org]))

            commits_writer.writerow(commits_datarow)
            changed_writer.writerow(changed_datarow)
            added_writer.writerow(added_datarow)
            removed_writer.writerow(removed_datarow)

    csvcommits.close()
    csvchanged.close()
    csvadded.close()
    csvremoved.close()

    f = open("last-updated", "w")
    f.write(str(datetime.datetime.now()))
    f.close()

    print("All done!\n")
