delimiter |

create event ranking
on schedule every 1 minute
do
begin
set @vsrank:=0;
update 1v1 set rank=(@vsrank:=@vsrank+1) where played != 0 order by points desc;
end |

delimiter ;

delimiter |

create event hero_ranking
on schedule every 1 minute
do
begin
set @newrank:=0;
update hero, ffa set hero.rank=10000 where hero.user_id=ffa.user_id and ffa.rank=0;
update hero set rank=(@newrank:=@newrank+1) where rank=10000 order by points desc;
end |

delimiter ;

delimiter |

create event ranking_backup
on schedule every 1 week
do
begin
insert into backup_1v1 select *, CURDATE() as date from 1v1;
insert into backup_ffa select *, CURDATE() as date from ffa;
insert into backup_hero select *, CURDATE() as date from hero;
insert into backup_gstats select *, CURDATE() as date from gstats;
insert into backup_tstats select *, CURDATE() as date from tstats;
end |

delimiter ;

update sb_stats A inner join sb_stats_backup B on A.user_id=B.user_id
set A.avgbpm=B.avgbpm, A.gamesplayed=B.gamesplayed, A.gameswon=B.gameswon, A.heropoints=B.heropoints,
A.herorank=B.herorank, A.maxbpm=B.maxbpm, A.maxcombo=B.maxcombo, A.points=B.points, A.rank=B.rank,
A.totalgames=B.totalgames, A.totalbpm=B.totalbpm, A.1vs1points=B.1vs1points, A.1vs1rank=B.1vs1rank,
A.tournamentsplayed=B.tournamentsplayed, A.tournamentswon=B.tournamentswon, A.gradeA=B.gradeA, A.gradeB=B.gradeB,
A.gradeC=B.gradeC, A.gradeD=B.gradeD where B.date='2017-07-17'