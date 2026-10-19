# Hospital Management System (HMS) — Team 11, Section F

Software Engineering mini-project (UE24CS341A), PES University.
A console program in C++17 that manages patient records, doctors and appointments for a hospital OPD.

| Member | GitHub | Role | Feature owned |
|---|---|---|---|
| Paras Agarwal (PES1UG24CS316) | @ParasAgarwal23 | Product Owner / Team Rep | SF-1 Login, roles, user accounts, security |
| Saurav | @SauravS9 | Dev Lead / Architecture owner | SF-2 Patient records |
| Pranav | @PranavdaSupreme1 | Design owner | SF-3 Doctors and appointments |
| Shristi Saha | @shristiglitter0820 | QA / Test owner | SF-4 Data storage, backup, (Sprint 2) consultations and reports |

## Documents (`docs/`)
- `HMS_SRS_v1.0.docx` — Software Requirements Specification
- `HMS_SAD_v1.0.docx` — Software Architecture and Design Specification
- `HMS_Test_Plan_v1.0.docx` — Software Test Plan (with test cases)
- `SETUP.md` — how to install the tools and build on your laptop
- `sprint1/` — Sprint 1 test report, deviations from the documents, demo video

## Build and run
```bash
cmake -S . -B build
cmake --build build
./build/hms            # Windows: build\hms.exe
```
Run the tests:
```bash
ctest --test-dir build --output-on-failure
```
Data is kept in `./data` (created on first run) and backups in `./backup`. Both folders are ignored by Git.

## How we work
Every change goes through its own branch and a pull request (PR) raised by the member the backlog item is assigned to.
A second team member reviews and merges it after the GitHub Actions checks pass.
