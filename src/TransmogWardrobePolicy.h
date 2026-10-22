// The Wardrobe addon's protocol: hidden addon whispers between the player and themself, prefix "SoloWowWardrobe".
//
// Client to server:
//   "L <slot>"         list the appearances the equipped item in <slot> can take
//   "T <slot> <item>"  transmogrify <slot> to <item>, which must be in the list
//   "H <slot>"         hide <slot>
//   "R <slot>"         remove <slot>'s transmogrification
// Server to client:
//   "O <slot>:<fake>,..."              a Warpweaver's menu opened; each equipped slot with its look (0 none, 1 hidden)
//   "A <slot> <item>,<item>,..."       part of a list, as many as fit in one whisper
//   "E <slot> <count> <price> <hide>"  the list is complete: its size, the price in copper, 1 when hiding is allowed
//   "D <slot> <fake>"                  <slot>'s look after a change
#ifndef DEF_TRANSMOG_WARDROBE_POLICY_H
#define DEF_TRANSMOG_WARDROBE_POLICY_H

#include <array>
#include <charconv>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace TransmogWardrobe
{
inline constexpr char const* AddonPrefix = "SoloWowWardrobe";
// A whisper's text, prefix and tab included, must stay under the client's 255 characters.
inline constexpr size_t MaxBody = 230;

// The equipment slots mod-transmog changes: not the neck, rings or trinkets.
inline constexpr std::array<uint8_t, 14> Slots{{0, 2, 3, 4, 5, 6, 7, 8, 9, 14, 15, 16, 17, 18}};

constexpr bool IsSlot(uint32_t slot)
{
    for (uint8_t known : Slots)
        if (known == slot)
            return true;
    return false;
}

struct Request
{
    char command = 0;
    uint8_t slot = 0;
    uint32_t item = 0;
};

namespace Detail
{
// Reads " <number>" from the front of `text`
inline bool ReadField(std::string_view& text, uint32_t& value)
{
    if (text.size() < 2 || text[0] != ' ' || text[1] < '0' || text[1] > '9')
        return false;
    auto const [end, error] = std::from_chars(text.data() + 1, text.data() + text.size(), value);
    if (error != std::errc())
        return false;
    text.remove_prefix(static_cast<size_t>(end - text.data()));
    return true;
}
}  // namespace Detail

inline std::optional<Request> ParseRequest(std::string_view body)
{
    if (body.empty())
        return std::nullopt;

    Request request;
    request.command = body[0];
    body.remove_prefix(1);
    uint32_t slot = 0;
    if (!Detail::ReadField(body, slot) || !IsSlot(slot))
        return std::nullopt;
    request.slot = static_cast<uint8_t>(slot);

    switch (request.command)
    {
        case 'L':
        case 'H':
        case 'R':
            break;
        case 'T':
            if (!Detail::ReadField(body, request.item) || !request.item)
                return std::nullopt;
            break;
        default:
            return std::nullopt;
    }
    if (!body.empty())
        return std::nullopt;
    return request;
}

inline std::string FormatOpen(std::vector<std::pair<uint8_t, uint32_t>> const& looks)
{
    std::string text = "O ";
    for (size_t i = 0; i < looks.size(); ++i)
        text += (i ? "," : "") + std::to_string(looks[i].first) + ":" + std::to_string(looks[i].second);
    return text;
}

// The "A" parts, then the "E" line
inline std::vector<std::string> FormatList(uint8_t slot, std::vector<uint32_t> const& items, uint32_t price,
                                           bool hideAllowed)
{
    std::vector<std::string> lines;
    std::string const head = "A " + std::to_string(slot) + " ";
    std::string line;
    for (uint32_t item : items)
    {
        std::string const id = std::to_string(item);
        if (!line.empty() && line.size() + 1 + id.size() > MaxBody)
        {
            lines.push_back(line);
            line.clear();
        }
        line += line.empty() ? head + id : "," + id;
    }
    if (!line.empty())
        lines.push_back(line);
    lines.push_back("E " + std::to_string(slot) + " " + std::to_string(items.size()) + " " + std::to_string(price) +
                    " " + (hideAllowed ? "1" : "0"));
    return lines;
}

inline std::string FormatDone(uint8_t slot, uint32_t fake)
{
    return "D " + std::to_string(slot) + " " + std::to_string(fake);
}
}  // namespace TransmogWardrobe

#endif
