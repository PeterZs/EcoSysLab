//
// Created by lllll on 10/24/2022.
//

#include "Tree.hpp"
#include "Graphics.hpp"
#include "EditorLayer.hpp"
#include "Application.hpp"

using namespace EcoSysLab;

void Tree::OnInspect() {
    static bool debugVisualization = true;
    static int version = -1;
    static std::vector<InternodeHandle> sortedInternodeList;
    static std::vector<BranchHandle> sortedBranchList;
    if (Editor::DragAndDropButton<TreeDescriptor>(m_treeDescriptor, "TreeDescriptor", true)) {
        m_treeModel.Clear();
        sortedInternodeList.clear();
        sortedBranchList.clear();
        version = -1;
    }

    if (m_treeDescriptor.Get<TreeDescriptor>()) {
        auto &parameters = m_treeDescriptor.Get<TreeDescriptor>()->m_treeStructuralGrowthParameters;
        if (!m_treeModel.IsInitialized()) m_treeModel.Initialize(parameters);
        if (ImGui::Button("Grow")) {
            m_treeModel.Grow({999}, parameters);
        }
        ImGui::Checkbox("Visualization", &debugVisualization);
        if (debugVisualization) {

            static std::vector<glm::vec4> randomColors;
            if (randomColors.empty()) {
                for (int i = 0; i < 100; i++) {
                    randomColors.emplace_back(glm::ballRand(1.0f), 1.0f);
                }
            }
            ImGui::Text("Internode count: %d", sortedInternodeList.size());
            ImGui::Text("Branch count: %d", sortedBranchList.size());
            static std::vector<glm::mat4> matrices;
            static std::vector<glm::vec4> colors;
            static Handle handle;
            bool needUpdate = handle == GetHandle();
            if (ImGui::Button("Update")) needUpdate = true;
            if (needUpdate || m_treeModel.m_tree->GetVersion() != version) {
                version = m_treeModel.m_tree->GetVersion();
                needUpdate = true;
                sortedBranchList = m_treeModel.m_tree->GetSortedBranchList();
                sortedInternodeList = m_treeModel.m_tree->GetSortedInternodeList();
            }
            if (needUpdate) {
                matrices.resize(sortedInternodeList.size());
                colors.resize(sortedInternodeList.size());
                std::vector<std::shared_future<void>> results;
                auto globalTransform = GetScene()->GetDataComponent<GlobalTransform>(GetOwner());
                Jobs::ParallelFor(sortedInternodeList.size(), [&](unsigned i) {
                    auto &internode = m_treeModel.m_tree->RefInternode(sortedInternodeList[i]);
                    auto rotation = globalTransform.GetRotation() * internode.m_globalRotation;
                    glm::vec3 translation = (globalTransform.m_value * glm::translate(internode.m_globalPosition))[3];
                    const auto direction = glm::normalize(rotation * glm::vec3(0, 0, -1));
                    const glm::vec3 position2 =
                            translation + internode.m_length * direction;
                    rotation = glm::quatLookAt(
                            direction, glm::vec3(direction.y, direction.z, direction.x));
                    rotation *= glm::quat(glm::vec3(glm::radians(90.0f), 0.0f, 0.0f));
                    const glm::mat4 rotationTransform = glm::mat4_cast(rotation);
                    matrices[i] =
                            glm::translate((translation + position2) / 2.0f) *
                            rotationTransform *
                            glm::scale(glm::vec3(
                                    internode.m_thickness,
                                    glm::distance(translation, position2) / 2.0f,
                                    internode.m_thickness));
                    colors[i] = randomColors[m_treeModel.m_tree->RefBranch(internode.m_branchHandle).m_data.m_order];
                }, results);
                for (auto &i: results) i.wait();
            }

            if (!matrices.empty()) {
                auto editorLayer = Application::GetLayer<EditorLayer>();
                Gizmos::DrawGizmoMeshInstancedColored(
                        DefaultResources::Primitives::Cylinder, editorLayer->m_sceneCamera,
                        editorLayer->m_sceneCameraPosition,
                        editorLayer->m_sceneCameraRotation,
                        *reinterpret_cast<std::vector<glm::vec4> *>(&colors),
                        *reinterpret_cast<std::vector<glm::mat4> *>(&matrices),
                        glm::mat4(1.0f), 1.0f);
            }
        }
    }

    if (ImGui::Button("Clear")) {
        m_treeModel.Clear();
        sortedInternodeList.clear();
        sortedBranchList.clear();
        version = -1;
    }
}

void Tree::OnCreate() {
}

void Tree::OnDestroy() {
    m_treeModel.Clear();
}

void TreeDescriptor::OnCreate() {

}

