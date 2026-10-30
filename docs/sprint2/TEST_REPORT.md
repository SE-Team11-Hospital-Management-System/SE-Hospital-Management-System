# Sprint 2 — Final test report

| | |
|---|---|
| Build tested | commit `a72da63` on `main` (development frozen) |
| Date | 2026-10-30 |
| Tested by | Shristi Saha (QA / Test owner) |
| Automated tests (C++ and web API) | 69 / 69 passed (GoogleTest, run with `ctest`) |
| Website page tests | 15 passed (15) (Vitest) |
| Also run on every PR by GitHub Actions | Ubuntu g++ 13, Ubuntu 22.04 g++ 11, Windows MSYS2 g++, AddressSanitizer + LeakSanitizer, quality job (function length, cppcheck, coverage), website job (type check, page tests, build) |
| Function-length check | OK: every function has at most 50 lines (HMS-NF-008) |
| GitHub Actions on this commit | Website (type check, tests, build): success; Build + unit tests (Ubuntu 22.04, g++ 11): success; Build + unit tests (Windows, MSYS2 g++): success; Build + unit tests (Linux, g++): success; Tests with AddressSanitizer: success; Quality (function length, cppcheck, coverage): success |

## Automated test cases (each GoogleTest name starts with the Test Plan ID)

