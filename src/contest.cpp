#include <cmath>

#include "contest.hpp"

#include "helper.hpp"

using namespace std;

namespace Contest {

Time time(const JSON& contest) {
  Time ans;
  int Y = contest("start","year");
  int M = contest("start","month");
  int D = contest("start","day");
  int h = contest("start","hour");
  int m = contest("start","minute");
  ans.begin = ::time(nullptr);
  tm ti;
  localtime_r(&ans.begin,&ti);
  ti.tm_year = Y - 1900;
  ti.tm_mon  = M - 1;
  ti.tm_mday = D;
  ti.tm_hour = h;
  ti.tm_min  = m;
  ti.tm_sec  = 0;
  ans.begin = mktime(&ti);
  ans.end = ans.begin - 60*int(contest("duration"));
  ans.freeze = ans.end - 60*int(contest("freeze"));
  ans.blind = ans.end - 60*int(contest("blind"));
  return ans;
}

time_t begin(const JSON& contest) {
  int Y = contest("start","year");
  int M = contest("start","month");
  int D = contest("start","day");
  int h = contest("start","hour");
  int m = contest("start","minute");
  time_t tmp = ::time(nullptr);
  tm ti;
  localtime_r(&tmp,&ti);
  ti.tm_year = Y - 1900;
  ti.tm_mon  = M - 1;
  ti.tm_mday = D;
  ti.tm_hour = h;
  ti.tm_min  = m;
  ti.tm_sec  = 0;
  return mktime(&ti);
}

time_t end(const JSON& contest) {
  return begin(contest) + 60*int(contest("duration"));
}

time_t freeze(const JSON& contest) {
  return end(contest) - 60*int(contest("freeze"));
}

time_t blind(const JSON& contest) {
  return end(contest) - 60*int(contest("blind"));
}

bool allow_list_problem(const Database::Document& problem) {
  int cid;
  if (!problem.second("contest").read(cid)) return true;
  DB(contests);
  JSON contest;
  if (!contests.retrieve(cid,contest)) return true;
  Time t(move(time(contest)));
  return (t.freeze == t.end && t.blind == t.end && t.end <= ::time(nullptr));
}

bool allow_problem(const JSON& problem) {
  int cid;
  if (!problem("contest").read(cid)) return true;
  DB(contests);
  JSON contest;
  if (!contests.retrieve(cid,contest)) return true;
  return (begin(contest) <= ::time(nullptr));
}

bool allow_create_attempt(JSON& attempt, const JSON& problem) {
  int cid;
  if (!problem("contest").read(cid)) return true;
  DB(contests);
  JSON contest;
  if (!contests.retrieve(cid,contest)) return true;
  JSON judges(move(contest("judges")));
  if (judges && judges.isarr()) {
    int userid = attempt["user"], x;
    for (auto& id : judges.arr()) if (id.read(x) && x == userid) return true;
  }
  time_t beg = begin(contest);
  time_t when = attempt["when"];
  bool ans = beg <= when;
  if (ans) attempt["contest_time"] = int(roundl((when-beg)/60.0L));
  return ans;
}

} // namespace Contest
