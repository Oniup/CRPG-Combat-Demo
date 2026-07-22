#pragma once

#include "engine/core/Error.h"
#include "engine/utilities/TypeInfo.h"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace eng::goap {

struct NodeCompare;

struct Blackboard
{
    using State        = std::variant<int, unsigned, float, bool, std::string>;
    using UnorderedMap = std::unordered_map<unsigned, State, StringHash, std::equal_to<>>;

    UnorderedMap worldState;

    Blackboard(UnorderedMap&& map);
    Blackboard(const Blackboard& other);
    Blackboard(Blackboard&& other);

    Blackboard& operator=(Blackboard&& other);
    Blackboard& operator=(const Blackboard& other);

    bool operator==(const Blackboard& other) const { return worldState == other.worldState; }

    template <typename T>
    const T& GetState(std::string_view name) const
    {
        const auto iter = worldState.find(name);
        ASSERT(iter != worldState.end() && std::holds_alternative<T>(iter->second));

        return std::get<T>(iter->second);
    }

    template <typename T>
    T& GetState(std::string_view name)
    {
        auto iter = worldState.find(name);
        ASSERT(iter != worldState.end() && std::holds_alternative<T>(iter->second));

        return std::get<T>(iter->second);
    }
};

class Goal
{
public:
    virtual ~Goal() = default;

    std::string GetName() const { return m_Name; }

    virtual bool RequiredState(Blackboard& state) = 0;
    virtual bool IsSatisfied(Blackboard& state)   = 0;

protected:
    Goal(const std::string& name)
        : m_Name(name)
    {
    }

private:
    std::string m_Name;
};

class Action
{
public:
    virtual ~Action() = default;

    std::string GetName() const { return m_Name; }

    virtual bool CheckPreconditions(Blackboard& state) = 0;
    virtual bool ApplyEffect(Blackboard& state)        = 0;
    virtual bool Execute(Blackboard& state)            = 0;

protected:
    Action(const std::string& name)
        : m_Name(name)
    {
    }

private:
    std::string m_Name;
};

class Planner
{
    friend NodeCompare;

    struct Node
    {
        Blackboard state;
        Node* parent;
        Action* action;
        float gCost;
    };

public:
    Planner()  = default;
    ~Planner() = default;

    bool Plan(Blackboard& currentState, Goal* goal);
    void PerformPlanAction(float deltaTime);
    bool ShouldIncrementToNextPlanAction();
    bool NextPlanAction();

    void AddGoal(std::string_view name, std::unique_ptr<Goal>&& goal);
    void AddAction(std::string_view name, std::unique_ptr<Action>&& action);

private:
    bool ActionProvidesEffect(Action* action, const Blackboard& currentState);
    int CalculateHeuristicScore(const Blackboard& currentState, Goal* goal);
    Blackboard RegressState(const Blackboard& currentState, Action* action);
    void ReconstructPlan(Node* finalNode);

    std::vector<std::unique_ptr<Goal>> m_Goals;
    std::vector<std::unique_ptr<Action>> m_Actions;

    std::vector<size_t> m_Plan;
    unsigned m_CurrentPlanIndex = 0;
};

} // namespace eng::goap
