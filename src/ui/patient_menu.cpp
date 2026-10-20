// Patient screens: register, search, update (HMS-F-010..013, F-015).
#include "security/access_control.h"
#include "ui/console_ui.h"
#include "ui/menus.h"
#include <iostream>

namespace hms {

namespace {
void show(const Patient& p) {
    std::cout << "  " << p.patientId << "  " << p.fullName << "  DOB " << p.dob << "  " << p.gender << "  " << p.phone
              << "  " << p.bloodGroup << (p.active ? "" : "  [Inactive]") << "\n      " << p.address << '\n';
}

PatientInput askAll() {
    PatientInput in;
    in.fullName = ConsoleUi::readLine("Full name: ");
    in.dob = ConsoleUi::readLine("Date of birth (YYYY-MM-DD): ");
    in.gender = ConsoleUi::readLine("Gender (M/F/O): ");
    in.phone = ConsoleUi::readLine("Phone (10 digits): ");
    in.address = ConsoleUi::readLine("Address: ");
    in.bloodGroup = ConsoleUi::readLine("Blood group (A+, A-, B+, B-, AB+, AB-, O+, O-): ");
    return in;
}

void registerFlow(AppContext& ctx, const Session& s) {
    ConsoleUi::header("Register patient");
    const PatientInput in = askAll();
    auto r = ctx.patients.registerPatient(s, in);
    if (!r.ok() && r.error().code == ErrorCode::E011) {                 // HMS-F-015: show existing ID, ask to confirm
        ConsoleUi::printError(r.error());
        if (!ConsoleUi::confirm("Register anyway as a new patient?")) { std::cout << "Nothing was saved.\n"; return; }
        r = ctx.patients.registerPatient(s, in, true);
    }
    if (r.ok()) {
        ConsoleUi::printOk("Patient registered with ID " + r.value() + ".");
        if (auto p = ctx.patients.get(r.value())) show(*p);
    } else {
        ConsoleUi::printError(r.error());
    }
}

void searchFlow(AppContext& ctx, const Session& s) {
    ConsoleUi::header("Search patient");
    const int k = ConsoleUi::readChoice("Search by: 1 = Patient ID, 2 = Name (part), 3 = Phone: ");
    if (k < 1 || k > 3) { ConsoleUi::printError("E010", "Search by: choose 1, 2 or 3."); return; }
    const std::string term = ConsoleUi::readLine("Search for: ");
    for (int page = 1;; ++page) {
        auto r = ctx.patients.search(s, static_cast<SearchKey>(k - 1), term, page);
        if (!r.ok()) { ConsoleUi::printError(r.error()); return; }
        if (r.value().items.empty()) { std::cout << (page == 1 ? "No patients found.\n" : "No more results.\n"); return; }
        std::cout << "Page " << page << ":\n";
        for (const auto& p : r.value().items) show(p);
        if (!r.value().hasNext || !ConsoleUi::confirm("Show next page?")) return;      // 20 per page (HMS-F-012)
    }
}

void updateFlow(AppContext& ctx, const Session& s) {
    ConsoleUi::header("Update patient");
    const std::string id = ConsoleUi::readLine("Patient ID (e.g. P000001): ");
    auto cur = ctx.patients.get(id);
    if (!cur) { ConsoleUi::printError("E020", "Patient not found."); return; }
    show(*cur);
    std::cout << "Press Enter to keep a value. Patient ID and date of birth cannot be changed.\n";
    PatientInput c;
    c.fullName = ConsoleUi::readLine("New full name: ");
    c.phone = ConsoleUi::readLine("New phone: ");
    c.address = ConsoleUi::readLine("New address: ");
    c.bloodGroup = ConsoleUi::readLine("New blood group: ");
    auto r = ctx.patients.updatePatient(s, id, c);
    if (r.ok()) ConsoleUi::printOk("Patient " + id + " updated.");
    else ConsoleUi::printError(r.error());
}
}  // namespace

void patientMenu(AppContext& ctx, const Session& s) {
    const bool canEdit = AccessControl::isAllowed(s.role, Operation::RegisterPatient);
    while (!ConsoleUi::inputClosed()) {
        ConsoleUi::header("Patients");
        std::cout << "  1. Search patient\n";
        if (canEdit) std::cout << "  2. Register patient\n  3. Update patient\n";
        std::cout << "  0. Back\n";
        const int c = ConsoleUi::readChoice("Choice: ");
        if (c == 0) return;
        if (c == 1) searchFlow(ctx, s);
        else if (c == 2 && canEdit) registerFlow(ctx, s);
        else if (c == 3 && canEdit) updateFlow(ctx, s);
        else ConsoleUi::printError("E010", "Choice: enter a number from the menu.");
    }
}
}  // namespace hms
