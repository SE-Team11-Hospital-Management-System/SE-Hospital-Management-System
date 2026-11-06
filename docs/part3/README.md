# Part-3 test reports

Made on 2026-11-06 from `main` at commit `079cd42` (product code); report branch commit `d201a90` adds only this generator.

| Report | What it shows | Headline |
|---|---|---|
| [Unit Test Report](UNIT_TEST_REPORT.md) | every automated test and its result | 119 of 119 tests passed |
| [System Validation Report](SYSTEM_VALIDATION_REPORT.md) | every SRS requirement and how it was verified | 55 of 55 requirements verified |
| [Code Coverage Report](CODE_COVERAGE_REPORT.md) | lines and functions run by the tests | C++ lines 97.1%, service layer 97.6% |
| [Branch Coverage Report](BRANCH_COVERAGE_REPORT.md) | branches of every decision taken by the tests | C++ branches 71.4% |

Clickable coverage: `coverage/cpp/index.html` (C++) and `coverage/web/index.html` (website). GitHub shows HTML files as text, so download the repo (Code -> Download ZIP) and open them in a browser. Raw results: `results/`.

To make these reports again: push a branch named `docs-part3-...`, or run the *Test reports* workflow from the Actions tab, or run `python3 tools/make_test_reports.py` on a computer with CMake, g++, gcovr and Node.js.
