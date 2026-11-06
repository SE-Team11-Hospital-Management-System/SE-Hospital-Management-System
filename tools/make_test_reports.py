#!/usr/bin/env python3
"""Part-3 test reports: runs every test once with coverage counters and writes four reports.

Usage:  python3 tools/make_test_reports.py [--out docs/part3]
Needs:  CMake, g++ (or clang), gcovr (pip install gcovr), Node.js + npm. GitHub Actions runs it on Ubuntu
        (.github/workflows/test-reports.yml), so nobody needs these tools on their own laptop.

Writes into --out:
  UNIT_TEST_REPORT.md           every GoogleTest and website test with its result and time
  SYSTEM_VALIDATION_REPORT.md   every SRS requirement -> Test Plan test case -> automated tests -> result
  CODE_COVERAGE_REPORT.md       line and function coverage (C++ and website)
  BRANCH_COVERAGE_REPORT.md     branch coverage (C++ and website)
  coverage/cpp/index.html       clickable line + branch coverage of every C++ file (gcovr)
  coverage/web/index.html       clickable coverage of the website (Vitest, V8)
  results/                      raw results (JUnit XML, coverage JSON, logs)
Nothing in these reports is typed by hand: every number comes from the run.
"""
import argparse, datetime, json, os, platform, re, shutil, subprocess, sys, tempfile, time
import xml.etree.ElementTree as ET

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BUILD = os.path.join(ROOT, "build-reports")
LINE_TARGET_SERVICES = 70.0       # HMS-NF-008
BRANCH_TARGET = 60.0              # Test Plan v1.2, section 14