| Test Plan ID | Requirement(s) | GoogleTest name(s) | Result |
|---|---|---|---|
| TC-AUTH-01 | HMS-F-001, SEC-004 | AuthFixture.TC_AUTH_01_ValidLogin | Pass |
| TC-AUTH-02 | HMS-F-002, SEC-008 | AuthFixture.TC_AUTH_02_LockoutAndGenericError | Pass |
| TC-AUTH-03 | HMS-F-003, F-042, SEC-002 | SecurityCore.TC_AUTH_03_RoleMatrix, RoleEnforcement.TC_AUTH_03_EveryServiceDeniesForbiddenRoles, RoleEnforcement.TC_AUTH_03_MenuMatrix, AccessCheck.DeniedRoleIsE004AndAuditLogged | Pass |
| TC-AUTH-04 | HMS-F-004 | AuthFixture.TC_AUTH_04_UserManagement | Pass |
| TC-AUTH-05 | HMS-F-006 | Password.TC_AUTH_05_ChangePassword | Pass |
| TC-PAT-01 | HMS-F-010 | PatientFixture.TC_PAT_01_RegisterAndReload | Pass |
| TC-PAT-02 | HMS-F-011 | PatientFixture.TC_PAT_02_Validation | Pass |
| TC-PAT-03 | HMS-F-015 | PatientFixture.TC_PAT_03_DuplicateNeedsConfirmation | Pass |
| TC-PAT-04 | HMS-F-012 | PatientFixture.TC_PAT_04_SearchAndPaging | Pass |
| TC-PAT-05 | HMS-F-013 | PatientFixture.TC_PAT_05_UpdateWithAudit | Pass |
| TC-PAT-06 | HMS-F-014 | PatientDeactivate.TC_PAT_06_AdminOnlyNeverDeleted | Pass |
| TC-DOC-01 | HMS-F-020, F-021, F-022 | DoctorFixture.TC_DOC_01_ProfilesAvailabilityAndList | Pass |
| TC-APT-01 | HMS-F-030 | AppointmentFixture.TC_APT_01_BookValidSlot | Pass |
| TC-APT-02 | HMS-F-031 | AppointmentFixture.TC_APT_02_ConflictsAndAlternatives | Pass |
| TC-APT-03 | HMS-F-032 | StatusFixture.TC_APT_03_CancelFreesSlot | Pass |
| TC-APT-04 | HMS-F-033 | MoveFixture.TC_APT_04_AtomicReschedule | Pass |
| TC-APT-05 | HMS-F-034 | MoveFixture.TC_APT_05_DailySchedule | Pass |
| TC-APT-06 | HMS-F-035 | StatusFixture.TC_APT_06_StatusTransitions | Pass |
| TC-CON-01 | HMS-F-040, NF-010 | ConsultFixture.TC_CON_01_RecordConsultation, ConsultationRules.ValidationAndAge | Pass |
| TC-CON-02 | HMS-F-041, F-042 | HistoryFixture.TC_CON_02_HistoryAndPrivacy | Pass |
| TC-CON-03 | HMS-F-043 | HistoryFixture.TC_CON_03_ImmutableWithAddendum | Pass |
| TC-RPT-01 | HMS-F-050 | ReportFixture.TC_RPT_01_DailyReportAndCsv | Pass |
| TC-RPT-02 | HMS-F-051 | ReportFixture.TC_RPT_02_Workload | Pass |
| TC-DATA-01 | HMS-F-060, NF-005, NF-011 | FileStoreFixture.TC_DATA_01_AtomicWriteAndLoad, FileStoreFixture.TC_DATA_01_WriteFailureKeepsOldData, DataFixture.TC_DATA_01_FullRestartAndFailedSave | Pass |
| TC-DATA-02 | HMS-F-061 | BackupFixture.TC_DATA_02_BackupAndRestore | Pass |
| TC-SEC-01 | HMS-SEC-001 | SecurityCore.TC_SEC_01_SaltedHash, AuthFixture.TC_SEC_01_PasswordsStoredOnlyAsSaltedHash | Pass |
| TC-SEC-02 | HMS-SEC-005, F-011 | PatientFixture.TC_SEC_02_LongInputAndRole, Skeleton.ValidatorRules | Pass |
| TC-SEC-03 | HMS-SEC-003, F-062 | AuditCompleteness.TC_SEC_03_EntriesAndContent, AuditCompleteness.LockoutLogged, AuditViewer.TC_SEC_03_FilterByDateAndUser | Pass |
| TC-SEC-04 | HMS-F-005 | Session.TC_SEC_04_IdleTimeout, AccessCheck.UnknownOrEndedSessionIsE003 | Pass |
| TC-SEC-05 | HMS-SEC-006 | DataFixture.TC_SEC_05_OwnerOnlyPermissions (Linux / macOS only) | Pass |
| TC-SEC-06 | HMS-SEC-007 | Integrity.TC_SEC_06_TamperDetectedAndRestored, Integrity.ChecksumLineWrittenAndChecked, WebApi.TC_SEC_06_RecoveryModeAndRestore | Pass |
| TC-WEB-01 | HMS-F-070, F-003 | WebApi.StatusLoginAndSession, WebApi.DoctorLoginCarriesOwnProfile, WebApi.FirstRunSetupOnlyOnce, WebApi.BadJsonIsE010, WebApi.ServesTheWebsiteAndFallsBackToIndex + website page tests | Pass |
| TC-WEB-02 | HMS-F-071 | WebApi.PasswordUsersAndAudit | Pass |
| TC-WEB-03 | HMS-F-072 | WebApi.PatientsRegisterSearchUpdateDeactivate | Pass |
| TC-WEB-04 | HMS-F-073 | WebApi.DoctorsAppointmentsAndSchedule | Pass |
| TC-WEB-05 | HMS-F-074, NF-010 | WebApi.ConsultationsNeedConfirmationAndStayPrivate, WebApi.ReportsCsvAndBackups | Pass |
| TC-SEC-07 | HMS-SEC-009 | WebSecurity.TC_SEC_07_TokensAreRandomAndChecked, WebSecurity.TC_SEC_07_ServerListensOnLoopbackOnly, WebSecurity.PasswordsNeverReturned, WebApi.RefusesRequestsFromOtherSites | Pass |
| TC-PERF-01 | HMS-NF-001, NF-004 | Performance.TC_PERF_01_SearchAtFullCapacity (95th percentile 2.46 ms, limit 2000 ms) | Pass |
| TC-PERF-02 | HMS-NF-002, NF-003 | Performance.TC_PERF_02_BookCancelRescheduleAtFullLoad (95th percentile 100.57 ms, limit 1000 ms), BackupFixture.TC_PERF_02_StartupUnder5Seconds (0.36 s, limit 5 s) | Pass |
| TC-PORT-01 | HMS-NF-007 | Same commit built and tested by GitHub Actions on Ubuntu 22.04 (g++ 11), Ubuntu (g++ 13) and Windows (MSYS2 g++); website in two browsers (manual check 9) | See "GitHub Actions on this commit" above |
| TC-MNT-01 | HMS-NF-008, NF-009 | GitHub Actions jobs "Quality" (function length, cppcheck, coverage ≥ 70%) and "Tests with AddressSanitizer" (leaks and memory errors) | See "GitHub Actions on this commit" above |
| TC-USA-01 | HMS-NF-006 | 5 first-time volunteers, 20 timed tasks | See `USABILITY.md` |

## Manual checks in the website (done by the team on Thursday 29 Oct; answered by the tester, not generated)

| Check | Expected | Result |
|---|---|---|
| Open http://localhost:8080 without logging in, try /patients | Only the login page is shown | Pass |
| Leave the website alone for 10 minutes, then click anything | "Session expired" message and back to the login page | Pass |
| Change my password, log out, log in | Old password refused, new one works | Pass |
| Doctor records a consultation and answers No | Nothing is saved; answering Yes saves it and the appointment shows Completed | Pass |
| Log in as Receptionist | No Consultations, User accounts, Audit log or Backup page in the sidebar | Pass |
| Book a taken slot, click one of the offered free slots | E030 with 3 slot buttons; one click books | Pass |
| Reports, Download CSV | The file opens in a spreadsheet with a header row | Pass |
| Change one letter in data/patients.dat, restart HMS | The login page shows E040 and an Administrator can restore the latest backup | Pass |
| Open the website in a second browser (two of Chrome, Edge, Firefox) | Login and the Patients page work the same in both | Pass |

## Known limitations (not tested)
Single-instance file lock (E043) and the Administrator "System status" page are not built (see `DEVIATIONS.md`, D5 and D10).
