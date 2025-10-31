from datetime import date
import matplotlib.pyplot as plt
import re

# data index is next to this file and contains all releases
release_index = "index.rst"

version_date_regexp = re.compile(r".+v(\d+\.\d+\.\d+).+(\d\d\d\d-\d\d-\d\d).+")

# read all of the releases from index.rst
versions = []
dates = []
with open(release_index, mode="r") as handle:
    for line in handle:
        line = line.strip()
        if line:
            match = version_date_regexp.match(line)
            if match:
                version, datestamp = match.groups()
                dates.append(date.fromisoformat(datestamp))

                major, minor = version.split(".")[:2]  # only major.minor

                versions.append(float(f"{major}.{int(minor):02d}"))

# set date-time for fist commit 44dd034194867159c37ee89c8d4087ed356361d8
datestamp_first_commit = date.fromisoformat("2007-04-04")

# plot in xkcd style - only this plot
with plt.xkcd():
    fig, ax = plt.subplots()
    ax.scatter(dates, versions)
    ax.plot((datestamp_first_commit, dates[-1]), (1, 1), linewidth=5)
    ax.tick_params("x", rotation=90)
    ax.set_xlabel("year")
    ax.set_ylabel("mantid version")
    ax.annotate("iterations", (datestamp_first_commit, 1.25))
    print(dir(fig))
    fig.tight_layout()
    plt.show()
