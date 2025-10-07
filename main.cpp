#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <sstream>

using namespace std;

struct Submission {
    string problem;
    string status;
    int time;
};

struct ProblemStatus {
    bool solved;
    int solveTime;
    int wrongAttempts;
    vector<Submission> frozenSubs;
    bool wasSolvedBeforeFreeze;

    ProblemStatus() : solved(false), solveTime(0), wrongAttempts(0),
                      wasSolvedBeforeFreeze(false) {}
};

struct Team {
    string name;
    map<string, ProblemStatus> problems;
    vector<Submission> submissions;

    Team(string n = "") : name(n) {}
};

class ICPCSystem {
private:
    map<string, Team> teams;
    vector<string> teamNames;
    bool started;
    bool frozen;
    int durationTime;
    int problemCount;
    vector<string> problemList;
    vector<pair<string, int>> lastRanking;

    struct TeamRankInfo {
        string name;
        int solved;
        int penalty;
        vector<int> times;
    };

    TeamRankInfo getTeamRankInfo(const string& teamName) {
        TeamRankInfo info;
        info.name = teamName;
        info.solved = 0;
        info.penalty = 0;

        const Team& t = teams[teamName];
        for (const auto& prob : problemList) {
            if (t.problems.count(prob)) {
                const ProblemStatus& ps = t.problems.at(prob);
                if (ps.solved && ps.wasSolvedBeforeFreeze) {
                    info.solved++;
                    int penalty = ps.solveTime + 20 * ps.wrongAttempts;
                    info.penalty += penalty;
                    info.times.push_back(ps.solveTime);
                }
            }
        }
        sort(info.times.rbegin(), info.times.rend());
        return info;
    }

    void calculateRanking(vector<pair<string, int>>& ranking) {
        ranking.clear();
        vector<TeamRankInfo> infos;
        infos.reserve(teamNames.size());

        for (const auto& name : teamNames) {
            infos.push_back(getTeamRankInfo(name));
        }

        vector<int> indices(teamNames.size());
        for (int i = 0; i < teamNames.size(); i++) {
            indices[i] = i;
        }

        sort(indices.begin(), indices.end(), [&](int a, int b) {
            const TeamRankInfo& ta = infos[a];
            const TeamRankInfo& tb = infos[b];

            if (ta.solved != tb.solved) return ta.solved > tb.solved;
            if (ta.penalty != tb.penalty) return ta.penalty < tb.penalty;
            if (ta.times != tb.times) return ta.times < tb.times;
            return ta.name < tb.name;
        });

        for (int i = 0; i < indices.size(); i++) {
            ranking.push_back({teamNames[indices[i]], i + 1});
        }
    }

    void printScoreboard() {
        vector<pair<string, int>> ranking;
        calculateRanking(ranking);

        for (const auto& p : ranking) {
            const Team& t = teams[p.first];

            int solved = 0, penalty = 0;
            for (const auto& prob : problemList) {
                if (t.problems.count(prob)) {
                    const ProblemStatus& ps = t.problems.at(prob);
                    if (ps.solved && ps.wasSolvedBeforeFreeze) {
                        solved++;
                        penalty += ps.solveTime + 20 * ps.wrongAttempts;
                    }
                }
            }

            cout << t.name << " " << p.second << " " << solved << " " << penalty;

            for (const auto& prob : problemList) {
                cout << " ";
                if (t.problems.count(prob)) {
                    const ProblemStatus& ps = t.problems.at(prob);
                    if (ps.solved && ps.wasSolvedBeforeFreeze) {
                        cout << "+";
                        if (ps.wrongAttempts > 0) {
                            cout << ps.wrongAttempts;
                        }
                    } else if (!ps.frozenSubs.empty()) {
                        int wrongBefore = ps.wrongAttempts;
                        if (wrongBefore > 0) {
                            cout << "-";
                        }
                        cout << wrongBefore << "/" << ps.frozenSubs.size();
                    } else if (ps.wrongAttempts > 0) {
                        cout << "-" << ps.wrongAttempts;
                    } else {
                        cout << ".";
                    }
                } else {
                    cout << ".";
                }
            }
            cout << "\n";
        }
    }

public:
    ICPCSystem() : started(false), frozen(false), durationTime(0),
                   problemCount(0) {}