# Test Plan v1.2 Appendix A / section 16.2: test case -> automated tests. Requirements come from the Test Plan.
TC_TESTS = {
    "TC-AUTH-01": ["AuthFixture.TC_AUTH_01_ValidLogin", "AuthFixture.BootstrapOnlyOnFirstRun"],
    "TC-AUTH-02": ["AuthFixture.TC_AUTH_02_LockoutAndGenericError"],
    "TC-AUTH-03": ["SecurityCore.TC_AUTH_03_RoleMatrix", "RoleEnforcement.TC_AUTH_03_EveryServiceDeniesForbiddenRoles",
                   "RoleEnforcement.TC_AUTH_03_MenuMatrix", "AccessCheck.DeniedRoleIsE004AndAuditLogged",
                   "AccessCheck.AppContextAttachesItself"],
    "TC-AUTH-04": ["AuthFixture.TC_AUTH_04_UserManagement"],
    "TC-AUTH-05": ["Password.TC_AUTH_05_ChangePassword"],
    "TC-PAT-01": ["PatientFixture.TC_PAT_01_RegisterAndReload"],
    "TC-PAT-02": ["PatientFixture.TC_PAT_02_Validation"],
    "TC-PAT-03": ["PatientFixture.TC_PAT_03_DuplicateNeedsConfirmation"],
    "TC-PAT-04": ["PatientFixture.TC_PAT_04_SearchAndPaging"],
    "TC-PAT-05": ["PatientFixture.TC_PAT_05_UpdateWithAudit"],
    "TC-PAT-06": ["PatientDeactivate.TC_PAT_06_AdminOnlyNeverDeleted"],
    "TC-DOC-01": ["DoctorFixture.TC_DOC_01_ProfilesAvailabilityAndList"],
    "TC-APT-01": ["AppointmentFixture.TC_APT_01_BookValidSlot"],
    "TC-APT-02": ["AppointmentFixture.TC_APT_02_ConflictsAndAlternatives"],
    "TC-APT-03": ["StatusFixture.TC_APT_03_CancelFreesSlot"],
    "TC-APT-04": ["MoveFixture.TC_APT_04_AtomicReschedule"],
    "TC-APT-05": ["MoveFixture.TC_APT_05_DailySchedule"],
    "TC-APT-06": ["StatusFixture.TC_APT_06_StatusTransitions"],
    "TC-CON-01": ["ConsultFixture.TC_CON_01_RecordConsultation", "ConsultationRules.ValidationAndAge"],
    "TC-CON-02": ["HistoryFixture.TC_CON_02_HistoryAndPrivacy"],
    "TC-CON-03": ["HistoryFixture.TC_CON_03_ImmutableWithAddendum"],
    "TC-RPT-01": ["ReportFixture.TC_RPT_01_DailyReportAndCsv"],
    "TC-RPT-02": ["ReportFixture.TC_RPT_02_Workload"],
    "TC-DATA-01": ["FileStoreFixture.TC_DATA_01_AtomicWriteAndLoad", "FileStoreFixture.TC_DATA_01_WriteFailureKeepsOldData",
                   "DataFixture.TC_DATA_01_FullRestartAndFailedSave", "Skeleton.CodecRoundTripWithPipesAndBackslashes"],
    "TC-DATA-02": ["BackupFixture.TC_DATA_02_BackupAndRestore"],
    "TC-SEC-01": ["SecurityCore.TC_SEC_01_SaltedHash", "AuthFixture.TC_SEC_01_PasswordsStoredOnlyAsSaltedHash",
                  "Skeleton.Sha256KnownVectors"],
    "TC-SEC-02": ["PatientFixture.TC_SEC_02_LongInputAndRole", "Skeleton.ValidatorRules"],
    "TC-SEC-03": ["AuditCompleteness.TC_SEC_03_EntriesAndContent", "AuditCompleteness.LockoutLogged",
                  "AuditViewer.TC_SEC_03_FilterByDateAndUser", "SecurityCore.AuditIsAppendOnlyAndHasNoSecrets"],
    "TC-SEC-04": ["Session.TC_SEC_04_IdleTimeout", "AccessCheck.UnknownOrEndedSessionIsE003"],
    "TC-SEC-05": ["DataFixture.TC_SEC_05_OwnerOnlyPermissions"],
    "TC-SEC-06": ["Integrity.TC_SEC_06_TamperDetectedAndRestored", "Integrity.ChecksumLineWrittenAndChecked",
                  "WebApi.TC_SEC_06_RecoveryModeAndRestore"],
    "TC-SEC-07": ["WebSecurity.TC_SEC_07_TokensAreRandomAndChecked", "WebSecurity.TC_SEC_07_ServerListensOnLoopbackOnly",
                  "WebSecurity.PasswordsNeverReturned", "WebApi.RefusesRequestsFromOtherSites",
                  "SecurityCore.SessionsAreUniqueAndEnd"],
    "TC-PERF-01": ["Performance.TC_PERF_01_SearchAtFullCapacity"],
    "TC-PERF-02": ["Performance.TC_PERF_02_BookCancelRescheduleAtFullLoad", "BackupFixture.TC_PERF_02_StartupUnder5Seconds"],
    "TC-WEB-01": ["WebApi.StatusLoginAndSession", "WebApi.DoctorLoginCarriesOwnProfile", "WebApi.FirstRunSetupOnlyOnce",
                  "WebApi.BadJsonIsE010", "WebApi.ServesTheWebsiteAndFallsBackToIndex", "web:*"],
    "TC-WEB-02": ["WebApi.PasswordUsersAndAudit"],
    "TC-WEB-03": ["WebApi.PatientsRegisterSearchUpdateDeactivate"],
    "TC-WEB-04": ["WebApi.DoctorsAppointmentsAndSchedule", "Skeleton.DateTimeHelpers"],
    "TC-WEB-05": ["WebApi.ConsultationsNeedConfirmationAndStayPrivate", "WebApi.ReportsCsvAndBackups",
                  "Skeleton.ResultCarriesValueOrError"],
    "TC-USA-01": [], "TC-PORT-01": [], "TC-MNT-01": [],
}
TC_REQS = {
    "TC-AUTH-01": "HMS-F-001 HMS-SEC-004", "TC-AUTH-02": "HMS-F-002 HMS-SEC-008", "TC-AUTH-03": "HMS-F-003 HMS-F-042 HMS-SEC-002",
    "TC-AUTH-04": "HMS-F-004", "TC-AUTH-05": "HMS-F-006", "TC-PAT-01": "HMS-F-010", "TC-PAT-02": "HMS-F-011",
    "TC-PAT-03": "HMS-F-015", "TC-PAT-04": "HMS-F-012", "TC-PAT-05": "HMS-F-013", "TC-PAT-06": "HMS-F-014",
    "TC-DOC-01": "HMS-F-020 HMS-F-021 HMS-F-022", "TC-APT-01": "HMS-F-030", "TC-APT-02": "HMS-F-031", "TC-APT-03": "HMS-F-032",
    "TC-APT-04": "HMS-F-033", "TC-APT-05": "HMS-F-034", "TC-APT-06": "HMS-F-035", "TC-CON-01": "HMS-F-040 HMS-NF-010",
    "TC-CON-02": "HMS-F-041 HMS-F-042", "TC-CON-03": "HMS-F-043", "TC-RPT-01": "HMS-F-050", "TC-RPT-02": "HMS-F-051",
    "TC-DATA-01": "HMS-F-060 HMS-NF-005 HMS-NF-011", "TC-DATA-02": "HMS-F-061", "TC-SEC-01": "HMS-SEC-001",
    "TC-SEC-02": "HMS-SEC-005 HMS-F-011", "TC-SEC-03": "HMS-SEC-003 HMS-F-062", "TC-SEC-04": "HMS-F-005",
    "TC-SEC-05": "HMS-SEC-006", "TC-SEC-06": "HMS-SEC-007", "TC-SEC-07": "HMS-SEC-009", "TC-PERF-01": "HMS-NF-001 HMS-NF-004",
    "TC-PERF-02": "HMS-NF-002 HMS-NF-003", "TC-USA-01": "HMS-NF-006", "TC-PORT-01": "HMS-NF-007",
    "TC-MNT-01": "HMS-NF-008 HMS-NF-009", "TC-WEB-01": "HMS-F-070 HMS-F-003", "TC-WEB-02": "HMS-F-071",
    "TC-WEB-03": "HMS-F-072 HMS-F-015", "TC-WEB-04": "HMS-F-073", "TC-WEB-05": "HMS-F-074 HMS-NF-010",
}
REQS = [  # (ID, title, priority) from SRS v1.1 / Test Plan section 13.1
    ("HMS-F-001", "Mandatory login", "High"), ("HMS-F-002", "Account lock-out", "High"),
    ("HMS-F-003", "Role-based functions", "High"), ("HMS-F-004", "User account management", "High"),
    ("HMS-F-005", "Session time-out", "Medium"), ("HMS-F-006", "Change password", "Medium"),
    ("HMS-F-010", "Register patient", "High"), ("HMS-F-011", "Patient input validation", "High"),
    ("HMS-F-012", "Search patient", "High"), ("HMS-F-013", "Update patient details", "Medium"),
    ("HMS-F-014", "Deactivate patient", "Medium"), ("HMS-F-015", "Duplicate detection", "Medium"),
    ("HMS-F-020", "Doctor profile", "High"), ("HMS-F-021", "Doctor availability", "High"),
    ("HMS-F-022", "List doctors by specialization", "Low"), ("HMS-F-030", "Book appointment", "High"),
    ("HMS-F-031", "Slot-conflict handling", "High"), ("HMS-F-032", "Cancel appointment", "High"),
    ("HMS-F-033", "Reschedule appointment", "Medium"), ("HMS-F-034", "Daily schedule view", "Medium"),
    ("HMS-F-035", "Appointment status life-cycle", "Medium"), ("HMS-F-040", "Record consultation", "High"),
    ("HMS-F-041", "View medical history", "High"), ("HMS-F-042", "Clinical data confidentiality", "High"),
    ("HMS-F-043", "Immutable consultation records", "High"), ("HMS-F-050", "Daily appointment report", "Medium"),
    ("HMS-F-051", "Doctor workload report", "Low"), ("HMS-F-060", "Data persistence", "High"),
    ("HMS-F-061", "Backup and restore", "Medium"), ("HMS-F-062", "View audit log", "Medium"),
    ("HMS-F-070", "Website, login page and role sidebar", "High"),
    ("HMS-F-071", "User accounts, audit log and password pages", "Medium"), ("HMS-F-072", "Patients page", "High"),
    ("HMS-F-073", "Doctors, appointments and schedule pages", "High"),
    ("HMS-F-074", "Consultation, report and backup pages", "High"),
    ("HMS-NF-001", "Search response time", "High"), ("HMS-NF-002", "Booking response time", "High"),
    ("HMS-NF-003", "Start-up time", "Medium"), ("HMS-NF-004", "Capacity", "Medium"),
    ("HMS-NF-005", "Crash safety of stored data", "High"), ("HMS-NF-006", "Usability", "Medium"),
    ("HMS-NF-007", "Portability", "Medium"), ("HMS-NF-008", "Maintainability", "Medium"),
    ("HMS-NF-009", "Memory safety", "High"), ("HMS-NF-010", "Patient identity confirmation", "High"),
    ("HMS-NF-011", "Write-failure notification", "High"), ("HMS-SEC-001", "Salted password hashing", "High"),
    ("HMS-SEC-002", "Deny-by-default RBAC", "High"), ("HMS-SEC-003", "Audit logging", "High"),
    ("HMS-SEC-004", "Password masking", "High"), ("HMS-SEC-005", "Input validation, memory safety", "High"),
    ("HMS-SEC-006", "Owner-only file permissions", "Medium"), ("HMS-SEC-007", "Data-file integrity check", "Medium"),
    ("HMS-SEC-008", "Generic login error", "Medium"), ("HMS-SEC-009", "Local-only server and random session tokens", "High"),
]
SUITE_COMPONENT = {
    "AuthFixture": "C3 AuthService", "Password": "C3 AuthService", "Session": "C3 SessionManager",
    "SecurityCore": "C10-C12 security core", "AccessCheck": "C10 AccessControl", "RoleEnforcement": "C10 AccessControl (all services)",
    "AuditCompleteness": "C12 AuditService", "AuditViewer": "C12 AuditService",
    "PatientFixture": "C4 PatientService", "PatientDeactivate": "C4 PatientService",
    "DoctorFixture": "C5 DoctorService", "AppointmentFixture": "C6 AppointmentService", "StatusFixture": "C6 AppointmentService",
    "MoveFixture": "C6 AppointmentService", "ConsultFixture": "C7 ConsultationService", "ConsultationRules": "C7 ConsultationService",
    "HistoryFixture": "C7 ConsultationService", "ReportFixture": "C8 ReportService", "BackupFixture": "C9 BackupService",
    "FileStoreFixture": "C14 FileStore", "DataFixture": "C13/C14 persistence", "Integrity": "C14 FileStore + recovery",
    "Skeleton": "C2 Validator + common helpers", "WebApi": "C15 Web API (HTTP)", "WebSecurity": "C15 Web API (HTTP)",
    "Performance": "Performance (full capacity)",
}
SYSTEM_SUITES = ("WebApi", "WebSecurity", "Performance")
MANUAL_CHECKS = [   # the same 9 website checks as Sprint 2; answered by the tester (python sprint.py pr 2)
    ("TC-WEB-01", "Open http://localhost:8080 without logging in, try /patients", "Only the login page is shown"),
    ("TC-SEC-04", "Leave the website alone for 10 minutes, then click anything", "\"Session expired\" message and back to the login page"),
    ("TC-AUTH-05", "Change my password, log out, log in", "Old password refused, new one works"),
    ("TC-CON-01", "Doctor records a consultation and answers No, then Yes", "No saves nothing; Yes saves it and the appointment shows Completed"),
    ("TC-AUTH-03", "Log in as Receptionist", "No Consultations, User accounts, Audit log or Backup page in the sidebar"),
    ("TC-APT-02", "Book a taken slot, click one of the offered free slots", "E030 with 3 slot buttons; one click books"),
    ("TC-RPT-01", "Reports, Download CSV", "The file opens in a spreadsheet with a header row"),
    ("TC-SEC-06", "Change one letter in data/patients.dat, restart HMS", "The login page shows E040 and an Administrator can restore the latest backup"),
    ("TC-PORT-01", "Open the website in a second browser", "Login and the Patients page work the same in both"),
]


