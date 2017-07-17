#ifndef VERTICAL_PROFILE_PRESENTER_H
#define VERTICAL_PROFILE_PRESENTER_H

// Internal
#include "base_presenter.h"
#include "db_traits.h"

namespace db
{
    class DbFacade;
}

namespace presentation
{
    class VerticalProfilePresenter: public BasePresenter
    {
        Q_OBJECT

    public:
        explicit VerticalProfilePresenter(db::DbFacade* dbFacade,
                                          QObject* parent = nullptr);

    public slots:
        void selectMission(const db::MissionPtr& mission);
        void updateMission();
        void clearMission();

    protected:
        void connectView(QObject* view) override;

    private:
        db::DbFacade* m_dbFacade;
        db::MissionPtr m_mission;
    };
}

#endif // VERTICAL_PROFILE_PRESENTER_H