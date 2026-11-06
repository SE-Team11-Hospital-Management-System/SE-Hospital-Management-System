# Deviations from the SRS, SAD and Test Plan — final list for the demo

This is the single place that lists every difference between the delivered HMS and the documents
(SRS v1.1, SAD v1.1, Test Plan v1.2), with the reason and the current status. The detailed notes written during the
sprints are in `sprint1/DEVIATIONS.md` and `sprint2/DEVIATIONS.md`; the test-case changes are also in Test Plan v1.2, section 16.

**Planned change, not a deviation:** in Sprint 2 the console interface was replaced by a website (React + TypeScript)
served by the C++ program on the same computer. The SRS, SAD and Test Plan were updated to v1.1 on 26 Oct 2026, before
the change was coded (PR #98), so the website is part of the baseline.

**Status words:** *Kept* = we built it differently on purpose and keep it; *Resolved* = the document was updated, nothing
differs any more; *Not built* = known limitation, not tested.

## A. Design and requirements (found in Sprint 1)
| # | Document, section | Document says | What HMS does | Why | Status |
|---|---|---|---|---|---|
| D1 | SAD 4.6 sprint plan | 3 sprints (07 Oct – 18 Nov) | 2 sprints: 19–23 Oct and 26–30 Oct | The course fixed the sprint dates after the SAD was submitted | Resolved (SAD v1.1) |
| D2 | SAD 4.1 persistence | Last line of each data file is `#SHA256=<hex>` | Missing in Sprint 1, added in Sprint 2 (HMS-SEC-007) | It was a Sprint 2 backlog item | Resolved |
| D3 | SAD 4.1 ID generation | Counters kept in `counters.dat` | Next ID = highest stored ID + 1, worked out at load | Same result with one file less; records are never deleted, so IDs are never reused | Kept |
| D4 | SAD 4.1 atomic write | flush **and fsync** before rename | flush + rename, no fsync | Standard C++ has no fsync; temp file + rename still protects against a killed program (HMS-NF-005), not against power loss | Kept |
| D5 | SAD C14 / R5, error E043 | File lock stops two copies of HMS using one data folder | Not built; a second HMS on the same port refuses to start | Not in any backlog item | Not built |
| D6 | SAD C1 file names | `console_ui.cpp`, `menu_controller.cpp` | One page and one route file per feature | Each owner edits only their own files; the console itself was removed in v1.1 | Resolved (no longer applies) |
| D7 | SAD persistence format | `\|` and `\n` escaped in fields | `\|` and `\\` escaped; newlines can never be stored | The validator rejects control characters (HMS-SEC-005) | Kept |
| D8 | SRS HMS-SEC-003 | Audit logging in Sprint 2 | Audit entries written from Sprint 1; viewer page in Sprint 2 | Every service had to log from the start | Resolved |
| D9 | SAD restore | Restore copies the whole backup | Restore copies everything **except** `audit.log` | Rolling back the audit log would hide who did what (HMS-SEC-003) | Kept |
| D10 | SAD 4.4 monitoring | Administrator "System status" screen | Not built | Not in any backlog item (SAD v1.1 marks it as a later release) | Not built |
| D11 | SAD build | Source files listed one by one in CMake | `file(GLOB_RECURSE ...)` picks up every file under `src/` | Nobody edits the shared build file when adding code | Kept |
| D12 | Appointments screen | — | Extra "List a patient's appointments" view | Needed to cancel / reschedule by patient | Kept |

## B. Design and requirements (found in Sprint 2)
| # | Document, section | Document says | What HMS does | Why | Status |
|---|---|---|---|---|---|
| E1 | SRS HMS-SEC-007 | Checksum checked at load | A file **without** a checksum line is also refused (E040) | Otherwise deleting the line would skip the check | Kept |
| E2 | SRS HMS-SEC-007 | E040 shown, restore offered | Recovery login is checked against `users.dat` **inside the backup** | The damaged file may be `users.dat` itself | Kept |
| E3 | SRS HMS-F-005 | 10 idle minutes end the session | Checked in the browser and on the server for every request (E003) | Works even if the tab is left open or closed | Kept |
| E4 | SRS HMS-F-043, TC-CON-03 | No edit or delete of consultations | `editConsultation()` / `deleteConsultation()` exist only to refuse; the web API has no such route | TC-CON-03 calls them and expects a refusal | Kept |
| E5 | SAD 4.3.8 web API | Web API | Requests handled one at a time (one lock) | Services are single-threaded by design (SAD 3.5); one user at a time (SRS 2.6) | Kept |
| E6 | Test Plan TC-PERF-02 | 95% of operations within 1 s | 1 s in the normal build, 3 s in the AddressSanitizer build | Sanitizer builds run 3–5 times slower | Resolved (Test Plan v1.2) |
| E7 | Test Plan TC-MNT-01, SAD tools | gcov / lcov and Valgrind | gcovr (line and branch), AddressSanitizer + LeakSanitizer | HMS-NF-009 accepts either; one tool runs on every PR | Resolved (Test Plan v1.2); the SAD still names lcov / Valgrind (see D) |
| E8 | SRS HMS-NF-008 | Functions at most 50 lines | Checked for `src/` **and** `tests/` | Stricter than required | Kept |
| E9 | SAD 4.3.3 status changes | Generic status change | `changeStatus()` does Cancelled / No-show; Completed only when a consultation is saved | Keeps Completed tied to the clinical record (HMS-F-035, F-040) | Kept |
| E10 | SRS HMS-F-021 | Weekly availability | The Doctors page edits one block per weekday; server and API accept several | Enough for an OPD timetable | Kept |

## C. Testing (Part-3)
| # | Document, section | Document says | What was done | Why | Status |
|---|---|---|---|---|---|
| T1 | Test Plan v1.1 TC-SEC-02 step 4 | Fuzz script with 1,000 random inputs | Over-long, illegal and non-JSON input sent to the web API | All input now arrives through the web API; it checks every field | Resolved (Test Plan v1.2 §16) |
| T2 | Test Plan v1.1 TC-DATA-01 | Kill the program during a write | Write failure injected (blocked temporary file) and a full restart | Temp file + rename makes a killed write equal to a failed one (D4) | Resolved (Test Plan v1.2 §16) |
| T3 | Test Plan v1.1 section 14 | Line coverage only | Branch coverage added, target 60% of all C++ code; exception-only and unreachable branches are left out | Part-3 asks for a branch coverage report | Resolved (Test Plan v1.2) |
| T4 | Part-3 brief, item 3 | Optional Sprint-3 | No Sprint-3 | Nothing spilled over after Sprint 2; D5 and D10 stay known limitations | — |
| T5 | — | Development frozen after Sprint 2 | In Part-3 only tests and documents were added (`tests/test_sf1_paths.cpp` and four website test files in `web/src/test/`); no product code changed | More error paths and every page tested, higher coverage | — |

## D. Documents not edited again after v1.1
SRS v1.1 and SAD v1.1 still mention `counters.dat`, fsync, the file lock, lcov and Valgrind. Those places are superseded by
D3, D4, D5 and E7 above; this file is the record of the difference.

## E. Process notes
| PR | What happened | Why |
|---|---|---|
| #78 (TASK-02) | Opened and merged by Pranav, Saurav co-author, no recorded review | Saurav's laptop set-up failed on 19 Oct and everyone needed the skeleton that morning |
| #79 | Small build fix without a backlog ID or recorded review | Placeholder code gave compiler warnings on macOS (clang) |
| #81 (TASK-01) | Merged without a recorded review or CI checks | It was opened before the CI workflow (PR #80) reached `main` |

Every other pull request was raised by its assignee, approved by another member and passed all GitHub Actions checks before merging.
