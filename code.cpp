#include <iostream>
#include <string>
#include <map>
#include <set>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iomanip>

using namespace std;

enum Status { Accepted, Wrong_Answer, Runtime_Error, Time_Limit_Exceed };

struct Submission {
    string problem;
    Status status;
    int time;
};

struct ProblemInfo {
    int wrong_attempts = 0;        // Total wrong attempts (before freeze or after scroll)
    int wrong_before_freeze = 0;   // Wrong attempts before freeze
    int wrong_before_accept = 0;   // Wrong attempts before first accept
    int freeze_submissions = 0;    // Submissions after freeze (for display)
    int accept_time = -1;          // Time when first accepted
    bool solved = false;           // Whether this problem is solved
    bool frozen = false;           // Whether this problem is frozen
    vector<Submission> frozen_subs; // Submissions during freeze period
};

struct Team {
    string name;
    map<string, ProblemInfo> problems;
    vector<Submission> submissions;
    
    int solved_count = 0;
    int total_penalty = 0;
    bool frozen = false;  // Whether team has any frozen problems
    mutable vector<int> cached_solve_times; // Cache for comparison
    mutable bool cache_valid = false;
    
    void updateSolvedAndPenalty() {
        solved_count = 0;
        total_penalty = 0;
        for (auto& p : problems) {
            if (p.second.solved) {
                solved_count++;
                total_penalty += 20 * p.second.wrong_before_accept + p.second.accept_time;
            }
        }
        cache_valid = false; // Invalidate cache
    }
    
    const vector<int>& getSortedSolveTimes() const {
        if (!cache_valid) {
            cached_solve_times.clear();
            for (const auto& p : problems) {
                if (p.second.solved) {
                    cached_solve_times.push_back(p.second.accept_time);
                }
            }
            sort(cached_solve_times.rbegin(), cached_solve_times.rend()); // Descending order
            cache_valid = true;
        }
        return cached_solve_times;
    }
};

class ICPCSystem {
private:
    map<string, Team> teams;
    int duration_time = 0;
    int problem_count = 0;
    bool started = false;
    bool frozen = false;
    bool first_flush = true;
    vector<string> team_ranking; // Current ranking
    bool ranking_valid = false; // Whether ranking is up to date
    
    Status parseStatus(const string& s) {
        if (s == "Accepted") return Accepted;
        if (s == "Wrong_Answer") return Wrong_Answer;
        if (s == "Runtime_Error") return Runtime_Error;
        return Time_Limit_Exceed;
    }
    
    string statusToString(Status s) {
        switch(s) {
            case Accepted: return "Accepted";
            case Wrong_Answer: return "Wrong_Answer";
            case Runtime_Error: return "Runtime_Error";
            case Time_Limit_Exceed: return "Time_Limit_Exceed";
        }
        return "";
    }
    
    void updateRanking() {
        if (ranking_valid) return; // Already up to date
        
        vector<pair<string, Team*>> team_ptrs;
        for (auto& t : teams) {
            team_ptrs.push_back({t.first, &t.second});
        }
        
        sort(team_ptrs.begin(), team_ptrs.end(), 
            [this](const pair<string, Team*>& a, const pair<string, Team*>& b) {
                return compareTeams(*(a.second), *(b.second));
            });
        
        team_ranking.clear();
        for (auto& p : team_ptrs) {
            team_ranking.push_back(p.first);
        }
        
        ranking_valid = true;
    }
    
    void invalidateRanking() {
        ranking_valid = false;
    }
    
    bool compareTeams(const Team& a, const Team& b) {
        // Compare by solved count (descending)
        if (a.solved_count != b.solved_count) {
            return a.solved_count > b.solved_count;
        }
        
        // Compare by penalty (ascending)
        if (a.total_penalty != b.total_penalty) {
            return a.total_penalty < b.total_penalty;
        }
        
        // Compare by solve times
        vector<int> times_a = a.getSortedSolveTimes();
        vector<int> times_b = b.getSortedSolveTimes();
        
        for (size_t i = 0; i < min(times_a.size(), times_b.size()); i++) {
            if (times_a[i] != times_b[i]) {
                return times_a[i] < times_b[i];
            }
        }
        
        // Compare by team name (lexicographically)
        return a.name < b.name;
    }
    
    string formatProblem(const ProblemInfo& info) {
        if (info.frozen) {
            // Frozen: -x/y or 0/y
            if (info.wrong_before_freeze == 0) {
                return "0/" + to_string(info.freeze_submissions);
            } else {
                return "-" + to_string(info.wrong_before_freeze) + "/" + to_string(info.freeze_submissions);
            }
        } else if (info.solved) {
            // Solved: +x or +
            if (info.wrong_before_accept == 0) {
                return "+";
            } else {
                return "+" + to_string(info.wrong_before_accept);
            }
        } else {
            // Not solved: -x or .
            if (info.wrong_attempts == 0) {
                return ".";
            } else {
                return "-" + to_string(info.wrong_attempts);
            }
        }
    }
    
