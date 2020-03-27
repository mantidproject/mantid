# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=invalid-name

import datetime
import csv
import os
import re

temp_filename = 'all-commits.stdout'
regex_git_log_entry = re.compile(
    r"Author:\s+(.+?)\s+Date:\s+(.+?)\B\s+(\S+).*?((\d+)\sfile.+?)?((\d+)+\sinsertion.+?)?((\d+)+\sdeletion.+?)?(commit\s[0-9a-f]{40}|$)",
    re.DOTALL)
regex_git_log_splitter = re.compile(
    r"commit\s[0-9a-f]{40}")
regex_name_email_address = re.compile(r"(.*?)<(\S+)>")

organisations = ['STFC', 'ORNL', 'ESS', 'ILL', 'PSI', 'ANSTO', 'KITWARE', 'JUELICH', 'OTHERS', 'CSNS']

domains = {'stfc.ac.uk': 'STFC',
           'clrc.ac.uk': 'STFC',
           'tessella.com': 'STFC',
           'ornl.gov': 'ORNL',
           'sns.gov': 'ORNL',
           'esss.se': 'ESS',
           'ill.fr': 'ILL',
           'ill.eu': 'ILL',
           'psi.ch': 'PSI',
           'ansto.gov.au': 'ANSTO',
           'ansto': 'ANSTO',
           'mantidproject.org': 'OTHERS',
           'MichaelWedel@users.noreply.github.com': 'PSI',
           'stuart.i.campbell@gmail.com': 'ORNL',
           'uwstout.edu': 'ORNL',
           'kitware.com': 'KITWARE',
           'juelich.de': 'JUELICH',
           'ian.bush@tessella.com': 'STFC',
           'dan@dan-nixon.com': 'STFC',
           'peterfpeterson@gmail.com': 'ORNL',
           'stuart@stuartcampbell.me': 'ORNL',
           'harry@exec64.co.uk': 'STFC',
           'martyn.gigg@gmail.com': 'STFC',
           'raquelalvarezbanos@users.noreply.github.com': 'STFC',
           'torben.nielsen@nbi.dk': 'ESS',
           'borreguero@gmail.com': 'ORNL',
           'raquel.alvarez.banos@gmail.com': 'STFC',
           'anton.piccardo-selg@tessella.com': 'STFC',
           'rosswhitfield@users.noreply.github.com': 'ORNL',
           'mareuternh@gmail.com': 'ORNL',
           'quantumsteve@gmail.com': 'ORNL',
           'ricleal@gmail.com': 'ORNL',
           'jawrainey@gmail.com': 'STFC',
           'xingxingyao@gmail.com': 'ORNL',
           'owen@laptop-ubuntu': 'STFC',
           'picatess@users.noreply.github.com': 'STFC',
           'Janik@Janik': 'ORNL',
           'debdepba@dasganma.tk': 'OTHERS',
           'matd10@yahoo.com': 'OTHERS',
           'diegomon93@gmail.com': 'OTHERS',
           'mgt110@ic.ac.uk': 'OTHERS',
           'granrothge@users.noreply.github.com': 'ORNL',
           'tom.g.r.brooks@gmail.com': 'STFC',
           'ross.whitfield@gmail.com': 'ORNL',
           'samueljackson@outlook.com': 'STFC',
           'AntonPiccardoSelg@users.noreply.github.com': 'STFC',
           'antibones@users.noreply.github.com': 'ILL',
           'MikeHart85@users.noreply.github.com': 'STFC',
           'dbt@aber.ac.uk': 'STFC',
           'DavidFair@users.noreply.github.com': 'STFC',
           'reimundILL@users.noreply.github.com': 'ILL',
           'jan@c53.be': 'JUELICH',
           'reimund@il.eu': 'ILL',
           'davidfair@users.noreply.github.com': 'STFC',
           'louisemccann@users.noreply.github.com': 'STFC',
           'DimitarTasev@users.noreply.github.com': 'STFC',
           'dimtasev@gmail.com': 'STFC',
           'fedepou@gmail.com': 'STFC',
           'cip.pruteanu@gmail.com': 'OTHERS',
           'kdymkowski84@gmail.com': 'OTHERS',
           'mayer.ali@t-online.de': 'OTHERS',
           'gagikvar@gmail.com': 'ILL',
           'bartomeu.llopis.vidal@gmail.com': 'STFC',
           'anton.piccardo-selg@tessella.ac.uk': 'STFC',
           'jamesphysics@users.noreply.github.com': 'STFC',
           'michaeljturner@live.com': 'STFC',
           'rprospero@gmail.com': 'STFC',
           'roman.tolchenov@gmail.com': 'STFC',
           'jiao.lin@gmail.com': 'ORNL',
           'erkn@fysik.dtu.dk': 'ESS',
           'daniel@pajerowski.com': 'ORNL',
           'ElliotAOram@users.noreply.github.com': 'STFC',
           '37333817+thomueller@users.noreply.github.com': 'ESS',
           't.w.jubb@gmail.com': 'STFC',
           'edward.brown.96@live.co.uk': 'STFC',
           'bhuvan_777@outlook.com': 'STFC',
           'joachimcoenen@icloud.com': 'JUELICH',
           'anton.piccardo.selg@gmail.com': 'STFC',
           '29330338+JoachimCoenen@users.noreply.github.com': 'JUELICH',
           'samjones714@gmail.com': 'STFC',
           '5237234+ewancook@users.noreply.github.com': 'STFC',
           '40766142+SamJenkins1@users.noreply.github.com': 'STFC',
           'aybamidele@gmail.com': 'STFC',
           '5237234+ewancook@users.noreply.github.com': 'STFC',
           'samjones714@gmail.com': 'STFC',
           '40830825+robertapplin@users.noreply.github.com': 'STFC',
           'robertgjapplin@gmail.com': 'STFC',
           'luzpaz@users.noreply.github.com': 'OTHERS',
           't.j.titcombe@gmail.com': 'STFC',
           '32938439+TTitcombe@users.noreply.github.com': 'STFC',
           '35809089+EdwardsLT@users.noreply.github.com': 'STFC',
           'EdwardsLT@cardiff.ac.uk': 'STFC',
           '39047984+nvaytet@users.noreply.github.com': 'ESS',
           'igudich@gmail.com': 'ESS',
           '46603316+alicerussell1@users.noreply.github.com': 'STFC',
           '31194136+aybamidele@users.noreply.github.com': 'STFC',
           '49688535+Harrietbrown@users.noreply.github.com': 'STFC',
           'takudzwamilli@gmail.com': 'STFC',
           'lorenzobasso@unseen.is': 'STFC',
           'a.j.jackson@physics.org': 'STFC',
           '32895149+LolloB@users.noreply.github.com': 'STFC',
           'philipc99@hotmail.co.uk': 'STFC',
           'conor.m.finn.99@gmail.com': 'STFC',
           '52415735+PhilColebrooke@users.noreply.github.com': 'STFC',
           'giodisiena@gmail.com': 'STFC',
           'matthew-d-jones@users.noreply.github.com': 'STFC',
           '32419974+TakudzwaMakoni@users.noreply.github.com': 'STFC',
           'hankwu@Hanks-MacBook-Air.local': 'STFC',
           '55147936+hankwustfc@users.noreply.github.com': 'STFC',
           '55979119+RichardWaiteSTFC@users.noreply.github.com': 'STFC',
           'Waite': 'STFC',
           '47181718+ConorMFinn@users.noreply.github.com': 'STFC',
           '31892119+Fahima-Islam@users.noreply.github.com': 'ORNL',
           '56431339+StephenSmith25@users.noreply.github.com': 'STFC',
           'williamfgc@yahoo.com': 'ORNL'}

