#ifndef STATSHOLDERS_H
#define STATSHOLDERS_H

#include <functional>
#include <unordered_map>
#include <vector>
#include "NetworkPackets.hpp"

struct StatsHolder {
    // General
    uint8_t maxCombo = 0, maxBpm = 0, alert = 1;
    float avgBpm = 0;
    uint32_t totalPlayed = 0, totalWon = 0, totalBpm = 0, challenges_played = 0;

    // 1v1
    uint16_t vsPoints = 1500, vsRank = 0;
    uint32_t vsPlayed = 0, vsWon = 0;

    // FFA
    int16_t ffaPoints = 0;
    uint8_t ffaRank = 25;
    uint32_t ffaPlayed = 0, ffaWon = 0;

    // Hero
    uint16_t heroPoints = 1500, heroRank = 0;
    uint32_t heroPlayed = 0, heroWon = 0;

    // Tournament
    uint16_t gradeA = 0, gradeB = 0, gradeC = 0, gradeD = 0;
    uint32_t tournamentsPlayed = 0, tournamentsWon = 0;

    void updateFFARank();

    void set(const std::string& name, int64_t value) { setMap[name](this, value); }
    std::vector<std::pair<std::string, int64_t>> get(const std::string& name) {
        std::vector<std::pair<std::string, int64_t>> retVec;
        for (auto& pair : getMap[name]) retVec.emplace_back(pair.first, pair.second(this));
        return retVec;
    }

    static std::unordered_map<std::string, std::function<void(StatsHolder*, int64_t)>> setMap;

    using variable_holder = std::vector<std::pair<std::string, std::function<int64_t(StatsHolder*)>>>;
    static std::unordered_map<std::string, variable_holder> getMap;

    template <typename MP>
    static void bind(const std::string& table_name, const std::string& variable_name, MP ptr) {
        setMap.emplace(table_name + variable_name, [ptr](StatsHolder* stats, int64_t value) { stats->*(ptr) = value; });
        if (getMap.find(table_name) == getMap.end()) getMap.emplace(table_name, variable_holder{});
        getMap[table_name].emplace_back(variable_name, [ptr](StatsHolder* stats) -> int64_t { return stats->*(ptr); });
    }

    static void mapStringsToVariables();
};

struct RoundStats : NP_GameOver {
    uint8_t position, garbageCleared;
    uint16_t spm, game_score, linesCleared;
    float incLines, linesAdjusted;

    void clear() {
        score.max_combo = 0;
        position = 0;
        garbageCleared = 0;
        score.lines_sent = 0;
        score.lines_recieved = 0;
        score.lines_blocked = 0;
        score.bpm = 0;
        spm = 0;
        game_score = 0;
        piece_count = 0;
        linesCleared = 0;
        incLines = 0;
        linesAdjusted = 0;
    }

    RoundStats() { clear(); }

    using NP_GameOver::operator=;
};

#endif