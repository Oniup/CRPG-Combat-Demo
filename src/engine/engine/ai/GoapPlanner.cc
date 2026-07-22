// #include "engine/ai/GoapPlanner.h"

// #include <queue>

// namespace eng::goap {

// struct NodeCompare
// {
//     bool operator()(const Planner::Node* a, const Planner::Node* b) const
//     {
//         return a->gCost > b->gCost;
//     }
// };

// Blackboard::Blackboard(UnorderedMap&& map)
//     : worldState(std::move(map))
// {
// }

// Blackboard::Blackboard(const Blackboard& other)
//     : worldState(other.worldState)
// {
// }

// Blackboard::Blackboard(Blackboard&& other)
//     : worldState(std::move(other.worldState))
// {
// }

// Blackboard& Blackboard::operator=(Blackboard&& other)
// {
//     worldState = std::move(other.worldState);
//     return *this;
// }

// Blackboard& Blackboard::operator=(const Blackboard& other)
// {
//     worldState = other.worldState;
//     return *this;
// }

// bool Planner::Plan(Blackboard& currentState, Goal* goal)
// {
//     std::vector<Node> nodePool;
//     std::priority_queue<Node*, std::vector<Node*>, NodeCompare> unexplored;
//     std::vector<Node*> explored;

//     nodePool.push_back(Node{
//         .state  = currentState,
//         .parent = nullptr,
//         .action = nullptr,
//         .gCost  = 0,
//     });
//     unexplored.emplace(&nodePool.back());

//     while (!unexplored.empty())
//     {
//         Node* current = unexplored.top();
//         unexplored.pop();
//         explored.push_back(current);

//         if (goal->IsSatisfied(current->state))
//         {
//             ReconstructPlan(current);
//             return true;
//         }

//         for (std::unique_ptr<Action>& action : m_Actions)
//         {
//             if (ActionProvidesEffect(action.get(), current->state))
//             {
//                 Blackboard regressedState = RegressState(current->state, action.get());
//             }
//         }
//     }

//     return false;
// }

// void Planner::ReconstructPlan(Node* finalNode)
// {
//     Node* current = finalNode;
//     std::vector<size_t> tempPlan;

//     // Traverse backwards from the goal to the start
//     while (current != nullptr && current->action != nullptr)
//     {
//         current = current->parent;
//     }

//     // The plan was built backwards, so reverse it for execution
//     for (auto it = tempPlan.rbegin(); it != tempPlan.rend(); ++it)
//     {
//         m_Plan.push_back(*it);
//     }
// }

// void Planner::PerformPlanAction(float deltaTime)
// {
//     if (m_Plan.empty() || m_CurrentPlanIndex >= m_Plan.size())
//         return;

//     unsigned currentActionHash = m_Plan[m_CurrentPlanIndex];
//     auto it                    = m_Actions.find(currentActionHash);

//     if (it != m_Actions.end())
//     {
//     }
// }

// bool Planner::ShouldIncrementToNextPlanAction()
// {
//     // Implementation depends on how you check if an action is finished.
//     // Often, Execute() returns true when done, or you poll a status.
//     return false;
// }

// bool Planner::NextPlanAction()
// {
//     m_CurrentPlanIndex++;
//     return m_CurrentPlanIndex < m_Plan.size();
// }

// void Planner::AddGoal(std::string_view name, std::unique_ptr<Goal>&& goal)
// {
//     m_Goals.emplace(std::string(name), std::move(goal));
// }

// void Planner::AddAction(std::string_view name, std::unique_ptr<Action>&& action)
// {
//     m_Actions.emplace(std::string(name), std::move(action));
// }

// bool Planner::ActionProvidesEffect(Action* action, const Blackboard& currentState)
// {
//     return false;
// }

// int Planner::CalculateHeuristicScore(const Blackboard& currentState, Goal* goal)
// {
//     return 0;
// }

// Blackboard Planner::RegressState(const Blackboard& currentState, Action* action)
// {
//     return Blackboard(currentState);
// }

// } // namespace eng::goap
