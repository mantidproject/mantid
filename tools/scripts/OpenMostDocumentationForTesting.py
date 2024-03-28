# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# This was written for Python 2.7
# You are likely doing unscripted testing for Mantid when using this script so basically follow this
# advice as well as using this script and it's results. (Confirm these all work as well)
# - Algorithm dialog snapshots should appear on algorithm pages in offline help
# - Math formulae should appear on algorithm pages in offline help
# - workflow diagrams should appear on algorithm pages in offline help
#
# Author: Samuel Jones - ISIS

# Need to install BeautifulSoup4:
# pip install beautifulsoup4
from bs4 import BeautifulSoup
import urllib.request as urllib2
import re
import random
import webbrowser
import time
import argparse


def crawl_url_for_html_addons(url, k=0):
    parent_url = url
    parent_url = re.sub("index.html$", "", parent_url)
    html_page = urllib2.urlopen(url)
    soup = BeautifulSoup(html_page)
    urls = []
    for link in soup.findAll("a", attrs={"href": re.compile(".html")}):
        html_ref = link.get("href")
        urls.append(parent_url + html_ref)
    if k > 0:
        return random.sample(urls, min(k, len(urls)))
    else:
        return urls


def open_urls(list_of_urls, delay=1):
    """
    :param list_of_urls:
    :param delay: in seconds
    :return:
    """
    for url in list_of_urls:
        time.sleep(delay)
        webbrowser.open(url)


parser = argparse.ArgumentParser()
parser.add_argument("-d", "--open-tab-delay", type=int, help="Delay between each new page tab in seconds.", default=1)
parser.add_argument("-k", "--k_samples", type=int, help="Sampling random subset of k urls per documentation section", default=0)
args = parser.parse_args()

all_urls = []

k_samples = args.k_samples
if k_samples is None:
    k_samples = 0

print("Crawling for Algorithm URLs...")
algorithm_urls = crawl_url_for_html_addons("http://docs.mantidproject.org/nightly/algorithms/index.html", k_samples)
all_urls.extend(algorithm_urls)

print("Crawling for Concept URLs...")
concept_urls = crawl_url_for_html_addons("http://docs.mantidproject.org/nightly/concepts/index.html", k_samples)
all_urls.extend(concept_urls)

print("Crawling for Interface URLs...")
interface_urls = crawl_url_for_html_addons("http://docs.mantidproject.org/nightly/interfaces/index.html", k_samples)
all_urls.extend(interface_urls)

print("Crawling for Technique URLs...")
technique_urls = crawl_url_for_html_addons("http://docs.mantidproject.org/nightly/techniques/index.html", k_samples)
all_urls.extend(technique_urls)

print("Crawling python api...")
mantid_kernel_urls = crawl_url_for_html_addons("http://docs.mantidproject.org/nightly/api/python/mantid/kernel/" "index.html", k_samples)
mantid_geometry_urls = crawl_url_for_html_addons(
    "http://docs.mantidproject.org/nightly/api/python/mantid/geometry/" "index.html", k_samples
)
mantid_api_urls = crawl_url_for_html_addons("http://docs.mantidproject.org/nightly/api/python/mantid/api/" "index.html", k_samples)
# Only one
mantid_plots_urls = ["http://docs.mantidproject.org/nightly/api/python/mantid/plots/index.html"]

# Only one
mantid_simpleapi_urls = ["http://docs.mantidproject.org/nightly/api/python/mantid/simpleapi.html"]

# Only one
mantid_fitfunctions = ["http://docs.mantidproject.org/nightly/api/python/mantid/fitfunctions.html"]

mantid_utils = ["https://docs.mantidproject.org/nightly/api/python/mantid/utils/index.html"]

all_urls.extend(mantid_api_urls)
all_urls.extend(mantid_fitfunctions)
all_urls.extend(mantid_geometry_urls)
all_urls.extend(mantid_kernel_urls)
all_urls.extend(mantid_plots_urls)
all_urls.extend(mantid_simpleapi_urls)
all_urls.extend(mantid_utils)

print("All webpages crawled")

print("Opening ", len(all_urls), " Urls...")

delay = args.open_tab_delay

if delay is None:
    delay = 1

open_urls(all_urls, delay)
print("All URLs opened")
