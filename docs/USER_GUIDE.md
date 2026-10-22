# HMS User Guide (Sprint 1 build)

## Starting the program
Open a terminal in the project folder and run `./build/hms` (Windows: `build\hms.exe`).
Data is stored in the `data` folder next to where you start it. Backups go to the `backup` folder.

## First run
If no accounts exist, HMS asks you to create the first **Administrator** (username 4–20 letters/digits; password 8–32 characters with at least one letter and one digit). Then log in with it.

## Logging in
Type your username and password. The password is not shown while you type.
After 3 wrong passwords the account is locked for 15 minutes. Type `quit` at the username prompt to exit.

## What each role can do (Sprint 1)
| Menu | Administrator | Receptionist | Doctor |
|---|---|---|---|
| Patients → Search | ✔ | ✔ | ✔ |
| Patients → Register / Update | ✔ | ✔ | — |
| Doctors → List | ✔ | ✔ | ✔ |
| Doctors → Add / Update / Set availability | ✔ | — | — |
| Appointments → Book / List | ✔ | ✔ | — |
| User accounts | ✔ | — | — |
| Backup and restore | ✔ | — | — |

## Typical first set-up (Administrator)
1. **User accounts → Create user** for each receptionist and doctor. Note the user ID shown (e.g. `U00003`).
2. **Doctors → Add doctor**, entering the doctor's user ID as the linked account. The doctor gets an ID like `D001`.
3. **Doctors → Set availability**: one line per working block, e.g. `1 09:00 13:00` = Monday 9 am to 1 pm. Empty line to finish.

## Registering a patient (Receptionist)
**Patients → Register patient**. Enter name, date of birth (`YYYY-MM-DD`), gender (`M`/`F`/`O`), 10-digit phone, address and blood group. The patient gets an ID like `P000001`.
If the same name, date of birth and phone already exist, HMS shows the existing ID and asks before creating a second record.

## Booking an appointment
**Appointments → Book appointment**: patient ID, doctor ID, date and time (on the hour or half hour).
If the slot is taken, in the past, or outside the doctor's hours, HMS says why and offers the next 3 free slots; type 1, 2 or 3 to book one.

## Backups (Administrator)
**Backup and restore → Create backup now** copies all data into `backup/YYYYMMDD_HHMMSS`.
**Restore a backup** replaces the current data after you confirm with `Y`. The audit log is never rolled back.

## Error codes you may see
| Code | Meaning |
|---|---|
| E001 | Invalid username or password |
| E002 | Account locked — wait the minutes shown |
| E004 | Your role is not allowed to do this |
| E010 | A field is empty or in the wrong format (the message names the field) |
| E011 | Duplicate (username or patient) |
| E020 / E021 / E023 | Patient / doctor / user not found |
| E025 | Patient record is inactive |
| E030 | Slot already taken |
| E031 | Slot in the past or outside the doctor's hours |
| E041 | Could not save; nothing was changed |
| E042 | Backup or restore failed |
