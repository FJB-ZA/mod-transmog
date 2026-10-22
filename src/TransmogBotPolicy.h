// Appearance rules for bot (headless) sessions: which looks they may collect and wear, and whether they are told.
#ifndef DEF_TRANSMOG_BOT_POLICY_H
#define DEF_TRANSMOG_BOT_POLICY_H

#include <cstdint>

namespace TransmogBotPolicy
{
// 0 disables the rule; 3 means rare (blue) and better.
constexpr bool Allows(bool bot, uint32_t quality, uint32_t botMinQuality)
{
    return !bot || quality >= botMinQuality;
}

// Nobody reads a bot's chat, so new-appearance messages are skipped.
constexpr bool Announces(bool bot) { return !bot; }
}

#endif