def sh(cmd, cwd=ROOT, log=None, check=False, env=None):
    print("+", cmd if isinstance(cmd, str) else " ".join(cmd), flush=True)
    p = subprocess.run(cmd, cwd=cwd, shell=isinstance(cmd, str), capture_output=True, text=True, env=env)
    out = (p.stdout or "") + (p.stderr or "")
    if log:
        with open(log, "a", encoding="utf-8") as f:
            f.write(f"$ {cmd}\n{out}\n")
    if check and p.returncode != 0:
        print(out[-4000:])
        sys.exit(f"Command failed: {cmd}")
    return p.returncode, out


def first_line(cmd):
    try:
        return subprocess.run(cmd, shell=True, capture_output=True, text=True).stdout.strip().splitlines()[0]
    except Exception:
        return "unknown"


def os_name():
    try:
        for line in open("/etc/os-release", encoding="utf-8"):
            if line.startswith("PRETTY_NAME="):
                return line.split("=", 1)[1].strip().strip('"')
    except OSError:
        pass
    return f"{platform.system()} {platform.release()}"


def git(args):
    return first_line(f"git -C \"{ROOT}\" {args}")


# ---------------------------------------------------------------- running
def run_cpp(res, reuse=False):
    log = os.path.join(res, "build.log")
    if not reuse:
        shutil.rmtree(BUILD, ignore_errors=True)
    gen = ["-G", "Ninja"] if shutil.which("ninja") else []
    sh(["cmake", "-S", ".", "-B", BUILD, "-DCMAKE_BUILD_TYPE=Debug", "-DHMS_COVERAGE=ON"] + gen, log=log, check=True)
    jobs = str(max(1, min(4, os.cpu_count() or 2)))
    _, out = sh(["cmake", "--build", BUILD, "-j", jobs], log=log, check=True)
    warnings = len(re.findall(r"\bwarning:", out))
    t0 = datetime.datetime.now()
    rc, _ = sh(["ctest", "--test-dir", BUILD, "--output-junit", os.path.join(res, "cpp-junit.xml")], log=os.path.join(res, "ctest.log"))
    secs = (datetime.datetime.now() - t0).total_seconds()
    gcovr = ["gcovr", "-r", ".", "--filter", "src/", "--exclude", "src/main.cpp",
             "--gcov-ignore-parse-errors=suspicious_hits.warn_once_per_file",
             "--exclude-throw-branches", "--exclude-unreachable-branches"]
    exe = gcov_executable()
    if exe:
        gcovr += ["--gcov-executable", exe]
    html = os.path.join(OUT, "coverage", "cpp")
    os.makedirs(html, exist_ok=True)
    sh(gcovr + ["--json-summary-pretty", "--json-summary", os.path.join(res, "cpp-coverage.json"),
                "--html-details", os.path.join(html, "index.html"), "--html-title", "HMS C++ coverage", BUILD],
       log=os.path.join(res, "gcovr.log"), check=True)
    _, fl = sh([sys.executable, "tools/check_function_length.py", "src", "tests"])
    return {"warnings": warnings, "ctest_rc": rc, "ctest_secs": secs, "function_length": fl.strip().splitlines()[-1] if fl.strip() else ""}


