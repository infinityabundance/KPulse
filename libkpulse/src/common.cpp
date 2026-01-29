#include "kpulse/common.hpp"

#include <QTimeZone>

namespace kpulse {

QDateTime nowUtc()
{
    // Qt6-friendly way to get current UTC time.
    return QDateTime::currentDateTimeUtc();
    // Alternatively:
    // return QDateTime::currentDateTime().toTimeZone(QTimeZone::utc());
}

} // namespace kpulse
