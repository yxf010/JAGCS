#include "units.h"

// Qt
#include <QMap>

using namespace utils;

namespace
{
    QMap<Units::SpeedUnits, float> speedUnitCoefs =
    {
        { Units::Mps, 1.0 },
        { Units::Kph, 3.6 },
        { Units::Knots, 1.943844 },
        { Units::Mph, 2.236936 }
    };

    QMap<Units::DistanceUnits, float> distanceUnitCoefs =
    {
        { Units::Meters, 1.0 },
        { Units::Kilometers, 1000.0 },
        { Units::Feets, 0.3048 },
        { Units::Miles, 1609.344 }
    };
}

Units::Units(QObject* parent):
    QObject(parent)
{}

float Units::convertSpeed(SpeedUnits to, float value)
{
    return value * ::speedUnitCoefs[to];
}

float Units::convertSpeed(SpeedUnits from, SpeedUnits to, float value)
{
    return Units::convertSpeed(to, value / ::speedUnitCoefs[from]);
}

float Units::convertDistance(DistanceUnits to, float value)
{
    return value / ::distanceUnitCoefs[to];
}

float Units::convertDistance(DistanceUnits from, DistanceUnits to, float value)
{
    return Units::convertDistance(to, value * ::distanceUnitCoefs[from]);
}

QString Units::trSpeedUnits(SpeedUnits unit)
{
    switch (unit)
    {
    case Mps: return tr("m/s");
    case Kph: return tr("km/h");
    case Knots: return tr("knot");
    case Mph: return tr("mph");
    default: return QString();
    }
}

QString Units::trDistanceUnits(DistanceUnits unit)
{
    switch (unit)
    {
    case Meters: return tr("m");
    case Kilometers: return tr("km");
    case Feets: return tr("ft");
    case Miles: return tr("mi");
    default: return QString();
    }
}