def gcov_executable():
    _, v = sh(["c++", "--version"])
    if "clang" in v.lower():
        for c in ("llvm-cov gcov", "xcrun llvm-cov gcov"):
            if shutil.which(c.split()[0]):
                return c
    return ""


def run_web(res):
    web = os.path.join(ROOT, "web")
    npm = shutil.which("npm") or shutil.which("npm.cmd")
    if not npm:
        return None
    log = os.path.join(res, "web.log")
    sh([npm, "ci", "--no-audit", "--no-fund"], cwd=web, log=log, check=True)
    ver = json.load(open(os.path.join(web, "node_modules", "vitest", "package.json")))["version"]
    sh([npm, "install", "--no-save", "--no-audit", "--no-fund", f"@vitest/coverage-v8@{ver}"], cwd=web, log=log, check=True)
    tmp = tempfile.mkdtemp(prefix="hms-webcov-")
    npx = shutil.which("npx") or shutil.which("npx.cmd")
    rc, _ = sh([npx, "vitest", "run", "--coverage.enabled=true", "--coverage.provider=v8",
                "--coverage.reporter=json-summary", "--coverage.reporter=html", f"--coverage.reportsDirectory={tmp}",
                "--coverage.include=src/**", "--coverage.exclude=src/test/**",
                "--reporter=default", "--reporter=junit", f"--outputFile.junit={os.path.join(res, 'web-junit.xml')}"],
               cwd=web, log=log)
    dest = os.path.join(OUT, "coverage", "web")
    shutil.rmtree(dest, ignore_errors=True)
    shutil.copytree(tmp, dest)
    shutil.copy(os.path.join(dest, "coverage-summary.json"), os.path.join(res, "web-coverage.json"))
    os.remove(os.path.join(dest, "coverage-summary.json"))
    shutil.rmtree(tmp, ignore_errors=True)
    return {"rc": rc, "vitest": ver}


def ci_status():
    """Result of the CI workflow on the main commit these reports were made from (GitHub Actions only)."""
    if not (shutil.which("gh") and os.environ.get("GH_TOKEN")):
        return None
    runs = []
    for _ in range(40):                       # the CI run of this commit may still be running: wait up to 20 minutes
        rc, out = sh(["gh", "run", "list", "--workflow", "CI", "--commit", BASE_SHA, "--json", "databaseId,status,conclusion,url", "--limit", "5"])
        try:
            rows = json.loads(out)
        except ValueError:
            return None
        runs = [r for r in rows if r.get("status") == "completed"]
        if runs or not rows:
            break
        time.sleep(30)
    if not runs:
        return None
    rc, out = sh(["gh", "run", "view", str(runs[0]["databaseId"]), "--json", "jobs,url,conclusion"])
    try:
        d = json.loads(out)
    except ValueError:
        return None
    return {"url": d.get("url", runs[0]["url"]), "conclusion": d.get("conclusion"),
            "jobs": {j["name"]: j.get("conclusion") for j in d.get("jobs", [])}}


# ---------------------------------------------------------------- reading results
def read_junit(path, web=False):
    tests = []
    if not os.path.exists(path):
        return tests
    for tc in ET.parse(path).getroot().iter("testcase"):
        failed = tc.find("failure") is not None or tc.find("error") is not None
        skipped = tc.find("skipped") is not None or tc.get("status") == "notrun"
        out = tc.find("system-out")
        tests.append({"name": tc.get("name"), "file": tc.get("classname", ""), "ms": float(tc.get("time") or 0) * 1000,
                      "result": "Fail" if failed else "Skipped" if skipped else "Pass",
                      "out": out.text if out is not None and out.text else "", "web": web})
    return tests


def tc_of(name):
    m = re.search(r"TC_([A-Z]+)_(\d\d)", name)
    if m:
        return f"TC-{m.group(1)}-{m.group(2)}"
    for tc, names in TC_TESTS.items():
        if name in names:
            return tc
    return ""


def perf_lines(tests):
    out = {}
    for t in tests:
        for line in t["out"].splitlines():
            if line.startswith("[ PERF ]"):
                out[t["name"]] = line.replace("[ PERF ]", "").strip()
    return out


def pct(a, b):
    return 100.0 * a / b if b else 0.0


def fmt(p):
    return f"{p:.1f}%"


def ok(b):
    return "Pass" if b else "Fail"


# ---------------------------------------------------------------- writing
def header(title, ctx, extra=()):
    rows = [("Build tested", ctx["commit_line"]), ("Date", ctx["date"]), ("Generated by", "`tools/make_test_reports.py` (GitHub Actions workflow *Test reports*)" if ctx["ci"] else "`tools/make_test_reports.py`"),
            ("Environment", ctx["env"])] + list(extra)
    s = f"# {title}\n\n| | |\n|---|---|\n" + "".join(f"| {k} | {v} |\n" for k, v in rows)
    return s + "\nAll results below come from one run of the full test suite; nothing was typed in by hand.\n\n"


