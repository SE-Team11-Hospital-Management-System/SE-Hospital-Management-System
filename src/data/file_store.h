// C14 FileStore: the only component that touches data files (SAD 4.1 persistence protocol).
// Sprint 1: atomic write (temp file + rename), owner-only permissions, E041 on any write failure
// (HMS-F-060, HMS-NF-005, HMS-NF-011, HMS-SEC-006). Checksums (HMS-SEC-007) are added in Sprint 2.
#pragma once
#include "common/types.h"
#include <string>
#include <vector>
namespace hms {

class FileStore {
public:
    // Writes all lines to path. Either the whole new file is in place or the old file is untouched (E041).
    static Result<void> atomicWrite(const std::string& path, const std::vector<std::string>& lines);
    // Reads all lines. A missing file returns an empty list (first run). Unreadable file -> E040.
    static Result<std::vector<std::string>> load(const std::string& path);
    static bool exists(const std::string& path);
    // Creates the folder (and parents) with owner-only permissions (700).
    static Result<void> ensureDirectory(const std::string& dir);
};
}  // namespace hms
