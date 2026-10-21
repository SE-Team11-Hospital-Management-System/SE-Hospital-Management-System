// Appointment screens: book with conflict handling, list a patient's appointments (HMS-F-030/031).
#include "ui/console_ui.h"
#include "ui/menus.h"
#include <iostream>
#include <sstream>
#include <vector>

namespace hms {

namespace {
std::vector<std::string> splitComma(const std::string& s) {
    std::vector<std::string> out; std::stringstream ss(s); std::string x;
    while (std::getline(ss, x, ',')) { while (!x.empty() && x.front() == ' ') x.erase(0, 1); if (!x.empty()) out.push_back(x); }
    return out;
}

void bookFlow(AppContext& ctx, const Session& s) {
    ConsoleUi::header("Book appointment");
    const std::string pid = ConsoleUi::readLine("Patient ID (e.g. P000001): ");
    auto p = ctx.patients.get(pid);
    if (!p) { ConsoleUi::printError("E020", "Patient not found."); return; }
    std::cout << "  -> " << p->fullName << ", " << p->gender << ", DOB " << p->dob << '\n';
    const std::string did = ConsoleUi::readLine("Doctor ID (e.g. D001): ");
    auto d = ctx.doctors.get(did);
    if (!d) { ConsoleUi::printError("E021", "Doctor not found."); return; }
    std::cout << "  -> " << d->fullName << ", " << d->specialization << '\n';
    const std::string date = ConsoleUi::readLine("Date (YYYY-MM-DD): ");
    const std::string time = ConsoleUi::readLine("Time (HH:MM, 30-minute steps): ");
    auto r = ctx.appointments.book(s, pid, did, date + " " + time);
    if (!r.ok() && (r.error().code == ErrorCode::E030 || r.error().code == ErrorCode::E031)) {
        ConsoleUi::printError(r.error());                                // the error already lists the next 3 free slots
        const auto alts = splitComma(r.error().details);
        if (alts.empty()) return;
        for (size_t i = 0; i < alts.size(); ++i) std::cout << "  " << i + 1 << ". " << alts[i] << '\n';
        const int c = ConsoleUi::readChoice("Pick one of these slots (0 = cancel): ");
        if (c < 1 || c > static_cast<int>(alts.size())) { std::cout << "Nothing was booked.\n"; return; }
        r = ctx.appointments.book(s, pid, did, alts[static_cast<size_t>(c - 1)]);
    }
    if (r.ok()) {
        auto a = ctx.appointments.get(r.value());
        ConsoleUi::printOk("Appointment " + r.value() + " booked: " + p->fullName + " with " + d->fullName + " at " + a->slotStart + " (SCHEDULED).");
    } else {
        ConsoleUi::printError(r.error());
    }
}

void listFlow(AppContext& ctx, const Session& s) {
    ConsoleUi::header("Appointments of a patient");
    const std::string pid = ConsoleUi::readLine("Patient ID (e.g. P000001): ");
    auto r = ctx.appointments.forPatient(s, pid);
    if (!r.ok()) { ConsoleUi::printError(r.error()); return; }
    if (r.value().empty()) std::cout << "No appointments.\n";
    for (const auto& a : r.value()) std::cout << "  " << a.appointmentId << "  " << a.slotStart << "  " << a.doctorId << "  " << toString(a.status) << '\n';
}
}  // namespace

void appointmentMenu(AppContext& ctx, const Session& s) {
    while (!ConsoleUi::inputClosed()) {
        ConsoleUi::header("Appointments");
        std::cout << "  1. Book appointment\n  2. List a patient's appointments\n  0. Back\n";
        const int c = ConsoleUi::readChoice("Choice: ");
        if (c == 0) return;
        if (c == 1) bookFlow(ctx, s);
        else if (c == 2) listFlow(ctx, s);
        else ConsoleUi::printError("E010", "Choice: enter a number from the menu.");
    }
}
}  // namespace hms
