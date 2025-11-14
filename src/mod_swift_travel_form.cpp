#include "ScriptMgr.h"
#include "Config.h"
#include "Player.h"
#include "Unit.h"
#include "Item.h"
#include "SharedDefines.h"
#include "EventProcessor.h"
#include "World.h"

namespace
{
    bool   sEnabled = true;
    uint32 sRequiredItemId = 0;
    uint32 sRequiredEquipId = 0;
    uint32 sRequiredSpellId = 0;
    uint32 sMinLevel = 60;
    bool   sNotInCombat = true;

    bool HasEquippedItem(Player* player, uint32 entry)
    {
        if (!player)
            return false;
        if (!entry)
            return true;

        for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
        {
            if (Item* it = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
                if (it->GetEntry() == entry)
                    return true;
        }
        return false;
    }

    bool RequirementsMet(Player* player)
    {
        if (!player)
            return false;
        if (!sEnabled)
            return false;
        if (player->GetLevel() < sMinLevel)
            return false;
        if (sRequiredItemId && !player->HasItemCount(sRequiredItemId, 1, false))
            return false;
        if (sRequiredEquipId && !HasEquippedItem(player, sRequiredEquipId))
            return false;
        if (sRequiredSpellId && !player->HasSpell(sRequiredSpellId))
            return false;
        if (sNotInCombat && player->IsInCombat())
            return false;
        return true;
    }

    class SwiftTravelEvent : public BasicEvent
    {
    public:
        SwiftTravelEvent(Player* player, uint8 form) : _player(player), _form(form) {}
        bool Execute(uint64, uint32) override
        {
            if (_player && _player->IsInWorld())
                if (_form == FORM_TRAVEL)
                    _player->SetSpeed(MOVE_RUN, 2.0f, true);
            return true;
        }
    private:
        Player* _player;
        uint8 _form;
    };
}

class SwiftTravelForm_WorldScript : public WorldScript
{
public:
    SwiftTravelForm_WorldScript() : WorldScript("SwiftTravelForm_WorldScript") {}

    void OnAfterConfigLoad(bool) override
    {
        sEnabled = sConfigMgr->GetOption<bool>("SwiftTravelForm.Enable", true);
        sRequiredItemId = sConfigMgr->GetOption<uint32>("SwiftTravelForm.RequiredItem", 0u);
        sRequiredEquipId = sConfigMgr->GetOption<uint32>("SwiftTravelForm.RequiredEquipment", 0u);
        sRequiredSpellId = sConfigMgr->GetOption<uint32>("SwiftTravelForm.RequiredSpell", 0u);
        sMinLevel = sConfigMgr->GetOption<uint32>("SwiftTravelForm.MinLevel", 60u);
        sNotInCombat = sConfigMgr->GetOption<bool>("SwiftTravelForm.NotInCombat", true);
    }
};

class SwiftTravelForm_UnitScript : public UnitScript
{
public:
    SwiftTravelForm_UnitScript()
    : UnitScript("SwiftTravelForm_UnitScript", true, { UNITHOOK_ON_UNIT_SET_SHAPESHIFT_FORM })
    {
    }

    void OnUnitSetShapeshiftForm(Unit* unit, uint8 form) override
    {
        if (!sEnabled)
            return;
        if (!unit || unit->GetTypeId() != TYPEID_PLAYER)
            return;

        Player* player = unit->ToPlayer();
        if (!player)
            return;
        if (!RequirementsMet(player))
            return;

        player->m_Events.AddEvent(new SwiftTravelEvent(player, form), player->m_Events.CalculateTime(50));
    }
};

void AddSC_swift_travel_form()
{
    new SwiftTravelForm_WorldScript();
    new SwiftTravelForm_UnitScript();
}