    void addTeam(const string& name) {
        if (started) {
            cout << "[Error]Add failed: competition has started.\n";
        } else if (teams.count(name)) {
            cout << "[Error]Add failed: duplicated team name.\n";
        } else {
            teams[name] = Team(name);
            teamNames.push_back(name);
            cout << "[Info]Add successfully.\n";
        }
    }

    void start(int duration, int problems) {
        if (started) {
            cout << "[Error]Start failed: competition has started.\n";
        } else {
            started = true;
            durationTime = duration;
            problemCount = problems;
            problemList.reserve(problems);
            for (int i = 0; i < problems; i++) {
                problemList.push_back(string(1, 'A' + i));
            }
            cout << "[Info]Competition starts.\n";
        }
    }

    void submit(const string& problem, const string& teamName,
                const string& status, int time) {
        Team& team = teams[teamName];
        team.submissions.push_back({problem, status, time});

        if (!team.problems.count(problem)) {
            team.problems[problem] = ProblemStatus();
        }

        ProblemStatus& ps = team.problems[problem];

        if (frozen && !ps.wasSolvedBeforeFreeze) {
            ps.frozenSubs.push_back({problem, status, time});
        } else if (!ps.solved) {
            if (status == "Accepted") {
                ps.solved = true;
                ps.solveTime = time;
                ps.wasSolvedBeforeFreeze = true;
            } else {
                ps.wrongAttempts++;
            }
        }
    }

    void flush(bool silent = false) {
        calculateRanking(lastRanking);
        if (!silent) {
            cout << "[Info]Flush scoreboard.\n";
        }
    }

    void freeze() {
        if (frozen) {
            cout << "[Error]Freeze failed: scoreboard has been frozen.\n";
        } else {
            frozen = true;
            for (auto& tp : teams) {
                Team& t = tp.second;
                for (auto& pp : t.problems) {
                    ProblemStatus& ps = pp.second;
                    if (ps.solved) {
                        ps.wasSolvedBeforeFreeze = true;
                    }
                }
            }
            cout << "[Info]Freeze scoreboard.\n";
        }
    }

    void scroll() {
        if (!frozen) {
            cout << "[Error]Scroll failed: scoreboard has not been frozen.\n";
            return;
        }

        cout << "[Info]Scroll scoreboard.\n";

        flush(true);
        printScoreboard();

        while (true) {
            bool hasFrozen = false;
            string lowestTeam = "";
            int lowestRank = 0;

            for (const auto& p : lastRanking) {
                const Team& t = teams[p.first];
                for (const auto& prob : problemList) {
                    if (t.problems.count(prob) && !t.problems.at(prob).frozenSubs.empty()) {
                        if (p.second > lowestRank) {
                            lowestRank = p.second;
                            lowestTeam = p.first;
                        }
                        hasFrozen = true;
                        break;
                    }
                }
            }

            if (!hasFrozen) break;

            Team& t = teams[lowestTeam];
            string unfreezeProb = "";
            for (const auto& prob : problemList) {
                if (t.problems.count(prob) && !t.problems[prob].frozenSubs.empty()) {
                    unfreezeProb = prob;
                    break;
                }
            }

            ProblemStatus& ps = t.problems[unfreezeProb];
            for (const auto& sub : ps.frozenSubs) {
                if (sub.status == "Accepted" && !ps.solved) {
                    ps.solved = true;
                    ps.solveTime = sub.time;
                    ps.wasSolvedBeforeFreeze = true;
                } else if (sub.status != "Accepted" && !ps.solved) {
                    ps.wrongAttempts++;
                }
            }
            ps.frozenSubs.clear();

            int oldRank = lowestRank;
            calculateRanking(lastRanking);

            int newRank = 0;
            for (const auto& p : lastRanking) {
                if (p.first == lowestTeam) {
                    newRank = p.second;
                    break;
                }
            }

            if (newRank < oldRank) {
                TeamRankInfo info = getTeamRankInfo(lowestTeam);

                string replacedTeam = "";
                for (const auto& p : lastRanking) {
                    if (p.second == newRank + 1) {
                        replacedTeam = p.first;
                        break;
                    }
                }

                cout << lowestTeam << " " << replacedTeam << " " << info.solved << " " << info.penalty << "\n";
            }
        }

        printScoreboard();

        frozen = false;
    }

