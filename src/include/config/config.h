#pragma once

#include <string>
#include <cstdint>

namespace spdb {
#define page_id_t int32_t
#define slot_id_t int32_t
#define INVALID_PAGE_ID -1
#define PAGE_SIZE 4096

class RID{
    private:
    page_id_t pid_;
    slot_id_t sid_;
    public:
    explicit RID(page_id_t pid,slot_id_t sid):pid_(pid),sid_(sid){}

    auto GetPageId() const ->page_id_t{return pid_;}
    auto GetSlotId() const -> slot_id_t{return sid_;}
};

typedef enum{
    INVALID,
    INT,
    BOOL
}CloumType;

class Cloum{
    public:
    std::string cloum_name_;
    CloumType type_;

    explicit Cloum(std::string& name ,CloumType type):cloum_name_(name),type_(type){}
};
}  // namespace spdb