#pragma once

#include <QDateTime>
#include <map>

#include "kpulse/event.hpp"

namespace kpulse {

class BaselineTracker
{
public:
    void observe(const Event &event);
    double score(const Event &event) const;

private:
    struct Bucket {
        int count = 0;
        QDateTime windowStart;
    };

    std::map<Category, Bucket> buckets_;
};

} // namespace kpulse
