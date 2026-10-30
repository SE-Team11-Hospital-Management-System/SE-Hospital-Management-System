# Hospital Management System (HMS) — Team 11, Section F

Software Engineering mini-project (UE24CS341A), PES University.
HMS manages patient records, doctors, appointments, consultations and reports for a hospital OPD.
A C++17 program holds all data and rules; staff use it through a website (React + TypeScript) in the browser of the same computer.

| Member | GitHub | Role | Feature owned |
|---|---|---|---|
| Paras Agarwal (PES1UG24CS316) | @ParasAgarwal23 | Product Owner / Team Rep | SF-1 Login, roles, user accounts, audit log, security |
| R Saurav Srinivas (PES1UG24CS352) | @SauravS9 | Dev Lead / Architecture owner | SF-2 Patient records, website foundation, data integrity, quality |
| Pranav Chandrasekar (PES1UG24CS332) | @PranavdaSupreme1 | Design owner | SF-3 Doctors, appointments, schedules |
| Shristi Saha (PES1UG24CS903) | @shristiglitter0820 | QA / Test owner | SF-4 Data storage, backup, consultations, reports |

## What it does
- Login with three roles (Administrator, Receptionist, Doctor); each role sees only its own pages.
- Patients: register, search, update, deactivate (never deleted).
- Doctors: profiles, weekly working hours, list by specialization.
- Appointments: book without double-booking, cancel, reschedule, mark no-show, daily schedule per doctor.
- Consultations (Doctors only): diagnosis, prescription, notes, history, addenda; saved records cannot be changed.
- Reports: daily appointment report with CSV download, doctor workload.
- Security: salted password hashing, 10-minute idle logout, append-only audit log with a viewer, checksums on every
  data file, backup and restore, server reachable only from the same computer (127.0.0.1).

## How it is built
| Part | Folder | Technology |
|---|---|---|
| Business rules, data, security | `src/` (services, security, data) | C++17 |
| Web API (local HTTP server) | `src/api/` | C++17, cpp-httplib, nlohmann/json |
| Website | `web/` | React 18 + TypeScript, Mantine, Vite |
| Tests | `tests/`, `web/src/test/` | GoogleTest (C++ and web API), Vitest (pages) |

## Documents (`docs/`)
- `HMS_SRS.docx` — Software Requirements Specification (v1.1)
- `SAD.docx` — Software Architecture and Design Specification (v1.1)
- `HMS_Test_Plan.docx` — Software Test Plan with test cases (v1.1)
- `SETUP.md` — how to install the tools
- `USER_GUIDE.md` — how to use HMS
- `sprint1/`, `sprint2/` — test reports, usability results, deviations, demo videos

## Build and run
```bash
cmake -S . -B build && cmake --build build          # the C++ program and its tests
cd web && npm ci && npm run build && cd ..           # the website (into web/dist)
./build/hms                                          # Windows: build\hms.exe
```
Then open http://localhost:8080. Data is kept in `./data`, backups in `./backup`, CSV reports in `./reports`
(all created on first use and ignored by Git).

Tests: `ctest --test-dir build --output-on-failure` (C++ and web API) and `cd web && npm test` (pages).

## Quality checks (GitHub Actions, on every pull request)
- C++ build and tests on Ubuntu (g++ 13 and g++ 11) and Windows (MSYS2 g++); all tests under AddressSanitizer + LeakSanitizer
- Website: TypeScript strict type check, page tests, production build
- No function longer than 50 lines (`tools/check_function_length.py`), cppcheck, service-layer line coverage of at least 70%

## How we work
Every change goes through its own branch and a pull request (PR) raised by the member the backlog item is assigned to.
A second team member reviews and merges it after the GitHub Actions checks pass.
