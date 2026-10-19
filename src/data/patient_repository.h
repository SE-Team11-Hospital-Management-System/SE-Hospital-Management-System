// Part of C13: patient storage with in-memory index (SAD 4.1), saved to <dataDir>/patients.dat.
// File format, one patient per line:  id|fullName|dob|gender|phone|address|bloodGroup|active(1/0)|registeredOn
#pragma once
#include "common/types.h"
#include <map>
#include <optional>
#include <string>
#include <vector>
namespace hms {

struct Patient {
    std::string patientId, fullName, dob, gender, phone, address, bloodGroup;
    bool active{true};
    std::string registeredOn;
};

class PatientRepository {
public:
    std::string nextId() const;                                       // P000001, P000002 ... (does not reserve)
    void put(const Patient& p);                                       // in memory only
    void remove(const std::string& id) { byId_.erase(id); }            // only used to undo a failed save
    std::optional<Patient> findById(const std::string& id) const;
    std::vector<Patient> findByNameContains(const std::string& fragment) const;   // case-insensitive
    std::vector<Patient> findByPhone(const std::string& phone) const;
    bool existsDuplicate(const std::string& name, const std::string& dob, const std::string& phone, std::string* existingId = nullptr) const;
    size_t size() const { return byId_.size(); }

    Result<void> load(const std::string& path);                       // replaces memory with file contents
    Result<void> persist(const std::string& path) const;              // writes memory to file (atomic)
private:
    std::map<std::string, Patient> byId_;
    int counter_{0};
};
}  // namespace hms
