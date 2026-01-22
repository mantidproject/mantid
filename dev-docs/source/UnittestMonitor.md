# Unittest Monitor

To monitor flakey and failing unittests from the Jenkins nightly
pipelines, use the [Unittest
Monitor](https://mantidproject.github.io/unittest-monitor). The Unittest
Monitor ([github](https://mantidproject.github.io/unittest-monitor/)) is
a django site which updates via a github action every day. It searches
for new nightly runs, fetches the test logs from the Jenkins artifacts,
and records them in the database hosted in the repo.
