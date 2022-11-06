#pragma once

#include "TreeStructure.hpp"

using namespace UniEngine;
namespace EcoSysLab {
    enum class BudType {
        Apical,
        LateralVegetative,
        LateralReproductive
    };

    enum class BudStatus {
        Dormant,
        Flushed,
        Died
    };

    class Bud {
    public:
        BudType m_type = BudType::Apical;
        BudStatus m_status = BudStatus::Dormant;
        glm::quat m_localRotation = glm::vec3(0.0f);
    };
    struct InternodeGrowthData {
        int m_age = 0;
        float m_inhibitor = 0;
        glm::quat m_desiredLocalRotation = glm::vec3(0.0f);
        float m_sagging = 0;

        float m_maxDistanceToAnyBranchEnd = 0;
        float m_level = 0;
        int m_order = 0;
        float m_childTotalBiomass = 0;

        float m_rootDistance = 0;

        float m_apicalControl = 0.0f;
        int m_decedentsAmount = 0;
        glm::vec3 m_lightDirection = glm::vec3(0, 1, 0);
        float m_lightIntensity = 1.0f;

        /**
         * List of buds, first one will always be the apical bud which points forward.
         */
        std::vector<Bud> m_buds;
        void Clear();
    };

    struct BranchGrowthData {
        int m_order = 0;
    };

    class TreeStructuralGrowthParameters : public ITreeGrowthParameters<InternodeGrowthData>{
    public:
        int m_lateralBudCount;
        /**
        * The mean and variance of the angle between the direction of a lateral bud and its parent shoot.
        */
        glm::vec2 m_branchingAngleMeanVariance{};
        /**
        * The mean and variance of an angular difference orientation of lateral buds between two internodes
        */
        glm::vec2 m_rollAngleMeanVariance{};
        /**
        * The mean and variance of the angular difference between the growth direction and the direction of the apical bud
        */
        glm::vec2 m_apicalAngleMeanVariance{};

        float m_gravitropism;
        float m_phototropism;
        float m_internodeLength;
        float m_growthRate;
        glm::vec2 m_endNodeThicknessAndControl{};
        float m_lateralBudFlushingProbability;
        /*
         * To form significant trunk. Larger than 1 means forming big trunk.
         */
        glm::vec2 m_apicalControlBaseDistFactor{};

        /**
        * How much inhibitor will an internode generate.
        */
        glm::vec3 m_apicalDominanceBaseAgeDist{};

        glm::vec2 m_budKillProbabilityApicalLateral{};
        /**
        * The limit of lateral branches being cut off when too close to the
        * root.
        */
        float m_lowBranchPruning;
        /**
         * The strength of gravity bending.
         */
        glm::vec3 m_saggingFactorThicknessReductionMax = glm::vec3(0.8f, 1.75f, 1.0f);


        [[nodiscard]] int GetLateralBudCount(const Internode<InternodeGrowthData> &internode) const override;

        [[nodiscard]] float GetDesiredBranchingAngle(const Internode<InternodeGrowthData> &internode) const override;

        [[nodiscard]] float GetDesiredRollAngle(const Internode<InternodeGrowthData> &internode) const override;

        [[nodiscard]] float GetDesiredApicalAngle(const Internode<InternodeGrowthData> &internode) const override;

        [[nodiscard]] float GetGravitropism(const Internode<InternodeGrowthData> &internode) const override;

        [[nodiscard]] float GetPhototropism(const Internode<InternodeGrowthData> &internode) const override;

        [[nodiscard]] float GetInternodeLength(const Internode<InternodeGrowthData> &internode) const override;

        [[nodiscard]] float GetGrowthRate(const Internode<InternodeGrowthData> &internode) const override;

        [[nodiscard]] float GetEndNodeThickness(const Internode<InternodeGrowthData> &internode) const override;

        [[nodiscard]] float GetThicknessControlFactor(const Internode<InternodeGrowthData> &internode) const override;

        [[nodiscard]] float GetLateralBudFlushingProbability(const Internode<InternodeGrowthData> &internode) const override;

        [[nodiscard]] float GetApicalControlBase(const Internode<InternodeGrowthData> &internode) const override;

        [[nodiscard]] float GetApicalDominanceBase(const Internode<InternodeGrowthData> &internode) const override;
        [[nodiscard]] float GetApicalDominanceDecrease(const Internode<InternodeGrowthData> &internode) const override;

        [[nodiscard]] float GetApicalBudKillProbability(const Internode<InternodeGrowthData> &internode) const override;

        [[nodiscard]] float GetLateralBudKillProbability(const Internode<InternodeGrowthData> &internode) const override;

        [[nodiscard]] bool GetPruning(const Internode<InternodeGrowthData> &internode) const override;
        [[nodiscard]] float GetLowBranchPruning(const Internode<InternodeGrowthData> &internode) const override;
        [[nodiscard]] float GetSagging(const Internode<InternodeGrowthData> &internode) const override;

        TreeStructuralGrowthParameters();
    };

    struct GrowthNutrients{
        float m_water = 0.0f;
    };

    class TreeModel {
        bool m_initialized = false;
        inline void LowBranchPruning(float maxDistance, InternodeHandle internodeHandle, const TreeStructuralGrowthParameters& parameters);
        inline void CalculateSagging(InternodeHandle internodeHandle, const TreeStructuralGrowthParameters& parameters);
        inline void CollectInhibitor(InternodeHandle internodeHandle, const TreeStructuralGrowthParameters& parameters);
        inline void GrowInternode(InternodeHandle internodeHandle, const TreeStructuralGrowthParameters& parameters, const GrowthNutrients& growthNutrients);

    public:
        glm::mat4 m_globalTransform = glm::translate(glm::vec3(0.0f)) * glm::mat4_cast(glm::quat(glm::vec3(0.0f))) * glm::scale(glm::vec3(1.0f));
        glm::vec3 m_gravityDirection = glm::vec3(0, -1, 0);
        TreeStructure<BranchGrowthData, InternodeGrowthData> m_treeStructure = {};
        [[nodiscard]] bool IsInitialized() const;
        void Initialize(const TreeStructuralGrowthParameters& parameters);
        void Clear();
        void Grow(const GrowthNutrients& growthNutrients, const TreeStructuralGrowthParameters& parameters);
    };

    class TreeModelGroup{
    public:
        std::vector<TreeModel> m_treeModels;
    };
}