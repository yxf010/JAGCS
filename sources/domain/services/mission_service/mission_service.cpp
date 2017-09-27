#include "mission_service.h"

// Qt
#include <QMap>
#include <QMutexLocker>
#include <QGeoCoordinate>

// Internal
#include "settings_provider.h"
#include "mission.h"
#include "mission_item.h"
#include "mission_assignment.h"

#include "generic_repository.h"
#include "generic_repository_impl.h"

using namespace dao;
using namespace domain;

class MissionService::Impl
{
public:
    QMutex mutex;

    GenericRepository<Mission> missionRepository;
    GenericRepository<MissionItem> itemRepository;
    GenericRepository<MissionAssignment> assignmentRepository;

    QMap <int, MissionItemPtr> currentItems;

    Impl():
        mutex(QMutex::Recursive),
        missionRepository("missions"),
        itemRepository("mission_items"),
        assignmentRepository("mission_assignments")
    {}

    void loadMissions(const QString& condition = QString())
    {
        for (int id: missionRepository.selectId(condition)) missionRepository.read(id);
    }

    void loadMissionItems(const QString& condition = QString())
    {
        for (int id: itemRepository.selectId(condition))
        {
            dao::MissionItemPtr item = itemRepository.read(id);

            if (item->sequence() != 0 || item->missionId() == 0) continue;

            missionRepository.read(item->missionId())->setHomeAltitude(item->altitude());
        }
    }

    void loadMissionAssignments(const QString& condition = QString())
    {
        for (int id: assignmentRepository.selectId(condition)) assignmentRepository.read(id);
    }
};

MissionService::MissionService(QObject* parent):
    QObject(parent),
    d(new Impl())
{
    qRegisterMetaType<dao::MissionPtr>("dao::MissionPtr");
    qRegisterMetaType<dao::MissionItemPtr>("dao::MissionItemPtr");
    qRegisterMetaType<dao::MissionAssignmentPtr>("dao::MissionAssignmentPtr");

    d->loadMissions();
    d->loadMissionItems();
    d->loadMissionAssignments();
}

MissionService::~MissionService()
{}

MissionPtr MissionService::mission(int id) const
{
    QMutexLocker locker(&d->mutex);

    return d->missionRepository.read(id);
}

MissionItemPtr MissionService::missionItem(int id) const
{
    QMutexLocker locker(&d->mutex);

    return d->itemRepository.read(id);
}

MissionAssignmentPtr MissionService::assignment(int id) const
{
    QMutexLocker locker(&d->mutex);

    return d->assignmentRepository.read(id);
}

MissionPtrList MissionService::missions() const
{
    QMutexLocker locker(&d->mutex);

    return d->missionRepository.loadedEntities();
}

MissionItemPtrList MissionService::missionItems() const
{
    QMutexLocker locker(&d->mutex);

    return d->itemRepository.loadedEntities();
}

MissionAssignmentPtrList MissionService::missionAssignments() const
{
    QMutexLocker locker(&d->mutex);

    return d->assignmentRepository.loadedEntities();
}

MissionItemPtr MissionService::currentWaypoint(int vehicleId) const
{
    QMutexLocker locker(&d->mutex);

    return d->currentItems.value(vehicleId);
}

bool MissionService::isItemCurrent(const MissionItemPtr& item) const
{
    QMutexLocker locker(&d->mutex);

    return d->currentItems.values().contains(item);
}

MissionAssignmentPtr MissionService::missionAssignment(int missionId) const
{
    QMutexLocker locker(&d->mutex);

    for (const MissionAssignmentPtr& assignment: d->assignmentRepository.loadedEntities())
    {
        if (assignment->missionId() == missionId) return assignment;
    }

    return MissionAssignmentPtr();
}

MissionAssignmentPtr MissionService::vehicleAssignment(int vehicleId) const
{
    QMutexLocker locker(&d->mutex);

    for (const MissionAssignmentPtr& assignment: d->assignmentRepository.loadedEntities())
    {
        if (assignment->vehicleId() == vehicleId) return assignment;
    }

    return MissionAssignmentPtr();
}

MissionItemPtrList MissionService::missionItems(int missionId) const
{
    QMutexLocker locker(&d->mutex);

    QMap<int, MissionItemPtr> items;
    for (const MissionItemPtr& item: d->itemRepository.loadedEntities())
    {
        if (item->missionId() == missionId) items[item->sequence()] = item;
    }
    return items.values();
}

MissionItemPtr MissionService::missionItem(int missionId, int sequence) const
{
    QMutexLocker locker(&d->mutex);

    for (const MissionItemPtr& item: d->itemRepository.loadedEntities())
    {
        if (item->missionId() == missionId && item->sequence() == sequence) return item;
    }
    return MissionItemPtr();
}

bool MissionService::save(const MissionPtr& mission)
{
    QMutexLocker locker(&d->mutex);

    bool isNew = mission->id() == 0;
    if (!d->missionRepository.save(mission)) return false;

    if (isNew)
    {
        settings::Provider::setValue(settings::mission::visibility + "/" + mission->id(), true);
    }

    emit (isNew ? missionAdded(mission) : missionChanged(mission));
    return true;
}

bool MissionService::save(const MissionItemPtr& item)
{
    QMutexLocker locker(&d->mutex);

    bool isNew = item->id() == 0;
    item->clearSuperfluousParameters();
    if (!d->itemRepository.save(item)) return false;

    if (isNew)
    {
        this->fixMissionItemOrder(item->missionId());
        emit missionItemAdded(item);
    }
    else
    {
        emit missionItemChanged(item);  // TODO: check changed flag
    }

    if (item->sequence() == 0 && item->missionId() != 0)
    {
        this->mission(item->missionId())->setHomeAltitude(item->altitude());
    }

    return true;
}