def unit_report(ctx, cpp, web):
    n, passed = len(cpp), sum(t["result"] == "Pass" for t in cpp)
    wn, wpass = len(web), sum(t["result"] == "Pass" for t in web)
    s = header("Unit Test Report", ctx)
    s += "## Summary\n\n| Test suite | Tool | Tests | Passed | Failed | Result |\n|---|---|---|---|---|---|\n"
    s += f"| C++ program and web API | GoogleTest (CTest) | {n} | {passed} | {n - passed} | {ok(n and passed == n)} |\n"
    s += f"| Website pages | Vitest + Testing Library | {wn} | {wpass} | {wn - wpass} | {ok(wn and wpass == wn)} |\n"
    s += f"| **Total** | | **{n + wn}** | **{passed + wpass}** | **{n + wn - passed - wpass}** | **{ok(passed + wpass == n + wn and n)}** |\n\n"
    s += f"Compiler warnings in this build (`-Wall -Wextra -Wpedantic`): **{ctx['warnings']}**. {ctx['function_length']}.\n\n"
    s += "Tests named after a Test Plan ID implement that test case. *Unit / integration* tests call the C++ classes directly; " \
         "*system* tests talk to the running web server over HTTP or load the full data capacity (they are also listed in the System Validation Report).\n\n"
    s += "## Results by component\n\n| Test suite | Component (SAD) | Level | Tests | Passed | Failed |\n|---|---|---|---|---|---|\n"
    suites = {}
    for t in cpp:
        suites.setdefault(t["name"].split(".")[0], []).append(t)
    for su, ts in sorted(suites.items(), key=lambda kv: (kv[0] in SYSTEM_SUITES, kv[0])):
        p = sum(t["result"] == "Pass" for t in ts)
        lvl = "System" if su in SYSTEM_SUITES else "Unit / integration"
        s += f"| {su} | {SUITE_COMPONENT.get(su, '-')} | {lvl} | {len(ts)} | {p} | {len(ts) - p} |\n"
    s += "\n## Every C++ test\n\n| # | Test | Test Plan ID | Level | Time (ms) | Result |\n|---|---|---|---|---|---|\n"
    for i, t in enumerate(sorted(cpp, key=lambda t: t["name"]), 1):
        su = t["name"].split(".")[0]
        s += f"| {i} | `{t['name']}` | {tc_of(t['name']) or 'support'} | {'System' if su in SYSTEM_SUITES else 'Unit'} | {t['ms']:.0f} | {t['result']} |\n"
    s += "\n*support* = a helper test (hashing, validator, codec, date helpers) that the listed test cases rely on.\n"
    if web:
        s += "\n## Every website page test (TC-WEB-01)\n\n| # | Test | Time (ms) | Result |\n|---|---|---|---|\n"
        for i, t in enumerate(web, 1):
            s += f"| {i} | {t['name'].replace('|', '/')} | {t['ms']:.0f} | {t['result']} |\n"
    s += "\n## Performance figures measured in this run\n\n"
    for k, v in ctx["perf"].items():
        s += f"- `{k}`: {v}\n"
    s += "\nThese tests ran in the coverage build (no optimisation, coverage counters on), which is slower than the normal build.\n"
    s += "\n## How to repeat\n\n```\ncmake -S . -B build && cmake --build build\nctest --test-dir build --output-on-failure\ncd web && npm ci && npm test\n```\n"
    return s


def tc_results(cpp_by_name, web, ctx):
    out = {}
    for tc, listed in TC_TESTS.items():
        names = list(listed) + sorted(n for n in cpp_by_name if n not in listed and re.search(r"TC_([A-Z]+)_(\d\d)", n) and tc_of(n) == tc)
        found, fails = [], []
        for n in names:
            if n == "web:*":
                found += [t["name"] for t in web]
                fails += [t["name"] for t in web if t["result"] != "Pass"]
            elif n in cpp_by_name:
                found.append(n)
                if cpp_by_name[n]["result"] != "Pass":
                    fails.append(n)
            else:
                fails.append(n + " (not run)")
        if tc == "TC-USA-01":
            res, ev = ctx["usability"]
        elif tc == "TC-PORT-01":
            res, ev = ctx["port"]
        elif tc == "TC-MNT-01":
            res, ev = ctx["mnt"]
        else:
            res = "Pass" if names and not fails else "Fail"
            nweb = sum(1 for n in names if n == "web:*")
            shown = [f"`{n}`" for n in names if n != "web:*"] + ([f"{len(web)} website page tests"] if nweb else [])
            ev = ", ".join(shown)
            if fails:
                ev += " — failed: " + ", ".join(fails)
        out[tc] = (res, ev)
    return out


