// Login screen, first-run set-up and the role-based main menu (HMS-F-001, F-003, F-004, SEC-004, SEC-008).
#include "common/datetime.h"
#include "security/access_control.h"
#include "ui/console_ui.h"
#include "ui/menus.h"
#include <iostream>
#include <vector>

namespace hms {

namespace {

void firstRunSetup(AppContext& ctx) {                                     // SRS 2.6: create the first Administrator
    ConsoleUi::header("First-run set-up");
    std::cout << "No user accounts exist yet. Create the first Administrator account.\n";
    while (!ctx.auth.hasUsers() && !ConsoleUi::inputClosed()) {
        const std::string name = ConsoleUi::readLine("Administrator username (4-20 letters or digits): ");
        const std::string pwd = ConsoleUi::readPassword("Password (8-32 characters, at least one letter and one digit): ");
        const std::string again = ConsoleUi::readPassword("Repeat password: ");
        if (pwd != again) { ConsoleUi::printError("E010", "Password: the two entries do not match."); continue; }
        auto r = ctx.auth.bootstrapAdmin(name, pwd);
        if (r.ok()) ConsoleUi::printOk("Administrator " + name + " created (" + r.value() + "). Please log in.");
        else ConsoleUi::printError(r.error());
    }
}

struct MenuItem { const char* label; Operation op; void (*run)(AppContext&, const Session&); };

void mainMenu(AppContext& ctx, const Session& s, const std::string& username) {
    // HMS-F-003: only the functions allowed for the role are displayed (SRS Table 6.5-1).
    static const MenuItem items[] = {
        {"Patients", Operation::SearchPatient, patientMenu},
        {"Doctors", Operation::ListDoctors, doctorMenu},
        {"Appointments", Operation::BookAppointment, appointmentMenu},
        {"User accounts", Operation::ManageUsers, userMenu},
        {"Backup and restore", Operation::Backup, backupMenu},
    };
    while (!ConsoleUi::inputClosed()) {
        ConsoleUi::header(std::string("HMS - ") + username + " (" + roleName(s.role) + ")   " + dt::now());
        std::vector<const MenuItem*> shown;
        for (const auto& it : items)
            if (AccessControl::isAllowed(s.role, it.op)) { shown.push_back(&it); std::cout << "  " << shown.size() << ". " << it.label << '\n'; }
        std::cout << "  0. Log out\n";
        const int c = ConsoleUi::readChoice("Choice: ");
        if (c == 0) break;
        if (c < 1 || c > static_cast<int>(shown.size())) { ConsoleUi::printError("E010", "Choice: enter a number from the menu."); continue; }
        shown[static_cast<size_t>(c - 1)]->run(ctx, s);
    }
    ctx.auth.logout(s);
    std::cout << "Logged out.\n";
}
}  // namespace

void userMenu(AppContext& ctx, const Session& s) {                       // HMS-F-004 (Administrator only)
    while (!ConsoleUi::inputClosed()) {
        ConsoleUi::header("User accounts");
        std::cout << "  1. List users\n  2. Create user\n  3. Deactivate user\n  4. Reactivate user\n  0. Back\n";
        const int c = ConsoleUi::readChoice("Choice: ");
        if (c == 0) return;
        if (c == 1) {
            auto r = ctx.auth.listUsers(s);
            if (!r.ok()) { ConsoleUi::printError(r.error()); continue; }
            for (const auto& u : r.value())
                std::cout << "  " << u.userId << "  " << u.username << "  " << roleName(u.role) << "  " << (u.active ? "Active" : "Inactive") << '\n';
        } else if (c == 2) {
            const std::string name = ConsoleUi::readLine("New username (4-20 letters or digits): ");
            const std::string pwd = ConsoleUi::readPassword("Temporary password (8-32, letter + digit): ");
            const int role = ConsoleUi::readChoice("Role: 1 = Administrator, 2 = Receptionist, 3 = Doctor: ");
            if (role < 1 || role > 3) { ConsoleUi::printError("E010", "Role: choose 1, 2 or 3."); continue; }
            auto r = ctx.auth.createUser(s, name, pwd, static_cast<Role>(role - 1));
            if (r.ok()) ConsoleUi::printOk("User " + name + " created with ID " + r.value() + ".");
            else ConsoleUi::printError(r.error());
        } else if (c == 3 || c == 4) {
            const std::string id = ConsoleUi::readLine("User ID (e.g. U00002): ");
            auto r = ctx.auth.setUserStatus(s, id, c == 4);
            if (r.ok()) ConsoleUi::printOk(std::string("User ") + id + (c == 4 ? " reactivated." : " deactivated."));
            else ConsoleUi::printError(r.error());
        } else {
            ConsoleUi::printError("E010", "Choice: enter a number from the menu.");
        }
    }
}

int runApp(AppContext& ctx) {
    std::cout << "HMS - Hospital Management System (Team 11)\n";
    if (!ctx.auth.hasUsers()) firstRunSetup(ctx);
    while (!ConsoleUi::inputClosed()) {                                  // HMS-F-001: nothing is reachable before login
        ConsoleUi::header("Login");
        const std::string user = ConsoleUi::readLine("Username (or 'quit'): ");
        if (user == "quit" || ConsoleUi::inputClosed()) break;
        const std::string pwd = ConsoleUi::readPassword("Password: ");  // HMS-SEC-004: not echoed
        auto r = ctx.auth.login(user, pwd);
        if (!r.ok()) { ConsoleUi::printError(r.error()); continue; }   // HMS-SEC-008: same message for every cause
        mainMenu(ctx, r.value(), user);
    }
    std::cout << "Goodbye.\n";
    return 0;
}
}  // namespace hms