bool MissionService::save(const MissionAssignmentPtr& assignment)
{
    QMutexLocker locker(&d->mutex);

    bool isNew = assignment->id() == 0;
    if (!d->assignmentRepository.save(assignment)) return false;

    emit (isNew ? assignmentAdded(assignment) : assignmentChanged(assignment));
    return true;
}

bool MissionService::remove(const MissionPtr& mission)
{
    QMutexLocker locker(&d->mutex);

    MissionAssignmentPtr assignment = this->missionAssignment(mission->id());
    if (assignment && !this->remove(assignment)) return false;

    for (const MissionItemPtr& item: this->missionItems(mission->id()))
    {
        if (!this->remove(item)) return false;
    }

    settings::Provider::remove(settings::mission::visibility + "/" + mission->id());
    if (!d->missionRepository.remove(mission)) return false;

    emit missionRemoved(mission);
    return true;
}

bool MissionService::remove(const MissionItemPtr& item)
{
    QMutexLocker locker(&d->mutex);

    // TODO: remove from current
    if (!d->itemRepository.remove(item)) return false;

    this->fixMissionItemOrder(item->missionId());
    emit missionItemRemoved(item);
    return true;
}

bool MissionService::remove(const MissionAssignmentPtr& assignment)
{
    QMutexLocker locker(&d->mutex);

    for (const dao::MissionItemPtr& item: this->missionItems(assignment->missionId()))
    {
        item->setStatus(MissionItem::NotActual);
        emit missionItemChanged(item);
    }

    for (const dao::MissionItemPtr& item: d->currentItems.values())
    {
        if (item->missionId() != assignment->missionId()) continue;

        d->currentItems.remove(d->currentItems.key(item));
        emit missionItemChanged(item);
    }

    if (!d->assignmentRepository.remove(assignment)) return false;

    emit assignmentRemoved(assignment);
    return true;
}

void MissionService::unload(const MissionPtr& mission)
{
    QMutexLocker locker(&d->mutex);

    d->missionRepository.unload(mission->id());
}

void MissionService::unload(const MissionItemPtr& item)
{
    QMutexLocker locker(&d->mutex);

    d->itemRepository.unload(item->id());
}

void MissionService::unload(const MissionAssignmentPtr& assignment)
{
    QMutexLocker locker(&d->mutex);

    d->assignmentRepository.unload(assignment->id());
}

void MissionService::addNewMissionItem(int missionId)
{
    QMutexLocker locker(&d->mutex);

    MissionPtr mission = this->mission(missionId);
    if (mission.isNull()) return;

    MissionItemPtr item = MissionItemPtr::create();
    MissionItemPtr lastItem = this->missionItem(missionId, mission->count() - 1);
    item->setMissionId(missionId);

    if (mission->count() > 1) // TODO: default parms to settings
    {
        item->setCommand(MissionItem::Waypoint);

        if (lastItem)
        {
            item->setAltitudeRelative(lastItem->useAltitudeRelative());
            item->setAltitude(lastItem->altitude());
        }
    }
    else if (mission->count() == 1)
    {
        item->setCommand(MissionItem::Takeoff);
        if (lastItem)
        {
            item->setAltitude(lastItem->altitude() + 50); // TODO: default parms to settings
            item->setAltitudeRelative(true);
        }
        item->setParameter(MissionItem::Pitch, 15);
    }
    else
    {
        item->setCommand(MissionItem::Home);
        item->setAltitude(0);
        item->setAltitudeRelative(false);
    }

    item->setSequence(mission->count());

    this->save(item);
}

void MissionService::fixMissionItemOrder(int missionId)
{
    QMutexLocker locker(&d->mutex);

    int counter = 0;
    for (const MissionItemPtr& item : this->missionItems(missionId))
    {
        if (item->sequence() != counter)
        {
            item->setSequence(counter);
            item->setStatus(MissionItem::NotActual);
            this->save(item);
        }
        counter++;
    }

    MissionPtr mission = this->mission(missionId);
    if (mission->count() != counter)
    {
        mission->setCount(counter);
        this->save(mission);
    }
}

void MissionService::assign(int missionId, int vehicleId)
{
    QMutexLocker locker(&d->mutex);

    // Unassign current vehicle's assignment
    MissionAssignmentPtr vehicleAssignment = this->vehicleAssignment(vehicleId);
    if (vehicleAssignment)
    {
        if (vehicleAssignment->missionId() == missionId) return;

        if (!this->remove(vehicleAssignment)) return;
    }

    // Read current mission assignment, if exist
    MissionAssignmentPtr missionAssignment = this->missionAssignment(missionId);
    if (missionAssignment.isNull())
    {
        missionAssignment = MissionAssignmentPtr::create();
        missionAssignment->setMissionId(missionId);
    }
    else if (missionAssignment->vehicleId() == vehicleId) return;

    missionAssignment->setVehicleId(vehicleId);

    this->save(missionAssignment);

    for (const dao::MissionItemPtr& item: this->missionItems(missionId))
    {
        item->setStatus(MissionItem::NotActual);
        emit missionItemChanged(item);
    }
}

void MissionService::unassign(int missionId)
{
    QMutexLocker locker(&d->mutex);

    MissionAssignmentPtr assignment = this->missionAssignment(missionId);
    if (!assignment.isNull()) this->remove(assignment);
}

void MissionService::setCurrentItem(int vehicleId, const MissionItemPtr& item)
{
    QMutexLocker locker(&d->mutex);

    MissionItemPtr oldCurrent = d->currentItems.take(vehicleId);
    if (item) d->currentItems[vehicleId] = item;

    if (oldCurrent) emit missionItemChanged(oldCurrent);
    if (item) emit missionItemChanged(item);
    // TODO: drop current on vehicle's removing
}

