// Doctor screens: add / update profile, set weekly availability, list doctors (HMS-F-020/021/022).
#include "security/access_control.h"
#include "ui/console_ui.h"
#include "ui/menus.h"
#include <iostream>

namespace hms {

namespace {
const char* kDays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

void show(const Doctor& d) {
    std::cout << "  " << d.doctorId << "  " << d.fullName << "  " << d.specialization << "  " << d.department << '\n';
    if (d.windows.empty()) std::cout << "      availability: not set\n";
    for (const auto& w : d.windows) std::cout << "      " << kDays[w.weekday] << " " << w.start << "-" << w.end << '\n';
}

void addFlow(AppContext& ctx, const Session& s) {
    ConsoleUi::header("Add doctor");
    DoctorInput in;
    in.fullName = ConsoleUi::readLine("Full name: ");
    in.specialization = ConsoleUi::readLine("Specialization (e.g. Cardiology): ");
    in.department = ConsoleUi::readLine("Department: ");
    in.userId = ConsoleUi::readLine("Linked Doctor user ID (e.g. U00003): ");
    auto r = ctx.doctors.addDoctor(s, in);
    if (r.ok()) ConsoleUi::printOk("Doctor added with ID " + r.value() + ". Set the availability next.");
    else ConsoleUi::printError(r.error());
}

void updateFlow(AppContext& ctx, const Session& s) {
    ConsoleUi::header("Update doctor");
    const std::string id = ConsoleUi::readLine("Doctor ID (e.g. D001): ");
    auto d = ctx.doctors.get(id);
    if (!d) { ConsoleUi::printError("E021", "Doctor not found."); return; }
    show(*d);
    std::cout << "Press Enter to keep a value.\n";
    DoctorInput c;
    c.fullName = ConsoleUi::readLine("New full name: ");
    c.specialization = ConsoleUi::readLine("New specialization: ");
    c.department = ConsoleUi::readLine("New department: ");
    auto r = ctx.doctors.updateDoctor(s, id, c);
    if (r.ok()) ConsoleUi::printOk("Doctor " + id + " updated.");
    else ConsoleUi::printError(r.error());
}

void availabilityFlow(AppContext& ctx, const Session& s) {
    ConsoleUi::header("Set availability");
    const std::string id = ConsoleUi::readLine("Doctor ID (e.g. D001): ");
    if (!ctx.doctors.exists(id)) { ConsoleUi::printError("E021", "Doctor not found."); return; }
    std::cout << "Enter one working block per line as  <day> <start> <end>, e.g.  1 09:00 13:00\n"
                 "Days: 0 Sun, 1 Mon, 2 Tue, 3 Wed, 4 Thu, 5 Fri, 6 Sat. Times 08:00-20:00 in 30-minute steps.\n"
                 "Press Enter on an empty line to finish. This replaces the old availability.\n";
    std::vector<Window> ws;
    while (!ConsoleUi::inputClosed()) {
        const std::string line = ConsoleUi::readLine("Block: ");
        if (line.empty()) break;
        if (line.size() != 13 || line[1] != ' ' || line[7] != ' ' || line[0] < '0' || line[0] > '6') {
            ConsoleUi::printError("E010", "Block: format is  <day 0-6> <HH:MM> <HH:MM>, e.g.  1 09:00 13:00");
            continue;
        }
        ws.push_back(Window{line[0] - '0', line.substr(2, 5), line.substr(8, 5)});
    }
    auto r = ctx.doctors.setAvailability(s, id, ws);
    if (r.ok()) ConsoleUi::printOk("Availability saved for " + id + ".");
    else ConsoleUi::printError(r.error());
}

void listFlow(AppContext& ctx, const Session& s) {
    ConsoleUi::header("Doctors");
    const std::string spec = ConsoleUi::readLine("Filter by specialization (Enter for all): ");
    auto r = ctx.doctors.list(s, spec.empty() ? std::nullopt : std::optional<std::string>(spec));
    if (!r.ok()) { ConsoleUi::printError(r.error()); return; }
    if (r.value().empty()) std::cout << "No doctors found.\n";
    for (const auto& d : r.value()) show(d);
}
}  // namespace

void doctorMenu(AppContext& ctx, const Session& s) {
    const bool admin = AccessControl::isAllowed(s.role, Operation::ManageDoctors);
    while (!ConsoleUi::inputClosed()) {
        ConsoleUi::header("Doctors");
        std::cout << "  1. List doctors\n";
        if (admin) std::cout << "  2. Add doctor\n  3. Update doctor\n  4. Set availability\n";
        std::cout << "  0. Back\n";
        const int c = ConsoleUi::readChoice("Choice: ");
        if (c == 0) return;
        if (c == 1) listFlow(ctx, s);
        else if (c == 2 && admin) addFlow(ctx, s);
        else if (c == 3 && admin) updateFlow(ctx, s);
        else if (c == 4 && admin) availabilityFlow(ctx, s);
        else ConsoleUi::printError("E010", "Choice: enter a number from the menu.");
    }
}
}  // namespace hms
