#include "assignment.h"

Assignment::Assignment(const std::string &memberId, const std::string &slipId,
                       Status status, const Dimensions &dimensions, const std::string &comment)
    : mMemberId(memberId), mSlipId(slipId), mStatus(status), mBoatDimensions(dimensions), mComment(comment) {
}

bool Assignment::assigned() const {
    return !mSlipId.empty();
}

std::string Assignment::statusToString(Status status) {
    switch (status) {
        case Status::PERMANENT:
            return "PERMANENT";
        case Status::SAME:
            return "SAME";
        case Status::NEW:
            return "NEW";
        case Status::UNASSIGNED:
            return "UNASSIGNED";
    }
    return "UNKNOWN";
}
