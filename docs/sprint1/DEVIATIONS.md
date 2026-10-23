# Deviations from the submitted documents (Sprint 1)

The course brief (Part-2, 2i) asks us to record every difference between the code and the SRS / SAD / Test Plan and explain it in the final demo.

| # | Document and section | What the document says | What we built | Why | Plan |
|---|---|---|---|---|---|
| D1 | SAD 4.6 Sprint plan | Three sprints: 07–21 Oct, 22 Oct–4 Nov, 5–18 Nov | Two sprints: 19–23 Oct and 26–30 Oct, as in the GitHub backlog | The course fixed the sprint dates after the SAD was submitted | Final SAD revision will copy the backlog plan |
| D2 | SAD 4.1 / 3.x Persistence | Last line of every data file is `#SHA256=<hex>` | No checksum line yet | HMS-SEC-007 is a Sprint-2 backlog item (Saurav) | Added in Sprint 2 |
| D3 | SAD 4.1 ID generation | Counters kept in `counters.dat` | Next ID = highest existing ID + 1, worked out when the file is loaded | Same result with one file less; records are never physically deleted, so IDs are never reused | Keep; update SAD |
| D4 | SAD 4.1 Atomic write step 2 | flush **and fsync** before rename | flush + rename (no fsync) | Standard C++ streams have no fsync; this protects against the program being killed (NF-005), not against power loss | Optional: add fsync with platform code in Sprint 2 |
| D5 | SAD C14 / R5, error E043 | File lock stops two copies of HMS using one data folder | Not built | Not in the Sprint-1 backlog | Sprint 2 if time allows, else listed as a known limitation |
| D6 | SAD C1 file names | `console_ui.cpp`, `menu_controller.cpp` | `console_ui.cpp` + one menu file per feature (`app_shell.cpp`, `patient_menu.cpp`, `doctor_menu.cpp`, `appointment_menu.cpp`, `backup_menu.cpp`) | Each owner edits only their own file, so four people can work in parallel without merge conflicts | Keep; update SAD |
| D7 | SAD persistence format | `\|` and `\n` escaped inside fields | `\|` and `\\` escaped; newlines can never be stored | The validator rejects control characters (HMS-SEC-005), so a newline can never reach a file | Keep |
| D8 | SRS HMS-SEC-003 (Sprint 2) | Audit logging | Audit entries are already written in Sprint 1; the viewer (HMS-F-062) comes in Sprint 2 | Every service needed to log from the start; adding it later would mean touching every file again | — |
| D9 | SAD restore API | Restore copies the backup over the data folder | Restore copies everything **except** `audit.log` | Rolling back the audit log would hide who did what (HMS-SEC-003) | Keep; update SAD |
| D10 | SAD Monitoring | Administrator "System status" screen | Not built | Not in any backlog item | Known limitation |
| D11 | CMakeLists in SAD | Source files listed one by one | `file(GLOB_RECURSE ...)` picks up every file under `src/` | Nobody has to edit the shared build file when adding code | Keep |
| D12 | Appointments screen | — | Extra option "List a patient's appointments" | Needed to show bookings in the demo before the schedule view (HMS-F-034, Sprint 2) | Keep |

Add a row whenever a PR's "Deviations" section is not "None".