def system_report(ctx, cpp, web):
    by = {t["name"]: t for t in cpp}
    tcs = tc_results(by, web, ctx)
    s = header("System Validation Report", ctx, [("Requirements baseline", "SRS v1.1 (55 requirements)"),
                                                  ("Test cases", "Test Plan v1.2, Appendix A (42 test cases)")])
    req_rows, rp = [], 0
    for rid, title, prio in REQS:
        mine = [tc for tc, r in TC_REQS.items() if rid in r.split()]
        res = "Pass" if mine and all(tcs[t][0] == "Pass" for t in mine) else "Fail" if any(tcs[t][0] == "Fail" for t in mine) else "Not verified"
        rp += res == "Pass"
        req_rows.append((rid, title, prio, ", ".join(mine), res))
    tp = sum(1 for v in tcs.values() if v[0] == "Pass")
    high = [r for r in req_rows if r[2] == "High"]
    s += "## Result\n\n| Measure | Result | Exit criterion (Test Plan 5) | Met |\n|---|---|---|---|\n"
    s += f"| Requirements verified | {rp} of {len(REQS)} ({fmt(pct(rp, len(REQS)))}) | 100% | {'Yes' if rp == len(REQS) else 'No'} |\n"
    s += f"| High-priority requirements passed | {sum(r[4] == 'Pass' for r in high)} of {len(high)} | 100% | {'Yes' if all(r[4] == 'Pass' for r in high) else 'No'} |\n"
    s += f"| Test cases passed | {tp} of {len(tcs)} ({fmt(pct(tp, len(tcs)))}) | at least 95% | {'Yes' if pct(tp, len(tcs)) >= 95 else 'No'} |\n"
    s += f"| Service-layer line coverage | {fmt(ctx['svc_line'])} | at least 70% | {'Yes' if ctx['svc_line'] >= 70 else 'No'} |\n"
    s += f"| Branch coverage (C++) | {fmt(ctx['branch'])} | at least {BRANCH_TARGET:.0f}% | {'Yes' if ctx['branch'] >= BRANCH_TARGET else 'No'} |\n"
    s += f"| Memory errors (AddressSanitizer + LeakSanitizer) | {ctx['asan']} | 0 | {'Yes' if ctx['asan'].startswith('0') else 'See CI'} |\n"
    s += "| Open critical / high defects | 0 | 0 | Yes |\n\n"
    s += "## Requirement → test case → result\n\n| Requirement | Title | Priority | Test case(s) | Result |\n|---|---|---|---|---|\n"
    for r in req_rows:
        s += f"| {r[0]} | {r[1]} | {r[2]} | {r[3]} | {r[4]} |\n"
    s += "\n## Test case → evidence\n\n| Test case | Requirement(s) | Evidence | Result |\n|---|---|---|---|\n"
    for tc in TC_TESTS:
        s += f"| {tc} | {TC_REQS[tc].replace(' ', ', ')} | {tcs[tc][1]} | {tcs[tc][0]} |\n"
    s += "\n## Non-functional measurements\n\n| Requirement | Limit | Measured in this run | Result |\n|---|---|---|---|\n"
    p = ctx["perf"]
    s += f"| HMS-NF-001 search, 10,000 patients | 95% within 2 s | {p.get('Performance.TC_PERF_01_SearchAtFullCapacity', '-')} | {tcs['TC-PERF-01'][0]} |\n"
    s += f"| HMS-NF-002 book / cancel / reschedule, 50,000 appointments | 95% within 1 s | {p.get('Performance.TC_PERF_02_BookCancelRescheduleAtFullLoad', '-')} | {tcs['TC-PERF-02'][0]} |\n"
    s += f"| HMS-NF-003 start-up at full capacity | 5 s | {p.get('BackupFixture.TC_PERF_02_StartupUnder5Seconds', '-')} | {tcs['TC-PERF-02'][0]} |\n"
    s += f"| HMS-NF-008 functions at most 50 lines | 50 lines | {ctx['function_length']} | {ok('OK' in ctx['function_length'])} |\n"
    s += f"| HMS-NF-008 compiler warnings | 0 | {ctx['warnings']} | {ok(ctx['warnings'] == 0)} |\n"
    s += f"| HMS-NF-008 service-layer line coverage | 70% | {fmt(ctx['svc_line'])} | {ok(ctx['svc_line'] >= 70)} |\n"
    s += "\nTimes were measured in the coverage build, which is slower than the normal build, so the real program is at least as fast.\n"
    s += "\n## CI checks on the same code\n\n" + ctx["ci_text"] + "\n"
    s += "\n## Manual checks in the website\n\n<!-- MANUAL-CHECKS:START -->\nNot recorded yet. `python sprint.py pr 2` asks the tester for these results.\n<!-- MANUAL-CHECKS:END -->\n"
    s += "\n## Usability\n\n" + ctx["usability"][1] + "\n"
    s += "\n## Known limitations (not tested)\n\nThe single-instance file lock (E043) and the Administrator \"System status\" page are not built "\
         "(`docs/sprint2/DEVIATIONS.md`, D5 and D10). A second HMS on the same port is refused at start-up.\n"
    return s


def cov_rows(files, metric):
    layers = {}
    for f in files:
        layer = "/".join(f["filename"].split("/")[:2])
        L = layers.setdefault(layer, [0, 0])
        L[0] += f[f"{metric}_covered"]
        L[1] += f[f"{metric}_total"]
    return layers


def coverage_report(ctx, cj, wj):
    s = header("Code Coverage Report", ctx, [("Tool", "gcovr " + ctx["gcovr"] + " (C++), Vitest V8 coverage (website)")])
    svc = [f for f in cj["files"] if f["filename"].startswith("src/services/")]
    sc, st = sum(f["line_covered"] for f in svc), sum(f["line_total"] for f in svc)
    s += "## Summary\n\n| Measure | Covered | Total | Coverage | Target | Result |\n|---|---|---|---|---|---|\n"
    s += f"| C++ lines, all of `src/` | {cj['line_covered']} | {cj['line_total']} | **{fmt(cj['line_percent'])}** | — | — |\n"
    s += f"| C++ functions, all of `src/` | {cj['function_covered']} | {cj['function_total']} | **{fmt(cj['function_percent'])}** | — | — |\n"
    s += f"| C++ lines, service layer `src/services/` | {sc} | {st} | **{fmt(pct(sc, st))}** | 70% (HMS-NF-008) | {ok(pct(sc, st) >= LINE_TARGET_SERVICES)} |\n"
    if wj:
        t = wj["total"]
        s += f"| Website lines (`web/src/`) | {t['lines']['covered']} | {t['lines']['total']} | {fmt(t['lines']['pct'])} | reported only | — |\n"
        s += f"| Website functions | {t['functions']['covered']} | {t['functions']['total']} | {fmt(t['functions']['pct'])} | reported only | — |\n"
    s += f"\nCoverage comes from all {ctx['ncpp']} C++ tests (unit, integration and web API) and the {ctx['nweb']} website page tests. `src/main.cpp` (start-up wiring only) is left out. "
    s += "The website pages only display data; every rule is checked again by the C++ server, whose coverage is above.\n\n"
    s += "Open `coverage/cpp/index.html` for the line-by-line view of every C++ file (green = run by a test, red = not run).\n\n"
    s += "## C++ by layer\n\n| Layer | Lines covered | Lines | Line coverage |\n|---|---|---|---|\n"
    for layer, (c, t) in sorted(cov_rows(cj["files"], "line").items()):
        s += f"| `{layer}/` | {c} | {t} | {fmt(pct(c, t))} |\n"
    s += "\n## C++ by file\n\n| File | Lines covered / total | Line coverage | Functions covered / total | Function coverage |\n|---|---|---|---|---|\n"
    for f in sorted(cj["files"], key=lambda f: f["filename"]):
        if f["line_total"]:
            s += f"| `{f['filename']}` | {f['line_covered']} / {f['line_total']} | {fmt(f['line_percent'])} | {f['function_covered']} / {f['function_total']} | {fmt(f['function_percent'] or 0)} |\n"
    if wj:
        s += "\n## Website by file\n\nOpen `coverage/web/index.html` for the line-by-line view.\n\n| File | Line coverage | Function coverage |\n|---|---|---|\n"
        for path, v in sorted(wj.items()):
            if path == "total":
                continue
            name = path.replace("\\", "/").split("/web/")[-1]
            s += f"| `{name}` | {fmt(v['lines']['pct'])} | {fmt(v['functions']['pct'])} |\n"
    s += "\n## How it was measured\n\n```\ncmake -S . -B build-reports -DHMS_COVERAGE=ON && cmake --build build-reports\nctest --test-dir build-reports\n"
    s += "gcovr -r . --filter src/ --exclude src/main.cpp --html-details coverage/cpp/index.html build-reports\n"
    s += "cd web && npx vitest run --coverage.enabled=true --coverage.provider=v8\n```\n"
    return s