aliases = {'Anthony':'Anthony Lim',
           'AnthonyLim23':'Anthony Lim',
           'abuts':'Alex Buts',
           'Ayomide Bamidele':'Andre Bamidele',
           'DanielMurphy22':'Daniel Murphy',
           'Harrietbrown':'Harriet Brown',
           'PhilColebrooke':'Phil Colebrooke',
           'Phil':'Phil Colebrooke',
           'Richard':'Richard Waite',
           'RichardWaiteSTFC':'Richard Waite',
           'Stephen':'Stephen Smith',
           'StephenSmith25':'Stephen Smith',
           'StephenSmith':'Stephen Smith',
           'Anders-Markvardsen':'Anders Markvardsen',
           'AndreiSavici':'Andrei Savici',
           'Antti Soininnen':'Antti Soininen',
           'Bilheux':'Jean Bilheux',
           'brandonhewer':'Brandon Hewer',
           'celinedurniak':'Celine Durniak',
           'DavidFair':'David Fairbrother',
           'DiegoMonserrat':'Diego Monserrat',
           'Dimitar Borislavov Tasev':'Dimitar Tasev',
           'Tasev':'Dimitar Tasev',
           'giovannidisiena':'Giovanni Di Siena ',
           'hankwustfc':'Hank Wu ',
           'igudich':'Igor Gudich',
           'josephframsay':'Joseph Ramsay',
           'LamarMoore':'Lamar Moore',
           'Moore':'Lamar Moore',
           'LolloB':'Lorenzo Basso',
           'NickDraper':'Nick Draper',
           'Pete Peterson':'Peter Peterson',
           'Parker, Peter G':'Peter Parker',
           'Raquel Alvarez':'Raquel Alvarez Banos',
           'reimundILL':'Verena Reimund ',
           'Ricardo Leal':'Ricardo Ferraz Leal',
           'Ricardo M. Ferraz Leal':'Ricardo Ferraz Leal',
           'Rob':'Robert Applin',
           'Rob Applin':'Robert Applin',
           'robertapplin ':'Robert Applin',
           'Sam':'Sam Jenkins',
           'SamJenkins1':'Sam Jenkins',
           'simonfernandes':'Simon Fernandes',
           'MichaelWedel':'Michael Wedel',
           'Steven E. Hahn':'Steven Hahn',
           'VickieLynch':'Vickie Lynch'
           }


