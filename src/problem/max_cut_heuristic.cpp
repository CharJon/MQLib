#include <cstdint>
#include <iostream>
#include <cmath>
#include <limits>
#include "mqlib/problem/max_cut_heuristic.h"
#include "mqlib/problem/max_cut_instance.h"

namespace mqlib {

    MaxCutHeuristic::MaxCutHeuristic(const MaxCutInstance &mi,
                                     double runtime_limit, bool validation,
                                     MaxCutCallback *mc,
                                     double solution_value_limit) :
            Heuristic(runtime_limit, validation, solution_value_limit),
            mi_(mi),
            mc_(mc) {
        // Store a new best solution with objective 0
        past_solutions_.emplace_back(mi, this, -1);
        past_solution_values_.push_back(0.0);
        past_solution_times_.push_back(Runtime());
    }

    bool MaxCutHeuristic::Report() {
        double rt = Runtime();
        if (mc_) {
            return mc_->Report(past_solutions_[past_solutions_.size() - 1], false, rt);
        } else {
            return rt < runtime_limit_;
        }
    }

    bool MaxCutHeuristic::Report(int iter) {
        double rt = Runtime();
        if (mc_) {
            return mc_->Report(past_solutions_[past_solutions_.size() - 1], false, rt,
                               iter);
        } else {
            return rt < runtime_limit_;
        }
    }

    bool MaxCutHeuristic::Report(const BaseSolution &solution) {
        double rt = Runtime();
        bool newBest = NewBest(solution, rt);
        if (mc_) {
            return mc_->Report(past_solutions_[past_solutions_.size() - 1], newBest, rt);
        } else {
            return (rt < runtime_limit_) && (best_ < solution_value_limit_);
        }
    }

    bool MaxCutHeuristic::Report(const BaseSolution &solution, int iter) {
        double rt = Runtime();
        bool newBest = NewBest(solution, rt);
        if (mc_) {
            return mc_->Report(past_solutions_[past_solutions_.size() - 1], newBest, rt,
                               iter);
        } else {
            return (rt < runtime_limit_) && (best_ < solution_value_limit_);
        }
    }

    bool MaxCutHeuristic::NewBest(const BaseSolution &solution, double runtime) {
        if (solution.ImprovesOver(best_)) {
            best_ = solution.get_weight();
            past_solution_times_.push_back(runtime);
            past_solution_values_.push_back(best_);
            if (validation_) {
                past_solutions_.emplace_back(mi_, this,
                                             solution.get_assignments(),
                                             solution.get_weight());
            } else {
                past_solutions_[0] = MaxCutSimpleSolution(mi_, this,
                                                          solution.get_assignments(),
                                                          solution.get_weight());
            }
            return true;
        }
        return false;
    }

    bool MaxCutHeuristic::IsHistoryValid() {
        if (!validation_) {
            return true;  // Not keeping track of solutions, so can't validate anything
        }
        if (past_solution_values_.size() != past_solutions_.size()) {
            std::cout << "Error: past solution information not correctly stored." <<
                      std::endl;
            exit(1);
        }

        // Go through stored solutions, recalculate their solution values, and
        // thus confirm our results are accurate
        for (uint64_t i = 0; i < past_solution_values_.size(); i++) {
            // Recalculate...
            past_solutions_[i].PopulateFromAssignments();
            double trueVal = past_solutions_[i].get_weight();
            double trueAbs = fabs(trueVal);
            double diff = fabs(trueVal - past_solution_values_[i]);
            if (diff != 0.0 && (trueAbs == 0 || diff / trueAbs >= 1e-8)) {
                // Significant error (based on relative difference)
                std::cout << std::endl << trueVal
                          << std::endl << past_solution_values_[i]
                          << std::endl << diff
                          << std::endl;
                return false;
            }
        }
        return true;
    }

}
