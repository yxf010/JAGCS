#include "navigation_display_presenter.h"

// Qt
#include <QDebug>

using namespace presentation;

NavigationDisplayPresenter::NavigationDisplayPresenter(QObject* parent):
    AbstractTelemetryPresenter(parent)
{}

void NavigationDisplayPresenter::connectNode(domain::Telemetry* node)
{
    this->chainNode(node->childNode(domain::Telemetry::Status),
                    std::bind(&NavigationDisplayPresenter::updateStatus, this, std::placeholders::_1));
    this->chainNode(node->childNode(domain::Telemetry::Compass),
                    std::bind(&NavigationDisplayPresenter::updateCompas, this, std::placeholders::_1));
    this->chainNode(node->childNode(domain::Telemetry::Satellite),
                    std::bind(&NavigationDisplayPresenter::updateSatellite, this, std::placeholders::_1));
    this->chainNode(node->childNode(domain::Telemetry::HomePosition),
                    std::bind(&NavigationDisplayPresenter::updateHome, this, std::placeholders::_1));
    this->chainNode(node->childNode(domain::Telemetry::Position),
                    std::bind(&NavigationDisplayPresenter::updatePosition, this, std::placeholders::_1));
    this->chainNode(node->childNode(domain::Telemetry::Navigator),
                    std::bind(&NavigationDisplayPresenter::updateNavigator, this, std::placeholders::_1));
    this->chainNode(node->childNode(domain::Telemetry::Wind),
                    std::bind(&NavigationDisplayPresenter::updateWind, this, std::placeholders::_1));
}

void NavigationDisplayPresenter::updateStatus(const domain::Telemetry::TelemetryMap& parameters)
{
    this->setViewProperty(PROPERTY(guided), parameters.value(domain::Telemetry::Guided));
}

void NavigationDisplayPresenter::updateCompas(const domain::Telemetry::TelemetryMap& parameters)
{
    this->setViewProperty(PROPERTY(compassEnabled), parameters.value(domain::Telemetry::Enabled, false));
    this->setViewProperty(PROPERTY(compassOperational), parameters.value(domain::Telemetry::Operational, false));
    this->setViewProperty(PROPERTY(heading), parameters.value(domain::Telemetry::Heading, 0));
}

void NavigationDisplayPresenter::updateSatellite(const domain::Telemetry::TelemetryMap& parameters)
{
    this->setViewProperty(PROPERTY(satelliteEnabled), parameters.value(domain::Telemetry::Enabled, false));
    this->setViewProperty(PROPERTY(satelliteOperational), parameters.value(domain::Telemetry::Operational, false));
    this->setViewProperty(PROPERTY(course), parameters.value(domain::Telemetry::Course, 0));
    this->setViewProperty(PROPERTY(groundspeed), parameters.value(domain::Telemetry::Groundspeed, 0));
}

void NavigationDisplayPresenter::updatePosition(const domain::Telemetry::TelemetryMap& parameters)
{
    m_position = parameters.value(domain::Telemetry::Coordinate).value<QGeoCoordinate>();

    this->updateHoming();
}

void NavigationDisplayPresenter::updateHome(const domain::Telemetry::TelemetryMap& parameters)
{
    m_homePosition = parameters.value(domain::Telemetry::Coordinate).value<QGeoCoordinate>();

    this->updateHoming();
}

void NavigationDisplayPresenter::updateNavigator(const domain::Telemetry::TelemetryMap& parameters)
{
    this->setViewProperty(PROPERTY(targetBearing), parameters.value(domain::Telemetry::TargetBearing, 0));
    this->setViewProperty(PROPERTY(desiredHeading), parameters.value(domain::Telemetry::DesiredHeading, 0));
    this->setViewProperty(PROPERTY(targetDistance), parameters.value(domain::Telemetry::TargetDistance, 0));
    this->setViewProperty(PROPERTY(trackError), parameters.value(domain::Telemetry::TrackError, 0));
}

void NavigationDisplayPresenter::updateWind(const domain::Telemetry::TelemetryMap& parameters)
{
    this->setViewProperty(PROPERTY(windDirection), parameters.value(domain::Telemetry::Yaw, false));
    this->setViewProperty(PROPERTY(windSpeed), parameters.value(domain::Telemetry::Speed, false));
}

void NavigationDisplayPresenter::updateHoming()
{
    if (m_position.isValid() && m_homePosition.isValid())
    {
        this->setViewProperty(PROPERTY(homeDistance), m_position.distanceTo(m_homePosition));
        this->setViewProperty(PROPERTY(homeDirection), m_position.azimuthTo(m_homePosition));
    }
    else
    {
        this->setViewProperty(PROPERTY(homeDistance), 0);
        this->setViewProperty(PROPERTY(homeDirection), 0);
    }
}