def generate_commit_data():
    print('Generating git commit data...')
    os.system("git --no-pager log --shortstat > " + temp_filename)


def parse_commit_data():
    print("Reading the file")
    # Open a file: file
    commit_entries = []
    commit_entry = ""
    with open (temp_filename, "r", encoding="utf-8") as file:
        # read all lines at once
        log_line = file.readline()
        while (log_line):
            if regex_git_log_splitter.match(log_line):
                commit_entries.append(commit_entry)
                commit_entry = log_line
            else:
                commit_entry += log_line
            log_line = file.readline()

    #find the matches
    print("searching for regex matches")
    with open('commits-report.csv', mode='w', newline='') as output_file:
        commit_writer = csv.writer(output_file, delimiter=',', quotechar='"', quoting=csv.QUOTE_MINIMAL)
        commit_writer.writerow(["Author", "Email", "Facility", "Date_time", "Year", "Quarter", "Month", "Week", "Commits",
                                "Files Changed", "Insertions", "Deletions", "Net Lines Changed"])
        for commit_text in commit_entries:
            parse_log_entry(commit_text,commit_writer)


def parse_log_entry(commit_text,commit_writer):
    if commit_text =="":
        return
    # black listed log entry that crashes the regex engine
    if commit_text.startswith("commit 4a6c0077a1dff965d767dc45a1517c7411a69070") or \
            commit_text.startswith("commit 16a4f16c99e3dc3b59d214067781c932f5a9eb8a"):
        return
    try:
        match = regex_git_log_entry.search(commit_text)
        #skip merges
        if match.group(3) != "Merge":
            author = match.group(1)
            name, email = extract_name_email_from_author(author)
            date_time_str = match.group(2).strip()
            date_time = None
            try:
                date_time = datetime.datetime.strptime(date_time_str, '%a %b %d %H:%M:%S %Y %z')
            except ValueError as e:
                print ("Date Parsing failed")
                print(date_time_str, e)
                print(commit_text)
                return
            files = 0 if match.group(5) is None else int(match.group(5))
            insertions = 0 if match.group(7) is None else int(match.group(7))
            deletions = 0 if match.group(9) is None else int(match.group(9))
            facility = get_user_facility(email,date_time)
            commit_writer.writerow([name,email,facility,date_time.strftime("%Y-%m-%d %H:%M"),
                                    date_time.strftime("%Y"), (date_time.month-1)//3 + 1, date_time.strftime("%m"),
                                    date_time.isocalendar()[1], 1,
                                    files,insertions,deletions, insertions-deletions])
    except RuntimeError as e:
        print("Match failed", e)
        print(commit_text)


def extract_name_email_from_author(author):
    match = regex_name_email_address.search(author)
    if match:
        original_name = match.group(1).strip()
        name = aliases[original_name] if original_name in aliases.keys() else original_name
        return name,match.group(2)
    else:
        return None


def get_user_facility(email, datetime):
    facility = "UNKNOWN"
    for domain in domains.keys():
        if domain in email:
            # ORNL didn't join until 2009
            if domains[domain] == 'ORNL' and datetime.year < 2009:
                domain = 'stfc.ac.uk'
            facility = domains[domain]
    if facility == "UNKNOWN":
        print("Unmatached email", email)
    return facility


if __name__ == '__main__':
    print("Generating github commit metrics...\n")

    generate_commit_data()
    parse_commit_data()
    os.remove(temp_filename)

    print("\n\nAll done!\n")