    void queryRanking(const string& name) {
        if (!teams.count(name)) {
            cout << "[Error]Query ranking failed: cannot find the team.\n";
            return;
        }

        cout << "[Info]Complete query ranking.\n";
        if (frozen) {
            cout << "[Warning]Scoreboard is frozen. The ranking may be inaccurate until it were scrolled.\n";
        }

        int rank = 0;
        if (!lastRanking.empty()) {
            for (const auto& p : lastRanking) {
                if (p.first == name) {
                    rank = p.second;
                    break;
                }
            }
        } else {
            vector<string> sortedNames = teamNames;
            sort(sortedNames.begin(), sortedNames.end());
            for (int i = 0; i < sortedNames.size(); i++) {
                if (sortedNames[i] == name) {
                    rank = i + 1;
                    break;
                }
            }
        }

        cout << name << " NOW AT RANKING " << rank << "\n";
    }

    void querySubmission(const string& teamName, const string& problem,
                         const string& status) {
        if (!teams.count(teamName)) {
            cout << "[Error]Query submission failed: cannot find the team.\n";
            return;
        }

        cout << "[Info]Complete query submission.\n";

        const Team& t = teams[teamName];
        const Submission* found = nullptr;

        for (int i = t.submissions.size() - 1; i >= 0; i--) {
            const Submission& sub = t.submissions[i];
            if ((problem == "ALL" || sub.problem == problem) &&
                (status == "ALL" || sub.status == status)) {
                found = &sub;
                break;
            }
        }

        if (found) {
            cout << teamName << " " << found->problem << " "
                 << found->status << " " << found->time << "\n";
        } else {
            cout << "Cannot find any submission.\n";
        }
    }

    void end() {
        cout << "[Info]Competition ends.\n";
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    ICPCSystem system;
    string line;

    while (getline(cin, line)) {
        if (line.empty()) continue;

        istringstream iss(line);
        string command;
        iss >> command;

        if (command == "ADDTEAM") {
            string name;
            iss >> name;
            system.addTeam(name);
        } else if (command == "START") {
            string dummy;
            int duration, problems;
            iss >> dummy >> duration >> dummy >> problems;
            system.start(duration, problems);
        } else if (command == "SUBMIT") {
            string problem, by, teamName, with, status, at;
            int time;
            iss >> problem >> by >> teamName >> with >> status >> at >> time;
            system.submit(problem, teamName, status, time);
        } else if (command == "FLUSH") {
            system.flush();
        } else if (command == "FREEZE") {
            system.freeze();
        } else if (command == "SCROLL") {
            system.scroll();
        } else if (command == "QUERY_RANKING") {
            string name;
            iss >> name;
            system.queryRanking(name);
        } else if (command == "QUERY_SUBMISSION") {
            string teamName, where, rest;
            iss >> teamName >> where;
            getline(iss, rest);

            size_t probPos = rest.find("PROBLEM=");
            size_t statPos = rest.find("STATUS=");

            string problem = rest.substr(probPos + 8,
                statPos - probPos - 13);
            string status = rest.substr(statPos + 7);

            system.querySubmission(teamName, problem, status);
        } else if (command == "END") {
            system.end();
            break;
        }
    }

    return 0;
}
