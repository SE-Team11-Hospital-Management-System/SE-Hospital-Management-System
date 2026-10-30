# HMS User Guide

## Starting HMS
Start the HMS program (developers: `python sprint.py run`; otherwise run `build/hms` from the project folder).
A window says **"HMS is running. Open http://localhost:8080"**. Open that address in Chrome, Edge or Firefox.
Keep the HMS window open while you work; closing it stops HMS.

## First run
If no accounts exist, the website asks you to create the first **Administrator** (username 4–20 letters/digits;
password 8–32 characters with at least one letter and one digit). Then log in with it.

## Logging in and out
Type your username and password; the password field shows only dots. After 3 wrong passwords the account is locked for 15 minutes.
If you do nothing for **10 minutes**, you are logged out automatically and must log in again.
The **Log out** button is at the top right. The moon / sun button switches between light and dark mode.

## Pages for each role
| Page | Administrator | Receptionist | Doctor |
|---|---|---|---|
| Dashboard | ✔ | ✔ | ✔ (today's appointments) |
| Patients — search | ✔ | ✔ | ✔ |
| Patients — register / edit | ✔ | ✔ | — |
| Patients — deactivate | ✔ | — | — |
| Doctors — list | ✔ | ✔ | ✔ |
| Doctors — add / edit / working hours | ✔ | — | — |
| Appointments — book, cancel, reschedule, no-show | ✔ | ✔ | — |
| Doctor schedules | ✔ (any doctor) | ✔ (any doctor) | ✔ (own only: "My schedule") |
| Consultations — record, history, addendum | — | — | ✔ |
| Reports — daily report + CSV | ✔ | ✔ | — |
| Reports — doctor workload | ✔ | — | — |
| User accounts, Audit log, Backup and restore | ✔ | — | — |
| Change my password | ✔ | ✔ | ✔ |

The sidebar shows only the pages your role may use.

## First set-up (Administrator)
1. **User accounts → Create user** for each receptionist and doctor. Note the user ID shown (e.g. `U00003`).
2. **Doctors → Add doctor**: pick the doctor's login account from the list. The doctor gets an ID like `D001`.
3. **Doctors → Hours**: tick the working days and choose start and end times (30-minute steps, 08:00–20:00).

## Patients (Receptionist)
- **Register patient**: name, date of birth, gender, 10-digit phone, address, blood group. The new ID (e.g. `P000001`) is shown.
  If the same name, date of birth and phone already exist, HMS shows the existing ID and asks before creating a second record.
- **Search** by name (part of it), patient ID or phone; 20 results per page.
- **Edit** changes name, phone, address and blood group; ID, date of birth and gender cannot change.
- **Deactivate** (Administrator) keeps the record but marks it Inactive; it cannot get new appointments.

## Appointments (Receptionist)
- **Book**: find the patient by ID, pick the doctor, date and time. If the slot is taken, in the past or outside the doctor's hours,
  HMS says why and shows the next 3 free slots as buttons; one click books one.
- **Cancel / reschedule / no-show**: enter the patient ID, then use the buttons next to a Scheduled appointment.
  Cancel only before the start time; no-show only after it. A cancelled slot is free again at once.
- Status is always Scheduled, Completed, Cancelled or No-show. Completed is set when the doctor saves the consultation.

## Consultations (Doctor)
1. **Record**: today's appointments are listed; click **Record** on one. Enter the diagnosis (up to 500 characters), up to 10
   medicines (name, dosage, days) and optional notes (up to 1000 characters), then **Continue**.
2. HMS shows the **patient ID, name and age**. **Yes — save** saves it; **No — discard** saves nothing.
3. **Patient history** shows all consultations of a patient, newest first.
4. Saved consultations can never be changed or deleted. **Add addendum** adds a dated note below the original.

Receptionists and Administrators never see diagnoses, prescriptions or notes.

## Reports
- **Daily appointments** (Administrator, Receptionist): totals per status and the list per doctor. **Download CSV** saves
  `daily_<date>.csv`, which opens in Excel, LibreOffice or Google Sheets (a copy is kept in the `reports` folder).
- **Doctor workload** (Administrator): completed appointments per doctor between two dates.

## Audit log (Administrator)
Who did what and when, filtered by dates and user. It can only be read, never edited, and never contains passwords or medical text.

## Backups (Administrator)
**Create backup now** copies all data into `backup/YYYYMMDD_HHMMSS`. **Restore** replaces the current data after you confirm.
The audit log is never rolled back.

If a data file was damaged or changed outside HMS, the login page shows **E040** and offers to restore the latest backup;
an Administrator logs in on that page to confirm.

## Error codes
| Code | Meaning |
|---|---|
| E001 | Invalid username or password (or wrong current password) |
| E002 | Account locked — wait the minutes shown |
| E003 | Session expired — log in again |
| E004 | Your role is not allowed to do this |
| E010 | A field is empty or in the wrong format (the message names the field and the rule) |
| E011 | Duplicate (username or patient) |
| E020 / E021 / E022 / E023 / E024 | Patient / doctor / appointment / user / consultation not found |
| E025 | Patient record is inactive |
| E030 | Slot already taken |
| E031 | Slot in the past or outside the doctor's hours |
| E032 | Not allowed in the current status |
| E040 | Data file damaged — restore from backup |
| E041 | Could not save; nothing was changed |
| E042 | Backup or restore failed |
| E099 | Unexpected error, or the HMS program is not running |
