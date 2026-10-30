# Deviations from the submitted documents — final (end of Sprint 2)

The course brief (Part-2, 2i) asks us to record every difference between the code and the SRS / SAD / Test Plan and to explain it in the final demo.

**Planned change (not a deviation):** in Sprint 2 the team replaced the console interface with a website (React + TypeScript) served by the C++ program on the same computer. The SRS, SAD and Test Plan were updated to **v1.1** on 26 Oct 2026, *before* the change was coded (pull request "DOCS: SRS, SAD and Test Plan v1.1"), so the website itself is part of the baseline. Sprint 1 items are in `../sprint1/DEVIATIONS.md`.

## A. Status of the Sprint 1 deviations
| Sprint 1 item | Final status |
|---|---|
| D1 Sprint plan (2 sprints instead of 3) | **Resolved** — SAD v1.1 section 4.6 now shows the two sprints. |
| D2 No checksum line in data files | **Resolved** in Sprint 2 (HMS-SEC-007). |
| D3 IDs from the highest existing ID instead of `counters.dat` | Kept. |
| D4 No fsync before rename | Kept: protects against the program being killed (HMS-NF-005), not against power loss. |
| D5 No file lock (E043) | Not built: known limitation. The web server itself refuses to start if port 8080 is already used by another HMS. |
| D6 One console menu file per feature | No longer applies: the console was removed in v1.1. The website follows the same idea (one page and one route file per feature). |
| D7 Escaping of `|` and `\` only | Kept. |
| D8 Audit log written from Sprint 1 | Completed in Sprint 2 with the viewer page (HMS-F-062). |
| D9 Restore never rolls back `audit.log` | Kept. |
| D10 No "System status" screen | Not built: known limitation (SAD v1.1 4.4 marks it as a later release). |
| D11 CMake picks up source files automatically | Kept. |
| D12 Extra "List a patient's appointments" screen | Kept (Appointments page, "Cancel / reschedule / no-show" tab). |

## B. Sprint 2 deviations from SRS / SAD / Test Plan v1.1
| # | Document and section | What the document says | What we built | Why |
|---|---|---|---|---|
| E1 | SRS HMS-SEC-007 | Checksum verified at load | A file **without** a checksum line is also refused (E040) | Otherwise deleting the line would bypass the check. Test data saved by the Sprint 1 build must be recreated. |
| E2 | SRS HMS-SEC-007 | E040 shown to the Administrator, restore offered | The login page shows E040 and asks for an Administrator login **checked against the users file inside the backup** | The damaged file may be `users.dat` itself. |
| E3 | SRS HMS-F-005 | 10 minutes without activity end the session | Checked twice: in the browser (mouse / keyboard activity) and on the server for every request (E003) | The server check works even if the browser tab is left open or closed. |
| E4 | SRS HMS-F-043, Test Plan TC-CON-03 | No edit or delete of consultations | `editConsultation()` / `deleteConsultation()` exist in the C++ service **only to refuse** (E032 / E004); the web API has no such route | TC-CON-03 calls them through the test harness and expects a refusal. |
| E5 | SAD 4.3.8 | Web API | Requests are handled one at a time (one lock) | The services were designed single-threaded (SAD 3.5); one user works at a time (SRS 2.6). |
| E6 | Test Plan TC-PERF-02 | 95% of operations within 1 s | Checked in the normal build; in the AddressSanitizer build (3–5× slower) the limit is 3 s | Sanitizer timings do not represent the reference machine. |
| E7 | Test Plan TC-MNT-01 | gcov / lcov, Valgrind and AddressSanitizer | gcovr for coverage, AddressSanitizer + LeakSanitizer for memory; no Valgrind | HMS-NF-009 accepts either; one tool runs on every PR. |
| E8 | SRS HMS-NF-008 | Every function at most 50 lines | Checked by `tools/check_function_length.py` for C++ in `src/` **and** `tests/` | Stricter than required; the website is checked by the TypeScript compiler and page tests instead. |
| E9 | SAD 4.3.3 | Status changes | `changeStatus()` handles Cancelled and No-show; Completed is set only when a consultation is saved | Keeps Scheduled → Completed tied to the clinical record (HMS-F-035, F-040). |
| E10 | SRS HMS-F-021 | Weekly availability | The Doctors page edits one working block per weekday; the C++ service and web API accept several | Enough for an OPD timetable; more blocks can be added later without changing the server. |

## C. Process notes (how the work was done)
| PR | What happened | Why |
|---|---|---|
| #78 (TASK-02) | Opened and merged by Pranav; Saurav is the co-author of the commit. No review was recorded. | Saurav's laptop set-up was not working on Monday 19 Oct, and the skeleton was needed by everyone that morning. |
| #79 | Small extra fix ("mark unused Sprint 1 skeleton fields") without a backlog ID or recorded review | Placeholder code caused compiler warnings on macOS (clang); the placeholders were replaced later in Sprint 1. |
| #81 (TASK-01) | Merged without a recorded review or CI checks | It was opened before the CI workflow (PR #80) was on `main`. |

From Tuesday 20 Oct onwards every pull request was raised by its assignee, approved by another member, and passed all GitHub Actions checks before merging, unless a note above says otherwise.
