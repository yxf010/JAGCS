#include "vehicle_dashboard_factory.h"

// Internal
#include "telemetry_service.h"
#include "telemetry.h"

#include "vehicle.h"

#include "dashboard_presenter.h"
#include "status_presenter.h"
#include "ahrs_presenter.h"
#include "satellite_presenter.h"
#include "barometric_presenter.h"
#include "pitot_presenter.h"
#include "compass_presenter.h"

using namespace presentation;

VehicleDashboardFactory::VehicleDashboardFactory(domain::TelemetryService* telemetryService,
                                                 const db::VehiclePtr& vehicle):
    IDashboardFactory(),
    m_telemetryService(telemetryService),
    m_vehicle(vehicle)
{}

DashboardPresenter* VehicleDashboardFactory::create()
{
    domain::Telemetry* node = m_telemetryService->node(m_vehicle->id());
    if (!node) return nullptr;

    // TODO: vehicle type
    DashboardPresenter* dashboard = new DashboardPresenter();

    dashboard->addInstrument("fd", new StatusPresenter(
                                  node->childNode(domain::Telemetry::Status), dashboard));
    dashboard->addInstrument("fd", new AhrsPresenter(
                                 node->childNode(domain::Telemetry::Ahrs), dashboard));
    dashboard->addInstrument("fd", new SatellitePresenter(
                                 node->childNode(domain::Telemetry::Satellite), dashboard));
    dashboard->addInstrument("fd", new BarometricPresenter(
                                 node->childNode(domain::Telemetry::Barometric), dashboard));
    dashboard->addInstrument("fd", new PitotPresenter(
                                 node->childNode(domain::Telemetry::Pitot), dashboard));

    dashboard->addInstrument("nd", new CompassPresenter(
                                 node->childNode(domain::Telemetry::Compass), dashboard));
    dashboard->addInstrument("nd", new SatellitePresenter(
                                 node->childNode(domain::Telemetry::Satellite), dashboard));

    return dashboard;
}