void TreeDescriptor::OnInspect() {
    if (ImGui::TreeNodeEx("Structure", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::DragInt("Lateral bud per node", &m_treeStructuralGrowthParameters.m_lateralBudCount);
        ImGui::DragFloat2("Branching Angle mean/var", &m_treeStructuralGrowthParameters.m_branchingAngleMeanVariance.x,
                          0.01f);
        ImGui::DragFloat2("Roll Angle mean/var", &m_treeStructuralGrowthParameters.m_rollAngleMeanVariance.x, 0.01f);
        ImGui::DragFloat2("Apical Angle mean/var", &m_treeStructuralGrowthParameters.m_apicalAngleMeanVariance.x,
                          0.01f);
        ImGui::DragFloat("Gravitropism", &m_treeStructuralGrowthParameters.m_gravitropism, 0.01f);
        ImGui::DragFloat("Phototropism", &m_treeStructuralGrowthParameters.m_phototropism, 0.01f);
        ImGui::DragFloat("Internode length", &m_treeStructuralGrowthParameters.m_internodeLength, 0.01f);
        ImGui::DragFloat("Growth rate", &m_treeStructuralGrowthParameters.m_growthRate, 0.01f);
        ImGui::DragFloat2("Thickness min/factor", &m_treeStructuralGrowthParameters.m_endNodeThicknessAndControl.x,
                          0.01f);
        ImGui::TreePop();
    }
    if (ImGui::TreeNodeEx("Bud", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::DragFloat("Lateral bud flushing probability",
                         &m_treeStructuralGrowthParameters.m_lateralBudFlushingProbability, 0.01f);
        ImGui::DragFloat2("Apical control base/dist", &m_treeStructuralGrowthParameters.m_apicalControlBaseDistFactor.x,
                          0.01f);
        ImGui::DragFloat3("Apical dominance base/age/dist",
                          &m_treeStructuralGrowthParameters.m_apicalDominanceBaseAgeDist.x, 0.01f);
        int maxAgeBeforeInhibitorEnds = m_treeStructuralGrowthParameters.m_apicalDominanceBaseAgeDist.x /
                                        m_treeStructuralGrowthParameters.m_apicalDominanceBaseAgeDist.y;
        float maxDistance = m_treeStructuralGrowthParameters.m_apicalDominanceBaseAgeDist.x /
                            m_treeStructuralGrowthParameters.m_apicalDominanceBaseAgeDist.z;
        ImGui::Text("Max age / distance: [%i, %.3f]", maxAgeBeforeInhibitorEnds, maxDistance);

        ImGui::DragFloat("Lateral bud lighting factor",
                         &m_treeStructuralGrowthParameters.m_lateralBudFlushingLightingFactor, 0.01f);
        ImGui::DragFloat2("Kill probability apical/lateral",
                          &m_treeStructuralGrowthParameters.m_budKillProbabilityApicalLateral.x, 0.01f);
        ImGui::TreePop();
    }
    if (ImGui::TreeNodeEx("Internode")) {
        ImGui::DragInt("Random pruning Order Protection",
                       &m_treeStructuralGrowthParameters.m_randomPruningOrderProtection);
        ImGui::DragFloat3("Random pruning base/age/max", &m_treeStructuralGrowthParameters.m_randomPruningBaseAgeMax.x,
                          0.0001f, -1.0f, 1.0f, "%.5f");
        const float maxAgeBeforeMaxCutOff =
                (m_treeStructuralGrowthParameters.m_randomPruningBaseAgeMax.z -
                 m_treeStructuralGrowthParameters.m_randomPruningBaseAgeMax.x) /
                m_treeStructuralGrowthParameters.m_randomPruningBaseAgeMax.y;
        ImGui::Text("Max age before reaching max: %.3f", maxAgeBeforeMaxCutOff);
        ImGui::DragFloat("Low Branch Pruning", &m_treeStructuralGrowthParameters.m_lowBranchPruning, 0.01f);
        ImGui::DragFloat3("Sagging thickness/reduction/max",
                          &m_treeStructuralGrowthParameters.m_saggingFactorThicknessReductionMax.x, 0.01f);
        ImGui::TreePop();
    }
}

void TreeDescriptor::CollectAssetRef(std::vector<AssetRef> &list) {

}

void TreeDescriptor::Serialize(YAML::Emitter &out) {
    out << YAML::Key << "m_lateralBudCount" << YAML::Value << m_treeStructuralGrowthParameters.m_lateralBudCount;
    out << YAML::Key << "m_branchingAngleMeanVariance" << YAML::Value
        << m_treeStructuralGrowthParameters.m_branchingAngleMeanVariance;
    out << YAML::Key << "m_rollAngleMeanVariance" << YAML::Value
        << m_treeStructuralGrowthParameters.m_rollAngleMeanVariance;
    out << YAML::Key << "m_apicalAngleMeanVariance" << YAML::Value
        << m_treeStructuralGrowthParameters.m_apicalAngleMeanVariance;
    out << YAML::Key << "m_gravitropism" << YAML::Value << m_treeStructuralGrowthParameters.m_gravitropism;
    out << YAML::Key << "m_phototropism" << YAML::Value << m_treeStructuralGrowthParameters.m_phototropism;
    out << YAML::Key << "m_internodeLength" << YAML::Value << m_treeStructuralGrowthParameters.m_internodeLength;
    out << YAML::Key << "m_growthRate" << YAML::Value << m_treeStructuralGrowthParameters.m_growthRate;
    out << YAML::Key << "m_endNodeThicknessAndControl" << YAML::Value
        << m_treeStructuralGrowthParameters.m_endNodeThicknessAndControl;
    out << YAML::Key << "m_lateralBudFlushingProbability" << YAML::Value
        << m_treeStructuralGrowthParameters.m_lateralBudFlushingProbability;
    out << YAML::Key << "m_apicalControlBaseDistFactor" << YAML::Value
        << m_treeStructuralGrowthParameters.m_apicalControlBaseDistFactor;
    out << YAML::Key << "m_apicalDominanceBaseAgeDist" << YAML::Value
        << m_treeStructuralGrowthParameters.m_apicalDominanceBaseAgeDist;
    out << YAML::Key << "m_lateralBudFlushingLightingFactor" << YAML::Value
        << m_treeStructuralGrowthParameters.m_lateralBudFlushingLightingFactor;
    out << YAML::Key << "m_budKillProbabilityApicalLateral" << YAML::Value
        << m_treeStructuralGrowthParameters.m_budKillProbabilityApicalLateral;
    out << YAML::Key << "m_randomPruningOrderProtection" << YAML::Value
        << m_treeStructuralGrowthParameters.m_randomPruningOrderProtection;
    out << YAML::Key << "m_randomPruningBaseAgeMax" << YAML::Value
        << m_treeStructuralGrowthParameters.m_randomPruningBaseAgeMax;
    out << YAML::Key << "m_lowBranchPruning" << YAML::Value << m_treeStructuralGrowthParameters.m_lowBranchPruning;
    out << YAML::Key << "m_saggingFactorThicknessReductionMax" << YAML::Value
        << m_treeStructuralGrowthParameters.m_saggingFactorThicknessReductionMax;
}

void TreeDescriptor::Deserialize(const YAML::Node &in) {
    if (in["m_lateralBudCount"]) m_treeStructuralGrowthParameters.m_lateralBudCount = in["m_lateralBudCount"].as<int>();
    if (in["m_branchingAngleMeanVariance"]) m_treeStructuralGrowthParameters.m_branchingAngleMeanVariance = in["m_branchingAngleMeanVariance"].as<glm::vec2>();
    if (in["m_rollAngleMeanVariance"]) m_treeStructuralGrowthParameters.m_rollAngleMeanVariance = in["m_rollAngleMeanVariance"].as<glm::vec2>();
    if (in["m_apicalAngleMeanVariance"]) m_treeStructuralGrowthParameters.m_apicalAngleMeanVariance = in["m_apicalAngleMeanVariance"].as<glm::vec2>();
    if (in["m_gravitropism"]) m_treeStructuralGrowthParameters.m_gravitropism = in["m_gravitropism"].as<float>();
    if (in["m_phototropism"]) m_treeStructuralGrowthParameters.m_phototropism = in["m_phototropism"].as<float>();
    if (in["m_internodeLength"]) m_treeStructuralGrowthParameters.m_internodeLength = in["m_internodeLength"].as<float>();
    if (in["m_growthRate"]) m_treeStructuralGrowthParameters.m_growthRate = in["m_growthRate"].as<float>();
    if (in["m_endNodeThicknessAndControl"]) m_treeStructuralGrowthParameters.m_endNodeThicknessAndControl = in["m_endNodeThicknessAndControl"].as<glm::vec2>();
    if (in["m_lateralBudFlushingProbability"]) m_treeStructuralGrowthParameters.m_lateralBudFlushingProbability = in["m_lateralBudFlushingProbability"].as<float>();
    if (in["m_apicalControlBaseDistFactor"]) m_treeStructuralGrowthParameters.m_apicalControlBaseDistFactor = in["m_apicalControlBaseDistFactor"].as<glm::vec2>();
    if (in["m_apicalDominanceBaseAgeDist"]) m_treeStructuralGrowthParameters.m_apicalDominanceBaseAgeDist = in["m_apicalDominanceBaseAgeDist"].as<glm::vec3>();
    if (in["m_lateralBudFlushingLightingFactor"]) m_treeStructuralGrowthParameters.m_lateralBudFlushingLightingFactor = in["m_lateralBudFlushingLightingFactor"].as<float>();
    if (in["m_budKillProbabilityApicalLateral"]) m_treeStructuralGrowthParameters.m_budKillProbabilityApicalLateral = in["m_budKillProbabilityApicalLateral"].as<glm::vec2>();
    if (in["m_randomPruningOrderProtection"]) m_treeStructuralGrowthParameters.m_randomPruningOrderProtection = in["m_randomPruningOrderProtection"].as<int>();
    if (in["m_randomPruningBaseAgeMax"]) m_treeStructuralGrowthParameters.m_randomPruningBaseAgeMax = in["m_randomPruningBaseAgeMax"].as<glm::vec3>();
    if (in["m_lowBranchPruning"]) m_treeStructuralGrowthParameters.m_lowBranchPruning = in["m_lowBranchPruning"].as<float>();
    if (in["m_saggingFactorThicknessReductionMax"]) m_treeStructuralGrowthParameters.m_saggingFactorThicknessReductionMax = in["m_saggingFactorThicknessReductionMax"].as<glm::vec3>();
}
