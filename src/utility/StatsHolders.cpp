#include "StatsHolders.h"

std::unordered_map<std::string, std::function<void(StatsHolder*, uint32_t)>> StatsHolder::setMap;
std::unordered_map<std::string, StatsHolder::variable_holder> StatsHolder::getMap;

void StatsHolder::mapStringsToVariables() {
	StatsHolder::bind("gstats", "maxcombo", &StatsHolder::maxCombo);
	StatsHolder::bind("gstats", "maxbpm", &StatsHolder::maxBpm);
	StatsHolder::bind("gstats", "avgbpm", &StatsHolder::avgBpm);
	StatsHolder::bind("gstats", "won", &StatsHolder::totalWon);
	StatsHolder::bind("gstats", "played", &StatsHolder::totalPlayed);
	StatsHolder::bind("gstats", "totalbpm", &StatsHolder::totalBpm);
	StatsHolder::bind("gstats", "alert", &StatsHolder::alert);
	StatsHolder::bind("gstats", "challenges_played", &StatsHolder::challenges_played);
	StatsHolder::bind("1v1", "rank", &StatsHolder::vsRank);
	StatsHolder::bind("1v1", "points", &StatsHolder::vsPoints);
	StatsHolder::bind("1v1", "played", &StatsHolder::vsPlayed);
	StatsHolder::bind("1v1", "won", &StatsHolder::vsWon);
	StatsHolder::bind("hero", "points", &StatsHolder::heroPoints);
	StatsHolder::bind("hero", "rank", &StatsHolder::heroRank);
	StatsHolder::bind("hero", "played", &StatsHolder::heroPlayed);
	StatsHolder::bind("hero", "won", &StatsHolder::heroWon);
	StatsHolder::bind("ffa", "points", &StatsHolder::ffaPoints);
	StatsHolder::bind("ffa", "rank", &StatsHolder::ffaRank);
	StatsHolder::bind("ffa", "played", &StatsHolder::ffaPlayed);
	StatsHolder::bind("ffa", "won", &StatsHolder::ffaWon);
	StatsHolder::bind("tstats", "played", &StatsHolder::tournamentsPlayed);
	StatsHolder::bind("tstats", "won", &StatsHolder::tournamentsWon);
	StatsHolder::bind("tstats", "gradeA", &StatsHolder::gradeA);
	StatsHolder::bind("tstats", "gradeB", &StatsHolder::gradeB);
	StatsHolder::bind("tstats", "gradeC", &StatsHolder::gradeC);
	StatsHolder::bind("tstats", "gradeD", &StatsHolder::gradeD);
}