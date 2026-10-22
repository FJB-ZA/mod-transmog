// The Wardrobe addon's server side (protocol in TransmogWardrobePolicy.h). Opening a Warpweaver's menu tells the
// addon, which shows its window beside the menu; its changes go through the same rules, prices and checks as the
// menu. Players without the addon keep using the menu.

#include <algorithm>
#include <climits>
#include <mutex>
#include <unordered_map>

#include "Chat.h"
#include "Creature.h"
#include "Item.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "Transmogrification.h"
#include "TransmogWardrobePolicy.h"
#include "WorldPacket.h"
#include "WorldSession.h"

// transmog_scripts.cpp
uint32 GetTransmogPrice(ItemTemplate const* targetItem);
std::vector<Item*> GetValidTransmogs(Player* player, Item* target, bool hasSearch, std::string searchTerm,
                                     std::vector<std::unique_ptr<Item>>& temporaries);
void PerformTransmogrification(Player* player, uint32 itemEntry, uint32 cost);
void RemoveTransmogrification(Player* player);

namespace
{
// The Warpweaver whose menu each player opened last; changes need them still in reach
std::mutex weaversLock;
std::unordered_map<ObjectGuid, ObjectGuid> weavers;

void Send(Player* player, std::string const& body)
{
    WorldPacket data;
    ChatHandler::BuildChatPacket(data, CHAT_MSG_WHISPER, LANG_ADDON, player, player,
                                 std::string(TransmogWardrobe::AddonPrefix) + '\t' + body);
    player->SendDirectMessage(&data);
}

uint32 LookOf(Player* player, uint8 slot)
{
    Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
    return item ? sTransmogrification->GetFakeEntry(item->GetGUID()) : 0;
}

bool NearWeaver(Player* player)
{
    ObjectGuid weaver;
    {
        std::lock_guard<std::mutex> guard(weaversLock);
        auto found = weavers.find(player->GetGUID());
        if (found == weavers.end())
            return false;
        weaver = found->second;
    }
    return player->GetNPCIfCanInteractWith(weaver, UNIT_NPC_FLAG_GOSSIP) != nullptr;
}

void SendList(Player* player, uint8 slot)
{
    Item* target = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
    std::vector<uint32> items;
    uint32 price = 0;
    if (target)
    {
        std::vector<std::unique_ptr<Item>> temporaries;
        for (Item* item : GetValidTransmogs(player, target, false, "", temporaries))
            items.push_back(item->GetEntry());
        price = GetTransmogPrice(target->GetTemplate());
    }
    for (std::string const& line :
         TransmogWardrobe::FormatList(slot, items, price, target && sTransmogrification->GetAllowHiddenTransmog()))
        Send(player, line);
}

void Change(Player* player, TransmogWardrobe::Request const& request)
{
    Item* target = player->GetItemByPos(INVENTORY_SLOT_BAG_0, request.slot);
    if (!target || !NearWeaver(player))
        return;

    sTransmogrification->SetSelectedSlot(player->GetGUID(), request.slot);
    uint32 const price = GetTransmogPrice(target->GetTemplate());
    if (request.command == 'R')
        RemoveTransmogrification(player);
    else if (request.command == 'H')
    {
        if (sTransmogrification->GetAllowHiddenTransmog())
            PerformTransmogrification(player, UINT_MAX, sTransmogrification->GetHiddenTransmogIsFree() ? 0 : price);
    }
    else
    {
        std::vector<std::unique_ptr<Item>> temporaries;
        std::vector<Item*> const allowed = GetValidTransmogs(player, target, false, "", temporaries);
        bool const listed = std::any_of(allowed.begin(), allowed.end(),
                                        [&](Item* item) { return item->GetEntry() == request.item; });
        if (listed)
            PerformTransmogrification(player, request.item, price);
    }
    Send(player, TransmogWardrobe::FormatDone(request.slot, LookOf(player, request.slot)));
}
}  // namespace

// Called when a Warpweaver's menu opens (transmog_scripts.cpp)
void TransmogWardrobe_Open(Player* player, Creature* weaver)
{
    if (player->GetSession()->IsHeadless())
        return;

    {
        std::lock_guard<std::mutex> guard(weaversLock);
        weavers[player->GetGUID()] = weaver->GetGUID();
    }
    std::vector<std::pair<uint8_t, uint32_t>> looks;
    for (uint8 slot : TransmogWardrobe::Slots)
        if (player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
            looks.emplace_back(slot, LookOf(player, slot));
    Send(player, TransmogWardrobe::FormatOpen(looks));
}

class TransmogWardrobeScript : public PlayerScript
{
public:
    TransmogWardrobeScript()
        : PlayerScript("TransmogWardrobeScript", {PLAYERHOOK_CAN_PLAYER_USE_PRIVATE_CHAT, PLAYERHOOK_ON_LOGOUT}) { }

    bool OnPlayerCanUseChat(Player* player, uint32 type, uint32 language, std::string& msg, Player* receiver) override
    {
        std::string const prefix = std::string(TransmogWardrobe::AddonPrefix) + '\t';
        if (language != LANG_ADDON || type != CHAT_MSG_WHISPER || receiver != player || msg.rfind(prefix, 0) != 0)
            return true;

        if (std::optional<TransmogWardrobe::Request> request =
                TransmogWardrobe::ParseRequest(std::string_view(msg).substr(prefix.size())))
        {
            if (request->command == 'L')
                SendList(player, request->slot);
            else
                Change(player, *request);
        }
        return false;
    }

    void OnPlayerLogout(Player* player) override
    {
        std::lock_guard<std::mutex> guard(weaversLock);
        weavers.erase(player->GetGUID());
    }
};

void AddSC_transmog_wardrobe()
{
    new TransmogWardrobeScript();
}
