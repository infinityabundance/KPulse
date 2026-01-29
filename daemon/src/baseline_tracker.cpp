#include "baseline_tracker.hpp"

namespace kpulse {

static constexpr int WindowSeconds = 300;

void BaselineTracker::observe(const Event &event)
{
    auto &bucket = buckets_[event.category];

    if (!bucket.windowStart.isValid() || bucket.windowStart.secsTo(event.timestamp) > WindowSeconds) {
        bucket.windowStart = event.timestamp;
        bucket.count = 0;
    }

    bucket.count++;
}

double BaselineTracker::score(const Event &event) const
{
    auto it = buckets_.find(event.category);
    if (it == buckets_.end()) {
        return 0.0;
    }

    const Bucket &bucket = it->second;
    if (bucket.count == 0) {
        return 0.0;
    }

    return static_cast<double>(bucket.count);
}

} // namespace kpulse