    void printScoreboard() {
        updateRanking();
        int rank = 1;
        for (const string& team_name : team_ranking) {
            const Team& team = teams[team_name];
            cout << team_name << " " << rank << " " << team.solved_count << " " << team.total_penalty;
            
            // Print problem statuses
            for (int i = 0; i < problem_count; i++) {
                string prob = string(1, 'A' + i);
                cout << " ";
                if (team.problems.find(prob) != team.problems.end()) {
                    cout << formatProblem(team.problems.at(prob));
                } else {
                    cout << ".";
                }
            }
            cout << "\n";
            rank++;
        }
    }
    
public:
    void addTeam(const string& name) {
        if (started) {
            cout << "[Error]Add failed: competition has started.\n";
            return;
        }
        
        if (teams.find(name) != teams.end()) {
            cout << "[Error]Add failed: duplicated team name.\n";
            return;
        }
        
        teams[name] = Team{name};
        cout << "[Info]Add successfully.\n";
    }
    
    void startCompetition(int duration, int problems) {
        if (started) {
            cout << "[Error]Start failed: competition has started.\n";
            return;
        }
        
        duration_time = duration;
        problem_count = problems;
        started = true;
        
        // Initialize ranking by team name (lexicographic order)
        updateRanking();
        
        cout << "[Info]Competition starts.\n";
    }
    
    void submit(const string& problem, const string& team_name, Status status, int time) {
        Team& team = teams[team_name];
        Submission sub{problem, status, time};
        team.submissions.push_back(sub);
        
        ProblemInfo& info = team.problems[problem];
        
        if (info.solved) {
            // Already solved, just record the submission
            return;
        }
        
        if (frozen) {
            // During freeze - store submissions for later processing
            info.frozen = true;
            info.frozen_subs.push_back(sub);
            info.freeze_submissions++;
        } else {
            // Not frozen
            if (status == Accepted) {
                info.solved = true;
                info.accept_time = time;
                info.wrong_before_accept = info.wrong_attempts;
                team.updateSolvedAndPenalty();
                invalidateRanking();
            } else {
                info.wrong_attempts++;
            }
        }
    }
    
    void flush() {
        // Update all teams' frozen status
        for (auto& t : teams) {
            t.second.frozen = false;
            for (auto& p : t.second.problems) {
                if (p.second.frozen) {
                    t.second.frozen = true;
                    break;
                }
            }
        }
        
        invalidateRanking();
        updateRanking();
        first_flush = false;
        cout << "[Info]Flush scoreboard.\n";
    }
    
    void freezeScoreboard() {
        if (frozen) {
            cout << "[Error]Freeze failed: scoreboard has been frozen.\n";
            return;
        }
        
        frozen = true;
        
        // Record wrong attempts before freeze
        for (auto& t : teams) {
            for (auto& p : t.second.problems) {
                p.second.wrong_before_freeze = p.second.wrong_attempts;
            }
        }
        
        cout << "[Info]Freeze scoreboard.\n";
    }
    
    void scrollScoreboard() {
        if (!frozen) {
            cout << "[Error]Scroll failed: scoreboard has not been frozen.\n";
            return;
        }
        
        cout << "[Info]Scroll scoreboard.\n";
        
        // Print scoreboard before scrolling (after flush)
        printScoreboard();
        
        // Unfreeze problems one by one
        vector<tuple<string, string, int, int>> ranking_changes;
        
        while (true) {
            // Find lowest-ranked team with frozen problems
            int lowest_rank = -1;
            string lowest_team;
            string lowest_problem;
            
            for (int i = team_ranking.size() - 1; i >= 0; i--) {
                const string& team_name = team_ranking[i];
                Team& team = teams[team_name];
                
                // Find frozen problem with smallest problem ID
                string min_frozen_problem = "";
                for (int j = 0; j < problem_count; j++) {
                    string prob = string(1, 'A' + j);
                    if (team.problems[prob].frozen) {
                        min_frozen_problem = prob;
                        break;
                    }
                }
                
                if (!min_frozen_problem.empty()) {
                    lowest_rank = i;
                    lowest_team = team_name;
                    lowest_problem = min_frozen_problem;
                    break;
                }
            }
            
            if (lowest_rank == -1) {
                // No more frozen problems
                break;
            }
            
            // Unfreeze this problem
            Team& team = teams[lowest_team];
            ProblemInfo& info = team.problems[lowest_problem];
            
            // Process frozen submissions that were stored
            bool solved_during_freeze = false;
            int wrong_attempts_during_freeze = 0;
            
            // Process the frozen submissions in order
            for (const Submission& sub : info.frozen_subs) {
                if (solved_during_freeze) continue; // Already solved
                
                if (sub.status == Accepted) {
                    solved_during_freeze = true;
                    info.solved = true;
                    info.accept_time = sub.time;
                    info.wrong_before_accept = info.wrong_before_freeze + wrong_attempts_during_freeze;
                } else {
                    wrong_attempts_during_freeze++;
                }
            }
            
            if (!solved_during_freeze) {
                // Not solved, just update wrong attempts
                info.wrong_attempts = info.wrong_before_freeze + wrong_attempts_during_freeze;
            }
            
            info.frozen = false;
            info.freeze_submissions = 0;
            info.frozen_subs.clear();
            team.updateSolvedAndPenalty();
            
            // Check if ranking changed
            int old_rank = lowest_rank;
            string old_team_at_old_rank = team_ranking[old_rank];
            
            invalidateRanking();
            updateRanking();
            int new_rank_idx = -1;
            for (int i = 0; i < team_ranking.size(); i++) {
                if (team_ranking[i] == lowest_team) {
                    new_rank_idx = i;
                    break;
                }
            }
            
            if (new_rank_idx < old_rank) {
                // Ranking improved - report the team that was displaced
                // The displaced team is the one now immediately after the moving team
                string displaced_team = team_ranking[new_rank_idx + 1];
                ranking_changes.push_back({lowest_team, displaced_team, 
                    team.solved_count, team.total_penalty});
            }
        }
        
        // Print ranking changes
        for (const auto& change : ranking_changes) {
            cout << get<0>(change) << " " << get<1>(change) << " " 
                 << get<2>(change) << " " << get<3>(change) << "\n";
        }
        
        // Print scoreboard after scrolling
        printScoreboard();
        
        frozen = false;
    }
    
