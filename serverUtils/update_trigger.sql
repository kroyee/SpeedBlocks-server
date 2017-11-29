CREATE TRIGGER insert_profile_field
AFTER INSERT ON sb_stats FOR EACH ROW
INSERT INTO phpbb_profile_fields_data
(user_id, pf_s_avgbpm, pf_s_gamesplayed, pf_s_gameswon, pf_s_heropoints, pf_s_herorank, pf_s_maxbpm,
pf_s_maxcombo, pf_s_points, pf_s_rank, pf_s_totalgames, pf_s_totalbpm, pf_s_vspoints, pf_s_vsrank, pf_s_tourplayed,
pf_s_tourwon, pf_s_gradea, pf_s_gradeb, pf_s_gradec, pf_s_graded)
VALUES (NEW.user_id, NEW.avgbpm, NEW.gamesplayed, NEW.gameswon, IF(NEW.rank=0,NEW.heropoints,NULL), IF(NEW.rank=0,NEW.herorank,NULL), NEW.maxbpm, NEW.maxcombo,
NEW.points, NEW.rank, NEW.totalgames, NEW.totalbpm, NEW.1vs1points, NEW.1vs1rank, NEW.tournamentsplayed, NEW.tournamentswon,
NEW.gradeA, NEW.gradeB, NEW.gradeC, NEW.gradeD)
ON DUPLICATE KEY UPDATE pf_s_avgbpm=NEW.avgbpm,
pf_s_gamesplayed=NEW.gamesplayed, pf_s_gameswon=NEW.gameswon, pf_s_heropoints=IF(NEW.rank=0,NEW.heropoints,NULL), pf_s_herorank=IF(NEW.rank=0,NEW.herorank,NULL),
pf_s_maxbpm=NEW.maxbpm, pf_s_maxcombo=NEW.maxcombo, pf_s_points=NEW.points, pf_s_rank=NEW.rank, pf_s_totalgames=NEW.totalgames,
pf_s_totalbpm=NEW.totalbpm, pf_s_vspoints=NEW.1vs1points, pf_s_vsrank=NEW.1vs1rank, pf_s_tourplayed=NEW.tournamentsplayed,
pf_s_tourwon=NEW.tournamentswon, pf_s_gradea=NEW.gradeA, pf_s_gradeb=NEW.gradeB, pf_s_gradec=NEW.gradeC, pf_s_graded=NEW.gradeD;

INSERT INTO sb_stats
(user_id, avgbpm, gamesplayed, gameswon, heropoints, herorank, maxbpm,
maxcombo, points, rank, totalgames, totalbpm)
VALUES (2, 2, 411, 197, 0, 0, 155, 10,
1000, 16, 411, 1898);

när man kör inserat och värdet på nåtgot är null blir motsvarande profile fields tom

CREATE TRIGGER update_profile_field
AFTER UPDATE ON sb_stats FOR EACH ROW
INSERT INTO phpbb_profile_fields_data
(user_id, pf_s_avgbpm, pf_s_gamesplayed, pf_s_gameswon, pf_s_heropoints, pf_s_herorank, pf_s_maxbpm,
pf_s_maxcombo, pf_s_points, pf_s_rank, pf_s_totalgames, pf_s_totalbpm, pf_s_vspoints, pf_s_vsrank, pf_s_tourplayed,
pf_s_tourwon, pf_s_gradea, pf_s_gradeb, pf_s_gradec, pf_s_graded)
VALUES (NEW.user_id, NEW.avgbpm, NEW.gamesplayed, NEW.gameswon, IF(NEW.rank=0,NEW.heropoints,NULL), IF(NEW.rank=0,NEW.herorank,NULL), NEW.maxbpm, NEW.maxcombo,
NEW.points, NEW.rank, NEW.totalgames, NEW.totalbpm, NEW.1vs1points, NEW.1vs1rank, NEW.tournamentsplayed, NEW.tournamentswon,
NEW.gradeA, NEW.gradeB, NEW.gradeC, NEW.gradeD)
ON DUPLICATE KEY UPDATE pf_s_avgbpm=NEW.avgbpm,
pf_s_gamesplayed=NEW.gamesplayed, pf_s_gameswon=NEW.gameswon, pf_s_heropoints=IF(NEW.rank=0,NEW.heropoints,NULL), pf_s_herorank=IF(NEW.rank=0,NEW.herorank,NULL),
pf_s_maxbpm=NEW.maxbpm, pf_s_maxcombo=NEW.maxcombo, pf_s_points=NEW.points, pf_s_rank=NEW.rank, pf_s_totalgames=NEW.totalgames,
pf_s_totalbpm=NEW.totalbpm, pf_s_vspoints=NEW.1vs1points, pf_s_vsrank=NEW.1vs1rank, pf_s_tourplayed=NEW.tournamentsplayed,
pf_s_tourwon=NEW.tournamentswon, pf_s_gradea=NEW.gradeA, pf_s_gradeb=NEW.gradeB, pf_s_gradec=NEW.gradeC, pf_s_graded=NEW.gradeD;