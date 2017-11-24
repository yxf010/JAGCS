#ifndef MISSION_ITEM_EDIT_PRESENTER_H
#define MISSION_ITEM_EDIT_PRESENTER_H

// Internal
#include "base_presenter.h"
#include "dao_traits.h"

namespace presentation
{
    class MapHandle;

    // TODO: MissionItemListPresenter
    class MissionItemEditPresenter: public BasePresenter
    {
        Q_OBJECT

    public:
        explicit MissionItemEditPresenter(QObject* object = nullptr);
        ~MissionItemEditPresenter() override;

    public slots:
        void enablePicker();

        void setMission(int id);
        void selectItem(int index);
        void save();
        void remove();
        void changeSequence(int sequence);
        void updateMissionItem();
        void updateCount();
        void updateAvailableCommands();
        void updateCommand(int commandIndex);

    private slots: // TODO: replace
        void onMissionSelected(const dao::MissionPtr& mission);
        void onMissionItemSelected(const dao::MissionItemPtr& item);

    private:
         class Impl;
         QScopedPointer<Impl> const d;
    };
}

#endif // MISSION_ITEM_EDIT_PRESENTER_H