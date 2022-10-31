//
// Created by lllll on 10/25/2022.
//

#include "Trees.hpp"
#include "Graphics.hpp"
#include "EditorLayer.hpp"
#include "Application.hpp"

using namespace EcoSysLab;

void Trees::OnInspect() {
    static bool debugVisualization = true;
    static std::vector<int> versions;
    static std::vector<glm::vec4> randomColors;
    if (randomColors.empty()) {
        for (int i = 0; i < 100; i++) {
            randomColors.emplace_back(glm::ballRand(1.0f), 1.0f);
        }
    }
    static glm::ivec2 gridSize = {32, 32};
    static glm::vec2 gridDistance = {10, 10};
    static float totalTime = 0.0f;
    static int internodeSize = 0;
    static int branchSize = 0;
    static int iteration = 0;
    if (Editor::DragAndDropButton<TreeDescriptor>(m_treeDescriptor, "TreeDescriptor", true)) {
        internodeSize = 0;
        branchSize = 0;
        iteration = 0;
        m_trees.clear();
        totalTime = 0.0f;
        versions.clear();
    }
    if (m_treeDescriptor.Get<TreeDescriptor>()) {
        auto &parameters = m_treeDescriptor.Get<TreeDescriptor>()->m_treeStructuralGrowthParameters;
        if (m_trees.empty()) {
            ImGui::DragInt2("Grid size", &gridSize.x, 1, 0, 100);
            ImGui::DragFloat2("Grid distance", &gridDistance.x, 0.1f, 0.0f, 100.0f);
            if (ImGui::Button("Create trees")) {
                internodeSize = 0;
                branchSize = 0;
                iteration = 0;
                m_trees.clear();
                totalTime = 0.0f;
                versions.clear();
                for (int i = 0; i < gridSize.x; i++) {
                    for (int j = 0; j < gridSize.y; j++) {
                        m_trees.emplace_back();
                        versions.emplace_back(-1);
                        auto &tree = m_trees.back();
                        tree.m_treeModel.Initialize(parameters);
                        tree.m_transform = Transform();
                        tree.m_transform.SetPosition(glm::vec3(i * gridDistance.x, 0.0f, j * gridDistance.y));
                    }
                }
            }
        } else {
            static float lastUsedTime = 0.0f;
            if (ImGui::Button("Grow")) {
                iteration++;
                float time = Application::Time().CurrentTime();
                std::vector<std::shared_future<void>> results;
                Jobs::ParallelFor(m_trees.size(), [&](unsigned i) {
                    m_trees[i].m_treeModel.Grow({999}, parameters);
                }, results);
                for (auto &i: results) i.wait();
                lastUsedTime = Application::Time().CurrentTime() - time;
                totalTime += lastUsedTime;
            }
            ImGui::Text("Growth time: %.4f", lastUsedTime);
            ImGui::Text("Total time: %.4f", totalTime);
            ImGui::Text("Iteration: %d", iteration);
            ImGui::Checkbox("Visualization", &debugVisualization);


            ImGui::Text("Internode count: %d", internodeSize);
            ImGui::Text("Branch count: %d", branchSize);
            ImGui::Text("Tree count: %d", m_trees.size());
            if (debugVisualization && !m_trees.empty()) {
                static bool branchOnly = false;
                ImGui::Checkbox("Branch only", &branchOnly);
                static std::vector<glm::mat4> matrices;
                static std::vector<glm::vec4> colors;
                static std::vector<glm::mat4> branchMatrices;
                static std::vector<glm::vec4> branchColors;
                static Handle handle;
                bool needUpdate = handle == GetHandle();
                if (ImGui::Button("Update")) needUpdate = true;
                int totalInternodeSize = 0;
                int totalBranchSize = 0;
                for (int i = 0; i < m_trees.size(); i++) {
                    auto &tree = m_trees[i];
                    if (versions[i] != tree.m_treeModel.m_tree->Skeleton().GetVersion()) {
                        versions[i] = tree.m_treeModel.m_tree->Skeleton().GetVersion();
                        needUpdate = true;
                    }
                    totalInternodeSize += tree.m_treeModel.m_tree->Skeleton().RefSortedInternodeList().size();
                    totalBranchSize += tree.m_treeModel.m_tree->Skeleton().RefSortedBranchList().size();
                }
                internodeSize = totalInternodeSize;
                branchSize = totalBranchSize;
                if (needUpdate) {
                    matrices.resize(totalInternodeSize);
                    colors.resize(totalInternodeSize);
                    branchMatrices.resize(totalBranchSize);
                    branchColors.resize(totalBranchSize);

                    int startIndex = 0;
                    auto entityGlobalTransform = GetScene()->GetDataComponent<GlobalTransform>(GetOwner());

                    for (int listIndex = 0; listIndex < m_trees.size(); listIndex++) {
                        auto &tree = m_trees[listIndex];
                        const auto &list = tree.m_treeModel.m_tree->Skeleton().RefSortedInternodeList();
                        std::vector<std::shared_future<void>> results;
                        GlobalTransform globalTransform;
                        globalTransform.m_value =
                                entityGlobalTransform.m_value * m_trees[listIndex].m_transform.m_value;
                        Jobs::ParallelFor(list.size(), [&](unsigned i) {
                            auto &internode = m_trees[listIndex].m_treeModel.m_tree->Skeleton().RefInternode(list[i]);
                            auto rotation = globalTransform.GetRotation() * internode.m_info.m_globalRotation;
                            glm::vec3 translation = (globalTransform.m_value *
                                                     glm::translate(internode.m_info.m_globalPosition))[3];
                            const auto direction = glm::normalize(rotation * glm::vec3(0, 0, -1));
                            auto localEndPosition = internode.m_info.m_globalPosition + internode.m_info.m_length * direction;
                            const glm::vec3 position2 = (globalTransform.m_value * glm::translate(localEndPosition))[3];
                            rotation = glm::quatLookAt(
                                    direction, glm::vec3(direction.y, direction.z, direction.x));
                            rotation *= glm::quat(glm::vec3(glm::radians(90.0f), 0.0f, 0.0f));
                            const glm::mat4 rotationTransform = glm::mat4_cast(rotation);
                            matrices[i + startIndex] =
                                    glm::translate((translation + position2) / 2.0f) *
                                    rotationTransform *
                                    glm::scale(glm::vec3(
                                            internode.m_info.m_thickness,
                                            glm::distance(translation, position2) / 2.0f,
                                            internode.m_info.m_thickness));
                            colors[i + startIndex] = randomColors[m_trees[listIndex].m_treeModel.m_tree->Skeleton().RefBranch(
                                    internode.m_branchHandle).m_data.m_order];
                        }, results);
                        for (auto &i: results) i.wait();
                        startIndex += list.size();
                    }

                    startIndex = 0;
                    for (int listIndex = 0; listIndex < m_trees.size(); listIndex++) {
                        auto &tree = m_trees[listIndex];
                        const auto &list = tree.m_treeModel.m_tree->Skeleton().RefSortedBranchList();
                        std::vector<std::shared_future<void>> results;
                        GlobalTransform globalTransform;
                        globalTransform.m_value =
                                entityGlobalTransform.m_value * m_trees[listIndex].m_transform.m_value;
                        Jobs::ParallelFor(list.size(), [&](unsigned i) {
                            auto &branch = m_trees[listIndex].m_treeModel.m_tree->Skeleton().RefBranch(list[i]);
                            glm::vec3 translation = (globalTransform.m_value *
                                                     glm::translate(branch.m_info.m_globalStartPosition))[3];
                            const auto direction = glm::normalize(
                                    branch.m_info.m_globalEndPosition - branch.m_info.m_globalStartPosition);
                            auto length = glm::distance(branch.m_info.m_globalStartPosition, branch.m_info.m_globalEndPosition);
                            auto thickness = (branch.m_info.m_startThickness + branch.m_info.m_endThickness) * 0.5;
                            const glm::vec3 position2 = (globalTransform.m_value *
                                                         glm::translate(branch.m_info.m_globalEndPosition))[3];
                            auto rotation = glm::quatLookAt(
                                    direction, glm::vec3(direction.y, direction.z, direction.x));
                            rotation *= glm::quat(glm::vec3(glm::radians(90.0f), 0.0f, 0.0f));
                            const glm::mat4 rotationTransform = glm::mat4_cast(rotation);
                            branchMatrices[i + startIndex] =
                                    glm::translate((translation + position2) / 2.0f) *
                                    rotationTransform *
                                    glm::scale(glm::vec3(
                                            thickness,
                                            glm::distance(translation, position2) / 2.0f,
                                            thickness));
                            branchColors[i + startIndex] = randomColors[branch.m_data.m_order];
                        }, results);
                        for (auto &i: results) i.wait();
                        startIndex += list.size();
                    }

                }
                if (!matrices.empty()) {
                    auto editorLayer = Application::GetLayer<EditorLayer>();
                    if (branchOnly) {
                        Gizmos::DrawGizmoMeshInstancedColored(
                                DefaultResources::Primitives::Cube, editorLayer->m_sceneCamera,
                                editorLayer->m_sceneCameraPosition,
                                editorLayer->m_sceneCameraRotation,
                                *reinterpret_cast<std::vector<glm::vec4> *>(&branchColors),
                                *reinterpret_cast<std::vector<glm::mat4> *>(&branchMatrices),
                                glm::mat4(1.0f), 1.0f);
                    } else {
                        Gizmos::DrawGizmoMeshInstancedColored(
                                DefaultResources::Primitives::Cube, editorLayer->m_sceneCamera,
                                editorLayer->m_sceneCameraPosition,
                                editorLayer->m_sceneCameraRotation,
                                *reinterpret_cast<std::vector<glm::vec4> *>(&colors),
                                *reinterpret_cast<std::vector<glm::mat4> *>(&matrices),
                                glm::mat4(1.0f), 1.0f);
                    }
                }
            }
        }
    }
    if (ImGui::Button("Clear")) {
        internodeSize = 0;
        branchSize = 0;
        iteration = 0;
        m_trees.clear();
        totalTime = 0.0f;
        versions.clear();
    }
}

void Trees::OnCreate() {

}

void Trees::OnDestroy() {

}