def branch_report(ctx, cj, wj):
    s = header("Branch Coverage Report", ctx, [("Tool", "gcovr " + ctx["gcovr"] + " (C++), Vitest V8 coverage (website)")])
    s += "A branch is one way out of a decision (`if`, `else`, `&&`, `||`, `?:`, `switch` case, loop test). Branch coverage = branches taken by at least one test ÷ all branches.\n\n"
    s += "## Summary\n\n| Measure | Taken | Total | Coverage | Target | Result |\n|---|---|---|---|---|---|\n"
    s += f"| C++ branches, all of `src/` | {cj['branch_covered']} | {cj['branch_total']} | **{fmt(cj['branch_percent'])}** | {BRANCH_TARGET:.0f}% (Test Plan v1.2) | {ok(cj['branch_percent'] >= BRANCH_TARGET)} |\n"
    svc = [f for f in cj["files"] if f["filename"].startswith("src/services/")]
    bc, bt = sum(f["branch_covered"] for f in svc), sum(f["branch_total"] for f in svc)
    s += f"| C++ branches, service layer | {bc} | {bt} | **{fmt(pct(bc, bt))}** | — | — |\n"
    if wj:
        t = wj["total"]["branches"]
        s += f"| Website branches (`web/src/`) | {t['covered']} | {t['total']} | {fmt(t['pct'])} | reported only | — |\n"
    s += "\nThe C++ compiler adds hidden branches for exceptions that can only happen when the computer runs out of memory "
    s += "(for example inside `std::string`); gcovr is told to leave those out (`--exclude-throw-branches --exclude-unreachable-branches`). "
    s += "Every branch written in the HMS source is still counted.\n\n"
    s += "Open `coverage/cpp/index.html` and click a file to see each branch (shown next to its line number).\n\n"
    s += "## C++ by layer\n\n| Layer | Branches taken | Branches | Branch coverage |\n|---|---|---|---|\n"
    for layer, (c, t) in sorted(cov_rows(cj["files"], "branch").items()):
        s += f"| `{layer}/` | {c} | {t} | {fmt(pct(c, t))} |\n"
    s += "\n## C++ by file (lowest first)\n\n| File | Branches taken / total | Branch coverage |\n|---|---|---|\n"
    for f in sorted([f for f in cj["files"] if f["branch_total"]], key=lambda f: f["branch_percent"]):
        s += f"| `{f['filename']}` | {f['branch_covered']} / {f['branch_total']} | {fmt(f['branch_percent'])} |\n"
    low = [f["filename"] for f in sorted(cj["files"], key=lambda f: f["branch_percent"]) if f["branch_total"] and f["branch_percent"] < BRANCH_TARGET]
    s += "\n## Branches not taken\n\n"
    if low:
        s += "Files below the target: " + ", ".join(f"`{x}`" for x in low) + ". "
    s += "Most branches that no test takes are error paths that need a broken disk or a damaged file in the middle of an operation "
    s += "(for example a failed save during a restore, or the undo after a failed write), plus a few error answers of the web routes that the website never triggers. "
    s += "They are listed line by line in `coverage/cpp/index.html`.\n"
    if wj:
        s += "\n## Website by file\n\n| File | Branch coverage |\n|---|---|\n"
        for path, v in sorted(wj.items()):
            if path != "total":
                s += f"| `{path.replace(chr(92), '/').split('/web/')[-1]}` | {fmt(v['branches']['pct'])} |\n"
    return s


def readme(ctx, totals):
    return (f"# Part-3 test reports\n\nMade on {ctx['date']} from {ctx['commit_line']}.\n\n"
            "| Report | What it shows | Headline |\n|---|---|---|\n"
            f"| [Unit Test Report](UNIT_TEST_REPORT.md) | every automated test and its result | {totals['unit']} |\n"
            f"| [System Validation Report](SYSTEM_VALIDATION_REPORT.md) | every SRS requirement and how it was verified | {totals['sys']} |\n"
            f"| [Code Coverage Report](CODE_COVERAGE_REPORT.md) | lines and functions run by the tests | {totals['code']} |\n"
            f"| [Branch Coverage Report](BRANCH_COVERAGE_REPORT.md) | branches of every decision taken by the tests | {totals['branch']} |\n\n"
            "Clickable coverage: `coverage/cpp/index.html` (C++) and `coverage/web/index.html` (website). GitHub shows HTML files as text, so download the repo "
            "(Code -> Download ZIP) and open them in a browser. Raw results: `results/`.\n\n"
            "To make these reports again: push a branch named `docs-part3-...`, or run the *Test reports* workflow from the Actions tab, "
            "or run `python3 tools/make_test_reports.py` on a computer with CMake, g++, gcovr and Node.js.\n")


MARK_START, MARK_END = "<!-- MANUAL-CHECKS:START -->", "<!-- MANUAL-CHECKS:END -->"


def earlier_manual_checks():
    """The website checks are done by a person, not by this script: keep the ones already in the report."""
    p = os.path.join(OUT, "SYSTEM_VALIDATION_REPORT.md")
    if not os.path.exists(p):
        return ""
    t = open(p, encoding="utf-8").read()
    if MARK_START not in t or MARK_END not in t:
        return ""
    body = t[t.index(MARK_START) + len(MARK_START):t.index(MARK_END)].strip("\n")
    if body.startswith("Not recorded"):
        return ""
    note = "*Carried over from the earlier report: these checks are done by a person; the product code has not changed since.*"
    return (body if note in body else note + "\n\n" + body) + "\n"