    void queryRanking(const string& team_name) {
        if (teams.find(team_name) == teams.end()) {
            cout << "[Error]Query ranking failed: cannot find the team.\n";
            return;
        }
        
        cout << "[Info]Complete query ranking.\n";
        
        if (frozen) {
            cout << "[Warning]Scoreboard is frozen. The ranking may be inaccurate until it were scrolled.\n";
        }
        
        int rank = -1;
        for (int i = 0; i < team_ranking.size(); i++) {
            if (team_ranking[i] == team_name) {
                rank = i + 1;
                break;
            }
        }
        
        cout << team_name << " NOW AT RANKING " << rank << "\n";
    }
    
    void querySubmission(const string& team_name, const string& problem, const string& status_str) {
        if (teams.find(team_name) == teams.end()) {
            cout << "[Error]Query submission failed: cannot find the team.\n";
            return;
        }
        
        cout << "[Info]Complete query submission.\n";
        
        const Team& team = teams[team_name];
        const Submission* last_match = nullptr;
        
        for (const Submission& sub : team.submissions) {
            bool match = true;
            
            if (problem != "ALL" && sub.problem != problem) {
                match = false;
            }
            
            if (status_str != "ALL" && statusToString(sub.status) != status_str) {
                match = false;
            }
            
            if (match) {
                last_match = &sub;
            }
        }
        
        if (last_match) {
            cout << team_name << " " << last_match->problem << " " 
                 << statusToString(last_match->status) << " " << last_match->time << "\n";
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
            string team_name;
            iss >> team_name;
            system.addTeam(team_name);
        }
        else if (command == "START") {
            string tmp;
            int duration, problem_count;
            iss >> tmp >> duration >> tmp >> problem_count;
            system.startCompetition(duration, problem_count);
        }
        else if (command == "SUBMIT") {
            string problem, tmp, team_name, status_str;
            int time;
            iss >> problem >> tmp >> team_name >> tmp >> status_str >> tmp >> time;
            Status status;
            if (status_str == "Accepted") status = Accepted;
            else if (status_str == "Wrong_Answer") status = Wrong_Answer;
            else if (status_str == "Runtime_Error") status = Runtime_Error;
            else status = Time_Limit_Exceed;
            system.submit(problem, team_name, status, time);
        }
        else if (command == "FLUSH") {
            system.flush();
        }
        else if (command == "FREEZE") {
            system.freezeScoreboard();
        }
        else if (command == "SCROLL") {
            system.scrollScoreboard();
        }
        else if (command == "QUERY_RANKING") {
            string team_name;
            iss >> team_name;
            system.queryRanking(team_name);
        }
        else if (command == "QUERY_SUBMISSION") {
            string team_name, tmp, problem_eq, status_eq;
            iss >> team_name >> tmp >> problem_eq >> tmp >> status_eq;
            
            // Parse PROBLEM=problem_name
            string problem = problem_eq.substr(7); // Skip "PROBLEM="
            
            // Parse STATUS=status
            string status_str = status_eq.substr(7); // Skip "STATUS="
            
            system.querySubmission(team_name, problem, status_str);
        }
        else if (command == "END") {
            system.end();
            break;
        }
    }
    
    return 0;
}
