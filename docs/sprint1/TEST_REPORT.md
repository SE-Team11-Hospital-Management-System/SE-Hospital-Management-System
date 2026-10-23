# Sprint 1 — Test report

| | |
|---|---|
| Build tested | commit `5b5b430` on `main` |
| Date | 2026-10-23 |
| Tested by | Shristi Saha (QA / Test owner) |
| Automated tests | 29 / 29 passed (GoogleTest, run with `ctest`) |
| Also run on every PR by GitHub Actions | Linux g++, Windows MSYS2 g++, AddressSanitizer + UBSan |

## Automated test cases
Each GoogleTest name starts with the Test Plan ID it implements.

| Test Plan ID | Requirement(s) | GoogleTest name | Result |
|---|---|---|---|
| TC-AUTH-01 | HMS-F-001, SEC-004 | AuthFixture.TC_AUTH_01_ValidLogin | Pass |
| TC-AUTH-02 | HMS-F-002, SEC-008 | AuthFixture.TC_AUTH_02_LockoutAndGenericError | Pass |
| TC-AUTH-03 (Sprint-1 part) | HMS-F-003 | SecurityCore.TC_AUTH_03_RoleMatrix | Pass |
| TC-AUTH-04 | HMS-F-004 | AuthFixture.TC_AUTH_04_UserManagement | Pass |
| TC-SEC-01 | HMS-SEC-001 | SecurityCore.TC_SEC_01_SaltedHash, AuthFixture.TC_SEC_01_PasswordsStoredOnlyAsSaltedHash | Pass |
| TC-PAT-01 | HMS-F-010 | PatientFixture.TC_PAT_01_RegisterAndReload | Pass |
| TC-PAT-02 | HMS-F-011 | PatientFixture.TC_PAT_02_Validation | Pass |
| TC-PAT-03 | HMS-F-015 | PatientFixture.TC_PAT_03_DuplicateNeedsConfirmation | Pass |
| TC-PAT-04 | HMS-F-012 | PatientFixture.TC_PAT_04_SearchAndPaging | Pass |
| TC-PAT-05 | HMS-F-013 | PatientFixture.TC_PAT_05_UpdateWithAudit | Pass |
| TC-SEC-02 | HMS-SEC-005, F-011 | PatientFixture.TC_SEC_02_LongInputAndRole, Skeleton.ValidatorRules | Pass |
| TC-DOC-01 | HMS-F-020, F-021, F-022 | DoctorFixture.TC_DOC_01_ProfilesAvailabilityAndList | Pass |
| TC-APT-01 | HMS-F-030 | AppointmentFixture.TC_APT_01_BookValidSlot | Pass |
| TC-APT-02 | HMS-F-031 | AppointmentFixture.TC_APT_02_ConflictsAndAlternatives | Pass |
| TC-DATA-01 | HMS-F-060, NF-005, NF-011 | FileStoreFixture.TC_DATA_01_AtomicWriteAndLoad, FileStoreFixture.TC_DATA_01_WriteFailureKeepsOldData, DataFixture.TC_DATA_01_FullRestartAndFailedSave | Pass |
| TC-DATA-02 | HMS-F-061 | BackupFixture.TC_DATA_02_BackupAndRestore | Pass |
| TC-PERF-02 | HMS-NF-003 | BackupFixture.TC_PERF_02_StartupUnder5Seconds (measured: 0.295529 s, limit 5 s) | Pass |
| TC-SEC-05 | HMS-SEC-006 | DataFixture.TC_SEC_05_OwnerOnlyPermissions (Linux / macOS only) | Pass |

## Manual checks (run in the program by the team on Thursday)

| Check | Expected | Result |
|---|---|---|
| Start with no data folder | First-run screen asks for an Administrator | Pass |
| Typing a password | Nothing appears on screen | Pass |
| Log in as Doctor | Menu shows only Patients (search) and Doctors (list) | Pass |
| Register with phone `98765` | `ERROR E010: Phone: must be exactly 10 digits.` | Pass |
| Book a slot that is already taken | `ERROR E030` with 3 free slots; choosing one books it | Pass |
| Close the program and start it again | All saved records are still there | Pass |
| Backup, add a patient, restore | The added patient is gone after restore | Pass |

## Not tested in Sprint 1 (Sprint 2 items)
Session time-out (F-005), change password (F-006), audit log viewer (F-062), patient deactivation (F-014),
cancel / reschedule (F-032, F-033), schedules (F-034), consultations and reports (F-040 to F-051),
data-file checksums (SEC-007), response-time tests (NF-001, NF-002).