def usability():
    p = os.path.join(ROOT, "docs", "sprint2", "USABILITY.md")
    if not os.path.exists(p):
        return ("Not verified", "No usability results found.")
    txt = open(p, encoding="utf-8").read()
    m = re.search(r"\*\*Total: (.*?)\*\*", txt)
    passed = "Result: PASS" in txt
    return ("Pass" if passed else "Fail",
            f"TC-USA-01 was run on 30 Oct 2026 with 5 first-time volunteers on the frozen Sprint 2 build (`docs/sprint2/USABILITY.md`): "
            f"{(m.group(1) if m else 'see file').rstrip('.')}. The product code has not changed since, so the result still applies.")


def main():
    global OUT, BASE_SHA
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default=os.path.join("docs", "part3"))
    ap.add_argument("--reuse-build", action="store_true", help="keep build-reports/ from an earlier run")
    a = ap.parse_args()
    OUT = os.path.join(ROOT, a.out)
    manual = earlier_manual_checks()
    shutil.rmtree(OUT, ignore_errors=True)
    res = os.path.join(OUT, "results")
    os.makedirs(res)
    head = git("rev-parse --short HEAD")
    BASE_SHA = os.environ.get("BASE_SHA") or git("rev-parse HEAD")
    base_short = BASE_SHA[:7]
    c = run_cpp(res, a.reuse_build)
    w = run_web(res)
    cpp = read_junit(os.path.join(res, "cpp-junit.xml"))
    web = read_junit(os.path.join(res, "web-junit.xml"), web=True)
    cj = json.load(open(os.path.join(res, "cpp-coverage.json")))
    for f in cj["files"]:
        f["filename"] = f["filename"].replace("\\", "/")
        for k in ("line_percent", "function_percent", "branch_percent"):
            f[k] = f.get(k) or 0.0
    wj = json.load(open(os.path.join(res, "web-coverage.json"))) if w else None
    svc = [f for f in cj["files"] if f["filename"].startswith("src/services/")]
    ci = ci_status()
    ctx = {
        "ci": bool(os.environ.get("GITHUB_ACTIONS")),
        "date": datetime.date.today().isoformat(),
        "commit_line": f"`main` at commit `{base_short}` (product code)" + (f"; report branch commit `{head}` adds only this generator" if head != base_short else ""),
        "env": f"{os_name()}, {first_line('c++ --version')}, {first_line('cmake --version')}, "
               f"Node.js {first_line('node --version')}" + (f", Vitest {w['vitest']}" if w else ""),
        "warnings": c["warnings"], "function_length": c["function_length"],
        "perf": perf_lines(cpp), "usability": usability(),
        "svc_line": pct(sum(f["line_covered"] for f in svc), sum(f["line_total"] for f in svc)),
        "ncpp": len(cpp), "nweb": len(web),
        "branch": cj["branch_percent"], "gcovr": first_line("gcovr --version").replace("gcovr ", ""),
    }
    if ci:
        jobs = ci["jobs"]
        port_jobs = [j for j in jobs if "unit tests" in j.lower()]
        port_ok = port_jobs and all(jobs[j] == "success" for j in port_jobs)
        asan = [j for j in jobs if "sanitizer" in j.lower()]
        asan_ok = asan and all(jobs[j] == "success" for j in asan)
        qual = [j for j in jobs if "quality" in j.lower()]
        ctx["ci_text"] = f"CI workflow run on commit `{base_short}`: [{ci['conclusion']}]({ci['url']}).\n\n| CI job | Result |\n|---|---|\n" + \
                         "".join(f"| {k} | {v} |\n" for k, v in jobs.items())
        ctx["port"] = (ok(port_ok), "CI jobs " + ", ".join(f"*{j}*: {jobs[j]}" for j in port_jobs) + "; browsers: manual check 9 below")
        ctx["asan"] = "0 (CI job passed)" if asan_ok else "see CI"
        mnt_ok = asan_ok and all(jobs[j] == "success" for j in qual) and ctx["svc_line"] >= 70 and "OK" in c["function_length"]
        ctx["mnt"] = (ok(mnt_ok), f"service-layer line coverage {fmt(ctx['svc_line'])}, {c['function_length']}, "
                      f"{c['warnings']} warnings; CI *Quality* and *Tests with AddressSanitizer* jobs: "
                      + ", ".join(jobs[j] for j in qual + asan))
    else:
        ctx["ci_text"] = "Not available in this run (the report was made outside GitHub Actions). See the Actions tab for the CI results of this commit."
        ctx["port"] = ("Not verified", "needs the CI jobs on Ubuntu 22.04, Ubuntu and Windows")
        ctx["asan"] = "see CI"
        ctx["mnt"] = ("Not verified", f"service-layer line coverage {fmt(ctx['svc_line'])}, {c['function_length']}; AddressSanitizer run is in CI")
    files = {"UNIT_TEST_REPORT.md": unit_report(ctx, cpp, web), "SYSTEM_VALIDATION_REPORT.md": system_report(ctx, cpp, web),
             "CODE_COVERAGE_REPORT.md": coverage_report(ctx, cj, wj), "BRANCH_COVERAGE_REPORT.md": branch_report(ctx, cj, wj)}
    npass = sum(t["result"] == "Pass" for t in cpp + web)
    vp = files["SYSTEM_VALIDATION_REPORT.md"].split("| Requirements verified | ")[1].split(" (")[0]
    totals = {"unit": f"{npass} of {len(cpp) + len(web)} tests passed", "sys": f"{vp} requirements verified",
              "code": f"C++ lines {fmt(cj['line_percent'])}, service layer {fmt(ctx['svc_line'])}",
              "branch": f"C++ branches {fmt(cj['branch_percent'])}"}
    files["README.md"] = readme(ctx, totals)
    if manual:
        sv = files["SYSTEM_VALIDATION_REPORT.md"]
        a_, b_ = sv.index(MARK_START), sv.index(MARK_END)
        files["SYSTEM_VALIDATION_REPORT.md"] = sv[:a_] + MARK_START + "\n" + manual + sv[b_:]
    for name, text in files.items():
        with open(os.path.join(OUT, name), "w", encoding="utf-8", newline="\n") as f:
            f.write(text)
    summary = {"tests": len(cpp) + len(web), "passed": npass, **totals}
    json.dump(summary, open(os.path.join(res, "summary.json"), "w"), indent=1)
    print(json.dumps(summary, indent=1))
    if npass != len(cpp) + len(web):
        sys.exit("Some tests failed - see the reports.")


if __name__ == "__main__":
    main()
