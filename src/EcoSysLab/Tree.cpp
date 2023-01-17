//
// Created by lllll on 10/24/2022.
//

#include "Tree.hpp"
#include "Graphics.hpp"
#include "EditorLayer.hpp"
#include "Application.hpp"
#include "TreeMeshGenerator.hpp"
#include "Soil.hpp"
#include "Climate.hpp"
#include "Octree.hpp"
#include "EcoSysLabLayer.hpp"
#include "HeightField.hpp"
using namespace EcoSysLab;

void Tree::OnInspect() {
	bool modelChanged = false;
	if (Editor::DragAndDropButton<TreeDescriptor>(m_treeDescriptor, "TreeDescriptor", true)) {
		m_treeModel.Clear();
		modelChanged = true;
	}
	if (m_treeDescriptor.Get<TreeDescriptor>()) {
		ImGui::Checkbox("Enable History", &m_enableHistory);
		ImGui::Checkbox("Receive light", &m_treeModel.m_collectLight);
		ImGui::Checkbox("Receive water", &m_treeModel.m_collectWater);
		ImGui::Checkbox("Receive nitrite", &m_treeModel.m_collectNitrite);
		ImGui::Checkbox("Enable Branch collision detection", &m_treeModel.m_enableBranchCollisionDetection);
		ImGui::Checkbox("Enable Root collision detection", &m_treeModel.m_enableRootCollisionDetection);
		if (ImGui::Button("Grow")) {
			TryGrow();
			modelChanged = true;
		}
		static int iterations = 5;
		ImGui::DragInt("Iterations", &iterations, 1, 1, 100);
		if (ImGui::TreeNode("Mesh generation")) {
			static TreeMeshGeneratorSettings meshGeneratorSettings;
			meshGeneratorSettings.OnInspect();
			if (ImGui::Button("Generate Mesh")) {
				GenerateMesh(meshGeneratorSettings);
			}
			ImGui::TreePop();
		}
		if (ImGui::Button("Clear")) {
			m_treeModel.Clear();
			modelChanged = true;
		}
	}

	if (ImGui::TreeNodeEx("Illumination Estimation settings"))
	{
		ImGui::DragFloat("Overall intensity", &m_treeModel.m_illuminationEstimationSettings.m_overallIntensity, 0.01f);
		ImGui::DragFloat("Occlusion", &m_treeModel.m_illuminationEstimationSettings.m_occlusion, 0.01f);
		ImGui::DragFloat("Occlusion distance Factor", &m_treeModel.m_illuminationEstimationSettings.m_occlusionDistanceFactor, 0.01f);
		ImGui::DragFloat("Min/max ratio", &m_treeModel.m_illuminationEstimationSettings.m_layerAngleFactor, 0.01f);
		ImGui::TreePop();
	}

	if (ImGui::Button("Calculate illumination"))
	{
		m_treeModel.CollectShootFlux(m_climate.Get<Climate>()->m_climateModel, m_treeDescriptor.Get<TreeDescriptor>()->m_treeGrowthParameters);
		modelChanged = true;
	}
	if (modelChanged) {
		const auto treeVisualizationLayer = Application::GetLayer<EcoSysLabLayer>();
		if (treeVisualizationLayer && treeVisualizationLayer->m_selectedTree == GetOwner()) {
			treeVisualizationLayer->m_treeVisualizer.Reset(m_treeModel);
		}
	}

	static bool visualizeVolume = false;
	ImGui::Checkbox("Visualize illumination volume", &visualizeVolume);
	if(visualizeVolume && m_treeModel.m_treeVolume.m_hasData)
	{
		std::vector<glm::mat4> matrices;

		for(int i = 0; i < m_treeModel.m_treeVolume.m_layerAmount; i++)
		{
			for (int j = 0; j < m_treeModel.m_treeVolume.m_sectorAmount; j++)
			{
				glm::vec3 tipPosition;
				m_treeModel.m_treeVolume.TipPosition(i, j, tipPosition);
				matrices.push_back(glm::translate(tipPosition) * glm::scale(glm::vec3(0.05f)));
			}
		}

		Gizmos::DrawGizmoMeshInstanced(DefaultResources::Primitives::Cube, glm::vec4(1, 0, 0, 1), matrices);
	}
}

void Tree::OnCreate() {
}

void Tree::OnDestroy() {
	m_treeModel.Clear();
	m_treeDescriptor.Clear();
	m_soil.Clear();
	m_climate.Clear();
	m_enableHistory = false;
}

bool Tree::TryGrow() {
	const auto scene = GetScene();
	const auto treeDescriptor = m_treeDescriptor.Get<TreeDescriptor>();
	if (!treeDescriptor) {
		UNIENGINE_ERROR("No tree descriptor!");
		return false;
	}
	if (!m_soil.Get<Soil>()) {
		UNIENGINE_ERROR("No soil model!");
		return false;
	}
	if (!m_climate.Get<Climate>()) {
		UNIENGINE_ERROR("No climate model!");
		return false;
	}
	const auto soil = m_soil.Get<Soil>();
	const auto climate = m_climate.Get<Climate>();
	if (m_enableHistory) m_treeModel.Step();

	const auto owner = GetOwner();
	return m_treeModel.Grow(scene->GetDataComponent<GlobalTransform>(owner).m_value, soil->m_soilModel, climate->m_climateModel,
		treeDescriptor->m_rootGrowthParameters, treeDescriptor->m_treeGrowthParameters);
}

void Tree::Serialize(YAML::Emitter& out)
{
	m_treeDescriptor.Save("m_treeDescriptor", out);
}

void Tree::Deserialize(const YAML::Node& in)
{
	m_treeDescriptor.Load("m_treeDescriptor", in);
}

void Tree::GenerateMesh(const TreeMeshGeneratorSettings& meshGeneratorSettings) {
	const auto scene = GetScene();
	const auto self = GetOwner();
	const auto children = scene->GetChildren(self);

	Entity branchEntity, rootEntity, foliageEntity, fruitEntity;

	for (const auto& child : children) {
		auto name = scene->GetEntityName(child);
		if (name == "Branch Mesh") {
			scene->DeleteEntity(child);
		}
		else if (name == "Root Mesh") {
			scene->DeleteEntity(child);
		}
		else if (name == "Foliage Mesh") {
			scene->DeleteEntity(child);
		}
		else if (name == "Fruit Mesh") {
			scene->DeleteEntity(child);
		}
	}
	if (meshGeneratorSettings.m_enableBranch)
	{
		branchEntity = scene->CreateEntity("Branch Mesh");
		scene->SetParent(branchEntity, self);

		std::vector<Vertex> vertices;
		std::vector<unsigned int> indices;
		switch (meshGeneratorSettings.m_branchMeshType)
		{
		case 0:
		{
			CylindricalMeshGenerator<SkeletonGrowthData, BranchGrowthData, InternodeGrowthData> meshGenerator;
			meshGenerator.Generate(m_treeModel.RefBranchSkeleton(), vertices, indices,
				meshGeneratorSettings);
		}
		break;
		case 1:
		{
			const auto treeDescriptor = m_treeDescriptor.Get<TreeDescriptor>();
			float minRadius = 0.01f;
			if (treeDescriptor)
			{
				minRadius = treeDescriptor->m_treeGrowthParameters.m_endNodeThickness;
			}
			VoxelMeshGenerator<SkeletonGrowthData, BranchGrowthData, InternodeGrowthData> meshGenerator;
			meshGenerator.Generate(m_treeModel.RefBranchSkeleton(), vertices, indices,
				meshGeneratorSettings, minRadius);
		}
		break;
		default: break;
		}


		auto mesh = ProjectManager::CreateTemporaryAsset<Mesh>();
		auto material = ProjectManager::CreateTemporaryAsset<Material>();
		material->SetProgram(DefaultResources::GLPrograms::StandardProgram);
		mesh->SetVertices(17, vertices, indices);
		auto meshRenderer = scene->GetOrSetPrivateComponent<MeshRenderer>(branchEntity).lock();
		material->m_materialProperties.m_albedoColor = glm::vec3(109, 79, 75) / 255.0f;
		material->m_materialProperties.m_roughness = 0.0f;
		meshRenderer->m_mesh = mesh;
		meshRenderer->m_material = material;
	}
	if (meshGeneratorSettings.m_enableRoot)
	{
		rootEntity = scene->CreateEntity("Root Mesh");
		scene->SetParent(rootEntity, self);
		std::vector<Vertex> vertices;
		std::vector<unsigned int> indices;
		switch (meshGeneratorSettings.m_rootMeshType)
		{
		case 0:
		{
			CylindricalMeshGenerator<RootSkeletonGrowthData, RootBranchGrowthData, RootInternodeGrowthData> meshGenerator;
			meshGenerator.Generate(m_treeModel.RefRootSkeleton(), vertices, indices,
				meshGeneratorSettings);
		}
		break;
		case 1:
		{
			const auto treeDescriptor = m_treeDescriptor.Get<TreeDescriptor>();
			float minRadius = 0.01f;
			if (treeDescriptor)
			{
				minRadius = treeDescriptor->m_rootGrowthParameters.m_endNodeThickness;
			}
			VoxelMeshGenerator<RootSkeletonGrowthData, RootBranchGrowthData, RootInternodeGrowthData> meshGenerator;
			meshGenerator.Generate(m_treeModel.RefRootSkeleton(), vertices, indices,
				meshGeneratorSettings, minRadius);
		}
		break;
		default: break;
		}
		auto mesh = ProjectManager::CreateTemporaryAsset<Mesh>();
		auto material = ProjectManager::CreateTemporaryAsset<Material>();
		material->SetProgram(DefaultResources::GLPrograms::StandardProgram);
		material->m_materialProperties.m_albedoColor = glm::vec3(80, 60, 50) / 255.0f;
		material->m_materialProperties.m_roughness = 1.0f;
		material->m_materialProperties.m_metallic = 0.0f;
		mesh->SetVertices(17, vertices, indices);
		auto meshRenderer = scene->GetOrSetPrivateComponent<MeshRenderer>(rootEntity).lock();
		meshRenderer->m_mesh = mesh;
		meshRenderer->m_material = material;
	}
	if (meshGeneratorSettings.m_enableFoliage)
	{
		foliageEntity = scene->CreateEntity("Foliage Mesh");
		scene->SetParent(foliageEntity, self);


		std::vector<Vertex> vertices;
		std::vector<unsigned int> indices;
		auto quadMesh = DefaultResources::Primitives::Quad;
		auto& quadTriangles = quadMesh->UnsafeGetTriangles();
		auto quadVerticesSize = quadMesh->GetVerticesAmount();
		size_t offset = 0;

		const auto& nodeList = m_treeModel.RefBranchSkeleton().RefSortedNodeList();
		for (const auto& internodeHandle : nodeList) {
			const auto& internode = m_treeModel.RefBranchSkeleton().RefNode(internodeHandle);
			const auto& internodeInfo = internode.m_info;
			const auto& internodeData = internode.m_data;
			auto internodeGlobalTransform = glm::translate(internodeInfo.m_globalPosition) * glm::mat4_cast(internodeInfo.m_globalRotation) * glm::scale(glm::vec3(1.0f));
			for (const auto& bud : internodeData.m_buds) {
				if (bud.m_status != BudStatus::Flushed) continue;
				if (bud.m_maturity <= 0.0f) continue;
				if (bud.m_type == BudType::Leaf)
				{
					auto matrix = internodeGlobalTransform * bud.m_reproductiveModuleTransform;
					Vertex archetype;
					for (auto i = 0; i < quadMesh->GetVerticesAmount(); i++) {
						archetype.m_position =
							matrix * glm::vec4(quadMesh->UnsafeGetVertices()[i].m_position, 1.0f);
						archetype.m_normal = glm::normalize(glm::vec3(
							matrix * glm::vec4(quadMesh->UnsafeGetVertices()[i].m_normal, 0.0f)));
						archetype.m_tangent = glm::normalize(glm::vec3(
							matrix *
							glm::vec4(quadMesh->UnsafeGetVertices()[i].m_tangent, 0.0f)));
						archetype.m_texCoord =
							quadMesh->UnsafeGetVertices()[i].m_texCoord;
						vertices.push_back(archetype);
					}
					for (auto triangle : quadTriangles) {
						triangle.x += offset;
						triangle.y += offset;
						triangle.z += offset;
						indices.push_back(triangle.x);
						indices.push_back(triangle.y);
						indices.push_back(triangle.z);
					}
					offset += quadVerticesSize;
				}
			}
		}

		auto mesh = ProjectManager::CreateTemporaryAsset<Mesh>();
		auto material = ProjectManager::CreateTemporaryAsset<Material>();
		material->SetProgram(DefaultResources::GLPrograms::StandardProgram);
		mesh->SetVertices(17, vertices, indices);
		material->m_materialProperties.m_albedoColor = glm::vec3(152 / 255.0f, 203 / 255.0f, 0 / 255.0f);
		material->m_materialProperties.m_roughness = 0.0f;
		auto meshRenderer = scene->GetOrSetPrivateComponent<MeshRenderer>(foliageEntity).lock();
		meshRenderer->m_mesh = mesh;
		meshRenderer->m_material = material;
	}
}

void TreeDescriptor::OnCreate() {

}


bool OnInspectTreeGrowthParameters(TreeGrowthParameters& treeGrowthParameters) {
	bool changed = false;
	if (ImGui::TreeNodeEx("Tree Parameters", ImGuiTreeNodeFlags_DefaultOpen)) {
		changed = ImGui::DragFloat("Growth rate", &treeGrowthParameters.m_growthRate, 0.01f) || changed;
		if (ImGui::TreeNodeEx("Structure", ImGuiTreeNodeFlags_DefaultOpen)) {
			changed = ImGui::DragInt3("Bud count lateral/fruit/leaf", &treeGrowthParameters.m_lateralBudCount, 1, 0, 3) || changed;
			changed = ImGui::DragFloat2("Branching Angle mean/var", &treeGrowthParameters.m_branchingAngleMeanVariance.x, 0.01f, 0.0f, 100.0f) || changed;
			changed = ImGui::DragFloat2("Roll Angle mean/var", &treeGrowthParameters.m_rollAngleMeanVariance.x, 0.01f, 0.0f, 100.0f) || changed;
			changed = ImGui::DragFloat2("Apical Angle mean/var", &treeGrowthParameters.m_apicalAngleMeanVariance.x, 0.01f, 0.0f, 100.0f) || changed;
			changed = ImGui::DragFloat("Gravitropism", &treeGrowthParameters.m_gravitropism, 0.01f) || changed;
			changed = ImGui::DragFloat("Phototropism", &treeGrowthParameters.m_phototropism, 0.01f) || changed;
			changed = ImGui::DragFloat("Internode length", &treeGrowthParameters.m_internodeLength, 0.01f) || changed;
			changed = ImGui::DragFloat3("Thickness min/factor/age", &treeGrowthParameters.m_endNodeThickness, 0.00001f, 0.0f, 1.0f, "%.6f") || changed;
			changed = ImGui::DragFloat("Low Branch Pruning", &treeGrowthParameters.m_lowBranchPruning, 0.01f) || changed;
			changed = ImGui::DragFloat("Light pruning factor", &treeGrowthParameters.m_endNodePruningLightFactor, 0.01f) || changed;
			changed = ImGui::DragFloat3("Sagging thickness/reduction/max", &treeGrowthParameters.m_saggingFactorThicknessReductionMax.x, 0.01f, 0.0f, 1.0f, "%.5f") || changed;
			ImGui::TreePop();
		}
		if (ImGui::TreeNodeEx("Bud Fate", ImGuiTreeNodeFlags_DefaultOpen)) {
			changed = ImGui::DragFloat4("Shoot flushing prob/temp range", &treeGrowthParameters.m_lateralBudFlushingProbabilityTemperatureRange.x, 0.01f) || changed;
			changed = ImGui::DragFloat4("Leaf flushing prob/temp range", &treeGrowthParameters.m_leafBudFlushingProbabilityTemperatureRange.x, 0.01f) || changed;
			changed = ImGui::DragFloat4("Fruit flushing prob/temp range", &treeGrowthParameters.m_fruitBudFlushingProbabilityTemperatureRange.x, 0.01f) || changed;
			changed = ImGui::DragFloat4("Bud light factor apical/lateral/leaf/fruit", &treeGrowthParameters.m_apicalBudLightingFactor, 0.01f) || changed;
			changed = ImGui::DragFloat3("Apical control base/age/dist", &treeGrowthParameters.m_apicalControl, 0.01f) || changed;
			changed = ImGui::DragFloat3("Apical dominance base/age/dist", &treeGrowthParameters.m_apicalDominance, 0.01f) || changed;
			//ImGui::Text("Max age / distance: [%.3f]", treeGrowthParameters.m_apicalDominance / treeGrowthParameters.m_apicalDominanceDistanceFactor);
			changed = ImGui::DragFloat4("Remove rate apical/lateral/leaf/fruit", &treeGrowthParameters.m_apicalBudExtinctionRate, 0.01f) || changed;
			changed = ImGui::DragFloat3("Base resource shoot/leaf/fruit", &treeGrowthParameters.m_shootMaintenanceVigorRequirement, 0.01f) || changed;
			changed = ImGui::DragFloat3("Productive resource shoot/leaf/fruit", &treeGrowthParameters.m_shootProductiveWaterRequirement, 0.01f) || changed;

			ImGui::TreePop();
		}
		if (ImGui::TreeNodeEx("Foliage", ImGuiTreeNodeFlags_DefaultOpen))
		{
			changed = ImGui::DragFloat3("Size", &treeGrowthParameters.m_maxLeafSize.x, 0.01f) || changed;
			changed = ImGui::DragFloat("Position Variance", &treeGrowthParameters.m_leafPositionVariance, 0.01f) || changed;
			changed = ImGui::DragFloat("Random rotation", &treeGrowthParameters.m_leafRandomRotation, 0.01f) || changed;
			changed = ImGui::DragFloat("Chlorophyll Loss", &treeGrowthParameters.m_leafChlorophyllLoss, 0.01f) || changed;
			changed = ImGui::DragFloat("Chlorophyll temperature", &treeGrowthParameters.m_leafChlorophyllSynthesisFactorTemperature, 0.01f) || changed;
			changed = ImGui::DragFloat("Drop prob", &treeGrowthParameters.m_leafFallProbability, 0.01f) || changed;
			changed = ImGui::DragFloat("Distance To End Limit", &treeGrowthParameters.m_leafDistanceToBranchEndLimit, 0.01f) || changed;
			ImGui::TreePop();
		}
		if (ImGui::TreeNodeEx("Fruit", ImGuiTreeNodeFlags_DefaultOpen))
		{
			changed = ImGui::DragFloat3("Size", &treeGrowthParameters.m_maxFruitSize.x, 0.01f) || changed;
			changed = ImGui::DragFloat("Position Variance", &treeGrowthParameters.m_fruitPositionVariance, 0.01f) || changed;
			changed = ImGui::DragFloat("Random rotation", &treeGrowthParameters.m_fruitRandomRotation, 0.01f) || changed;
			ImGui::TreePop();
		}
		ImGui::TreePop();
	}

	return changed;
}

bool OnInspectRootGrowthParameters(RootGrowthParameters& rootGrowthParameters) {
	bool changed = false;
	if (ImGui::TreeNodeEx("Root Parameters", ImGuiTreeNodeFlags_DefaultOpen)) {
		changed = ImGui::DragFloat("Growth rate", &rootGrowthParameters.m_growthRate, 0.01f) || changed;
		if (ImGui::TreeNodeEx("Structure", ImGuiTreeNodeFlags_DefaultOpen)) {
			changed = ImGui::DragFloat("Root node length", &rootGrowthParameters.m_rootNodeLength, 0.01f) || changed;
			
			changed = ImGui::DragFloat3("Thickness min/factor/age", &rootGrowthParameters.m_endNodeThickness, 0.00001f, 0.0f, 1.0f, "%.6f") || changed;
			
			ImGui::TreePop();
		}
		if (ImGui::TreeNodeEx("Growth", ImGuiTreeNodeFlags_DefaultOpen))
		{
			changed = ImGui::DragFloat2("Branching Angle mean/var", &rootGrowthParameters.m_branchingAngleMeanVariance.x, 0.01f) || changed;
			changed = ImGui::DragFloat2("Roll Angle mean/var", &rootGrowthParameters.m_rollAngleMeanVariance.x, 0.01f) || changed;
			changed = ImGui::DragFloat2("Apical Angle mean/var", &rootGrowthParameters.m_apicalAngleMeanVariance.x, 0.01f) || changed;
			changed = ImGui::DragFloat2("Environmental friction base/factor", &rootGrowthParameters.m_environmentalFriction, 0.01f) || changed;
			changed = ImGui::DragFloat2("Apical control base/dist", &rootGrowthParameters.m_apicalControlBaseDistFactor.x, 0.01f) || changed;
			changed = ImGui::DragFloat2("Apical dominance base/dist", &rootGrowthParameters.m_apicalDominance, 0.01f) || changed;
			changed = ImGui::DragFloat("Tropism switching prob", &rootGrowthParameters.m_tropismSwitchingProbability, 0.01f) || changed;
			changed = ImGui::DragFloat("Tropism switching prob dist factor", &rootGrowthParameters.m_tropismSwitchingProbabilityDistanceFactor, 0.01f) || changed;
			changed = ImGui::DragFloat("Tropism intensity", &rootGrowthParameters.m_tropismIntensity, 0.01f) || changed;
			changed = ImGui::DragFloat("Branching prob base", &rootGrowthParameters.m_baseBranchingProbability, 0.01f) || changed;
			changed = ImGui::DragFloat("Branching prob child decrease", &rootGrowthParameters.m_branchingProbabilityChildrenDecrease, 0.01f) || changed;
			changed = ImGui::DragFloat("Branching prob dist decrease", &rootGrowthParameters.m_branchingProbabilityDistanceDecrease, 0.01f) || changed;
			changed = ImGui::DragFloat("Branching prob order decrease", &rootGrowthParameters.m_branchingProbabilityOrderDecrease, 0.01f) || changed;
			ImGui::TreePop();
		}
		ImGui::TreePop();
	}
	return changed;
}

void TreeDescriptor::OnInspect() {
	bool changed = false;
	auto environmentLayer = Application::GetLayer<EcoSysLabLayer>();
	const auto soil = environmentLayer->m_soilHolder.Get<Soil>();
	const auto climate = environmentLayer->m_climateHolder.Get<Climate>();
	if (soil && climate) {
		if (ImGui::Button("Instantiate")) {
			auto scene = Application::GetActiveScene();
			auto treeEntity = scene->CreateEntity(GetTitle());
			auto tree = scene->GetOrSetPrivateComponent<Tree>(treeEntity).lock();
			float height = 0;
			auto soilDescriptor = soil->m_soilDescriptor.Get<SoilDescriptor>();
			if (soilDescriptor)
			{
				auto heightField = soilDescriptor->m_heightField.Get<HeightField>();
				if (heightField) height = heightField->GetValue({ 0.0f, 0.0f }) - 0.05f;
			}
			GlobalTransform globalTransform;
			globalTransform.SetPosition(glm::vec3(0, height, 0));
			scene->SetDataComponent(treeEntity, globalTransform);
			tree->m_treeDescriptor = ProjectManager::GetAsset(GetHandle());
		}
	}
	else
	{
		ImGui::Text("Attach soil and climate entity to instantiate!");
	}
	if (OnInspectTreeGrowthParameters(m_treeGrowthParameters)) { changed = true; }
	if (OnInspectRootGrowthParameters(m_rootGrowthParameters)) { changed = true; }
	if (changed) m_saved = false;
}

void TreeDescriptor::CollectAssetRef(std::vector<AssetRef>& list) {

}

void SerializeTreeGrowthParameters(const std::string& name, const TreeGrowthParameters& treeGrowthParameters, YAML::Emitter& out) {
	out << YAML::Key << name << YAML::BeginMap;
	out << YAML::Key << "m_growthRate" << YAML::Value << treeGrowthParameters.m_growthRate;

	//Structure
	out << YAML::Key << "m_lateralBudCount" << YAML::Value << treeGrowthParameters.m_lateralBudCount;
	out << YAML::Key << "m_fruitBudCount" << YAML::Value << treeGrowthParameters.m_fruitBudCount;
	out << YAML::Key << "m_leafBudCount" << YAML::Value << treeGrowthParameters.m_leafBudCount;
	out << YAML::Key << "m_branchingAngleMeanVariance" << YAML::Value << treeGrowthParameters.m_branchingAngleMeanVariance;
	out << YAML::Key << "m_rollAngleMeanVariance" << YAML::Value << treeGrowthParameters.m_rollAngleMeanVariance;
	out << YAML::Key << "m_apicalAngleMeanVariance" << YAML::Value << treeGrowthParameters.m_apicalAngleMeanVariance;
	out << YAML::Key << "m_gravitropism" << YAML::Value << treeGrowthParameters.m_gravitropism;
	out << YAML::Key << "m_phototropism" << YAML::Value << treeGrowthParameters.m_phototropism;
	out << YAML::Key << "m_internodeLength" << YAML::Value << treeGrowthParameters.m_internodeLength;
	out << YAML::Key << "m_endNodeThickness" << YAML::Value << treeGrowthParameters.m_endNodeThickness;
	out << YAML::Key << "m_thicknessAccumulationFactor" << YAML::Value << treeGrowthParameters.m_thicknessAccumulationFactor;
	out << YAML::Key << "m_thicknessAccumulateAgeFactor" << YAML::Value << treeGrowthParameters.m_thicknessAccumulateAgeFactor;

	//Bud
	out << YAML::Key << "m_lateralBudFlushingProbabilityTemperatureRange" << YAML::Value << treeGrowthParameters.m_lateralBudFlushingProbabilityTemperatureRange;
	out << YAML::Key << "m_leafBudFlushingProbabilityTemperatureRange" << YAML::Value << treeGrowthParameters.m_leafBudFlushingProbabilityTemperatureRange;
	out << YAML::Key << "m_fruitBudFlushingProbabilityTemperatureRange" << YAML::Value << treeGrowthParameters.m_fruitBudFlushingProbabilityTemperatureRange;

	out << YAML::Key << "m_apicalBudLightingFactor" << YAML::Value << treeGrowthParameters.m_apicalBudLightingFactor;
	out << YAML::Key << "m_lateralBudLightingFactor" << YAML::Value << treeGrowthParameters.m_lateralBudLightingFactor;
	out << YAML::Key << "m_leafBudLightingFactor" << YAML::Value << treeGrowthParameters.m_leafBudLightingFactor;
	out << YAML::Key << "m_fruitBudLightingFactor" << YAML::Value << treeGrowthParameters.m_fruitBudLightingFactor;


	out << YAML::Key << "m_apicalControl" << YAML::Value << treeGrowthParameters.m_apicalControl;
	out << YAML::Key << "m_apicalControlAgeFactor" << YAML::Value << treeGrowthParameters.m_apicalControlAgeFactor;
	out << YAML::Key << "m_apicalControlDistanceFactor" << YAML::Value << treeGrowthParameters.m_apicalControlDistanceFactor;

	out << YAML::Key << "m_apicalDominance" << YAML::Value << treeGrowthParameters.m_apicalDominance;
	out << YAML::Key << "m_apicalDominanceAgeFactor" << YAML::Value << treeGrowthParameters.m_apicalDominanceAgeFactor;
	out << YAML::Key << "m_apicalDominanceDistanceFactor" << YAML::Value << treeGrowthParameters.m_apicalDominanceDistanceFactor;

	out << YAML::Key << "m_apicalBudExtinctionRate" << YAML::Value << treeGrowthParameters.m_apicalBudExtinctionRate;
	out << YAML::Key << "m_lateralBudExtinctionRate" << YAML::Value << treeGrowthParameters.m_lateralBudExtinctionRate;
	out << YAML::Key << "m_leafBudExtinctionRate" << YAML::Value << treeGrowthParameters.m_leafBudExtinctionRate;
	out << YAML::Key << "m_fruitBudExtinctionRate" << YAML::Value << treeGrowthParameters.m_fruitBudExtinctionRate;

	out << YAML::Key << "m_shootMaintenanceVigorRequirement" << YAML::Value << treeGrowthParameters.m_shootMaintenanceVigorRequirement;
	out << YAML::Key << "m_leafMaintenanceVigorRequirement" << YAML::Value << treeGrowthParameters.m_leafMaintenanceVigorRequirement;
	out << YAML::Key << "m_fruitBaseWaterRequirement" << YAML::Value << treeGrowthParameters.m_fruitBaseWaterRequirement;
	out << YAML::Key << "m_shootProductiveWaterRequirement" << YAML::Value << treeGrowthParameters.m_shootProductiveWaterRequirement;
	out << YAML::Key << "m_leafProductiveWaterRequirement" << YAML::Value << treeGrowthParameters.m_leafProductiveWaterRequirement;
	out << YAML::Key << "m_fruitProductiveWaterRequirement" << YAML::Value << treeGrowthParameters.m_fruitProductiveWaterRequirement;


	//Internode
	out << YAML::Key << "m_lowBranchPruning" << YAML::Value << treeGrowthParameters.m_lowBranchPruning;
	out << YAML::Key << "m_saggingFactorThicknessReductionMax" << YAML::Value << treeGrowthParameters.m_saggingFactorThicknessReductionMax;

	//Foliage
	out << YAML::Key << "m_maxLeafSize" << YAML::Value << treeGrowthParameters.m_maxLeafSize;
	out << YAML::Key << "m_leafPositionVariance" << YAML::Value << treeGrowthParameters.m_leafPositionVariance;
	out << YAML::Key << "m_leafRandomRotation" << YAML::Value << treeGrowthParameters.m_leafRandomRotation;
	out << YAML::Key << "m_leafChlorophyllLoss" << YAML::Value << treeGrowthParameters.m_leafChlorophyllLoss;
	out << YAML::Key << "m_leafChlorophyllSynthesisFactorTemperature" << YAML::Value << treeGrowthParameters.m_leafChlorophyllSynthesisFactorTemperature;
	out << YAML::Key << "m_leafFallProbability" << YAML::Value << treeGrowthParameters.m_leafFallProbability;
	out << YAML::Key << "m_leafDistanceToBranchEndLimit" << YAML::Value << treeGrowthParameters.m_leafDistanceToBranchEndLimit;

	out << YAML::Key << "m_maxFruitSize" << YAML::Value << treeGrowthParameters.m_maxFruitSize;
	out << YAML::Key << "m_fruitPositionVariance" << YAML::Value << treeGrowthParameters.m_fruitPositionVariance;
	out << YAML::Key << "m_fruitRandomRotation" << YAML::Value << treeGrowthParameters.m_fruitRandomRotation;

	out << YAML::EndMap;
}
void SerializeRootGrowthParameters(const std::string& name, const RootGrowthParameters& rootGrowthParameters, YAML::Emitter& out) {
	out << YAML::Key << name << YAML::BeginMap;
	out << YAML::Key << "m_growthRate" << YAML::Value << rootGrowthParameters.m_growthRate;

	out << YAML::Key << "m_branchingAngleMeanVariance" << YAML::Value
		<< rootGrowthParameters.m_branchingAngleMeanVariance;
	out << YAML::Key << "m_rollAngleMeanVariance" << YAML::Value
		<< rootGrowthParameters.m_rollAngleMeanVariance;
	out << YAML::Key << "m_apicalAngleMeanVariance" << YAML::Value
		<< rootGrowthParameters.m_apicalAngleMeanVariance;
	out << YAML::Key << "m_rootNodeLength" << YAML::Value << rootGrowthParameters.m_rootNodeLength;
	
	out << YAML::Key << "m_endNodeThickness" << YAML::Value
		<< rootGrowthParameters.m_endNodeThickness;
	out << YAML::Key << "m_thicknessAccumulationFactor" << YAML::Value
		<< rootGrowthParameters.m_thicknessAccumulationFactor;
	out << YAML::Key << "m_thicknessAccumulateAgeFactor" << YAML::Value
		<< rootGrowthParameters.m_thicknessAccumulateAgeFactor;

	

	out << YAML::Key << "m_environmentalFriction" << YAML::Value << rootGrowthParameters.m_environmentalFriction;
	out << YAML::Key << "m_environmentalFrictionFactor" << YAML::Value << rootGrowthParameters.m_environmentalFrictionFactor;


	out << YAML::Key << "m_apicalControlBaseDistFactor" << YAML::Value << rootGrowthParameters.m_apicalControlBaseDistFactor;
	out << YAML::Key << "m_apicalDominance" << YAML::Value << rootGrowthParameters.m_apicalDominance;
	out << YAML::Key << "m_apicalDominanceDistanceFactor" << YAML::Value << rootGrowthParameters.m_apicalDominanceDistanceFactor;
	out << YAML::Key << "m_tropismSwitchingProbability" << YAML::Value << rootGrowthParameters.m_tropismSwitchingProbability;
	out << YAML::Key << "m_tropismSwitchingProbabilityDistanceFactor" << YAML::Value << rootGrowthParameters.m_tropismSwitchingProbabilityDistanceFactor;
	out << YAML::Key << "m_tropismIntensity" << YAML::Value << rootGrowthParameters.m_tropismIntensity;
	out << YAML::Key << "m_baseBranchingProbability" << YAML::Value << rootGrowthParameters.m_baseBranchingProbability;
	out << YAML::Key << "m_branchingProbabilityChildrenDecrease" << YAML::Value << rootGrowthParameters.m_branchingProbabilityChildrenDecrease;
	out << YAML::Key << "m_branchingProbabilityDistanceDecrease" << YAML::Value << rootGrowthParameters.m_branchingProbabilityDistanceDecrease;
	out << YAML::Key << "m_branchingProbabilityOrderDecrease" << YAML::Value << rootGrowthParameters.m_branchingProbabilityOrderDecrease;

	out << YAML::EndMap;
}
void TreeDescriptor::Serialize(YAML::Emitter& out) {
	SerializeTreeGrowthParameters("m_treeGrowthParameters", m_treeGrowthParameters, out);
	SerializeRootGrowthParameters("m_rootGrowthParameters", m_rootGrowthParameters, out);
}

void DeserializeTreeGrowthParameters(const std::string& name, TreeGrowthParameters& treeGrowthParameters, const YAML::Node& in) {
	if (in[name]) {
		auto& param = in[name];
		if (param["m_growthRate"]) treeGrowthParameters.m_growthRate = param["m_growthRate"].as<float>();
		//Structure
		if (param["m_lateralBudCount"]) treeGrowthParameters.m_lateralBudCount = param["m_lateralBudCount"].as<int>();
		if (param["m_fruitBudCount"]) treeGrowthParameters.m_fruitBudCount = param["m_fruitBudCount"].as<int>();
		if (param["m_leafBudCount"]) treeGrowthParameters.m_leafBudCount = param["m_leafBudCount"].as<int>();

		if (param["m_branchingAngleMeanVariance"]) treeGrowthParameters.m_branchingAngleMeanVariance = param["m_branchingAngleMeanVariance"].as<glm::vec2>();
		if (param["m_rollAngleMeanVariance"]) treeGrowthParameters.m_rollAngleMeanVariance = param["m_rollAngleMeanVariance"].as<glm::vec2>();
		if (param["m_apicalAngleMeanVariance"]) treeGrowthParameters.m_apicalAngleMeanVariance = param["m_apicalAngleMeanVariance"].as<glm::vec2>();
		if (param["m_gravitropism"]) treeGrowthParameters.m_gravitropism = param["m_gravitropism"].as<float>();
		if (param["m_phototropism"]) treeGrowthParameters.m_phototropism = param["m_phototropism"].as<float>();

		if (param["m_internodeLength"]) treeGrowthParameters.m_internodeLength = param["m_internodeLength"].as<float>();
		if (param["m_endNodeThickness"]) treeGrowthParameters.m_endNodeThickness = param["m_endNodeThickness"].as<float>();
		if (param["m_thicknessAccumulationFactor"]) treeGrowthParameters.m_thicknessAccumulationFactor = param["m_thicknessAccumulationFactor"].as<float>();
		if (param["m_thicknessAccumulateAgeFactor"]) treeGrowthParameters.m_thicknessAccumulateAgeFactor = param["m_thicknessAccumulateAgeFactor"].as<float>();

		if (param["m_lowBranchPruning"]) treeGrowthParameters.m_lowBranchPruning = param["m_lowBranchPruning"].as<float>();
		if (param["m_saggingFactorThicknessReductionMax"]) treeGrowthParameters.m_saggingFactorThicknessReductionMax = param["m_saggingFactorThicknessReductionMax"].as<glm::vec3>();

		//Bud fate
		
		if (param["m_lateralBudFlushingProbabilityTemperatureRange"]) treeGrowthParameters.m_lateralBudFlushingProbabilityTemperatureRange = param["m_lateralBudFlushingProbabilityTemperatureRange"].as<glm::vec4>();
		if (param["m_leafBudFlushingProbabilityTemperatureRange"]) treeGrowthParameters.m_leafBudFlushingProbabilityTemperatureRange = param["m_leafBudFlushingProbabilityTemperatureRange"].as< glm::vec4>();
		if (param["m_fruitBudFlushingProbabilityTemperatureRange"]) treeGrowthParameters.m_fruitBudFlushingProbabilityTemperatureRange = param["m_fruitBudFlushingProbabilityTemperatureRange"].as<glm::vec4>();

		if (param["m_apicalBudLightingFactor"]) treeGrowthParameters.m_apicalBudLightingFactor = param["m_apicalBudLightingFactor"].as<float>();
		if (param["m_lateralBudLightingFactor"]) treeGrowthParameters.m_lateralBudLightingFactor = param["m_lateralBudLightingFactor"].as<float>();
		if (param["m_leafBudLightingFactor"]) treeGrowthParameters.m_leafBudLightingFactor = param["m_leafBudLightingFactor"].as<float>();
		if (param["m_fruitBudLightingFactor"]) treeGrowthParameters.m_fruitBudLightingFactor = param["m_fruitBudLightingFactor"].as<float>();

		if (param["m_apicalControl"]) treeGrowthParameters.m_apicalControl = param["m_apicalControl"].as<float>();
		if (param["m_apicalControlAgeFactor"]) treeGrowthParameters.m_apicalControlAgeFactor = param["m_apicalControlAgeFactor"].as<float>();
		if (param["m_apicalControlDistanceFactor"]) treeGrowthParameters.m_apicalControlDistanceFactor = param["m_apicalControlDistanceFactor"].as<float>();
		if (param["m_apicalDominance"]) treeGrowthParameters.m_apicalDominance = param["m_apicalDominance"].as<float>();
		if (param["m_apicalDominanceAgeFactor"]) treeGrowthParameters.m_apicalDominanceAgeFactor = param["m_apicalDominanceAgeFactor"].as<float>();
		if (param["m_apicalDominanceDistanceFactor"]) treeGrowthParameters.m_apicalDominanceDistanceFactor = param["m_apicalDominanceDistanceFactor"].as<float>();

		if (param["m_apicalBudExtinctionRate"]) treeGrowthParameters.m_apicalBudExtinctionRate = param["m_apicalBudExtinctionRate"].as<float>();
		if (param["m_lateralBudExtinctionRate"]) treeGrowthParameters.m_lateralBudExtinctionRate = param["m_lateralBudExtinctionRate"].as<float>();
		if (param["m_leafBudExtinctionRate"]) treeGrowthParameters.m_leafBudExtinctionRate = param["m_leafBudExtinctionRate"].as<float>();
		if (param["m_fruitBudExtinctionRate"]) treeGrowthParameters.m_fruitBudExtinctionRate = param["m_fruitBudExtinctionRate"].as<float>();

		if (param["m_shootMaintenanceVigorRequirement"]) treeGrowthParameters.m_shootMaintenanceVigorRequirement = param["m_shootMaintenanceVigorRequirement"].as<float>();
		if (param["m_leafMaintenanceVigorRequirement"]) treeGrowthParameters.m_leafMaintenanceVigorRequirement = param["m_leafMaintenanceVigorRequirement"].as<float>();
		if (param["m_fruitBaseWaterRequirement"]) treeGrowthParameters.m_fruitBaseWaterRequirement = param["m_fruitBaseWaterRequirement"].as<float>();
		if (param["m_shootProductiveWaterRequirement"]) treeGrowthParameters.m_shootProductiveWaterRequirement = param["m_shootProductiveWaterRequirement"].as<float>();
		if (param["m_leafProductiveWaterRequirement"]) treeGrowthParameters.m_leafProductiveWaterRequirement = param["m_leafProductiveWaterRequirement"].as<float>();
		if (param["m_fruitProductiveWaterRequirement"]) treeGrowthParameters.m_fruitProductiveWaterRequirement = param["m_fruitProductiveWaterRequirement"].as<float>();


		//Foliage
		if (param["m_maxLeafSize"]) treeGrowthParameters.m_maxLeafSize = param["m_maxLeafSize"].as<glm::vec3>();
		if (param["m_leafPositionVariance"]) treeGrowthParameters.m_leafPositionVariance = param["m_leafPositionVariance"].as<float>();
		if (param["m_leafRandomRotation"]) treeGrowthParameters.m_leafRandomRotation = param["m_leafRandomRotation"].as<float>();
		if (param["m_leafChlorophyllLoss"]) treeGrowthParameters.m_leafChlorophyllLoss = param["m_leafChlorophyllLoss"].as<float>();
		if (param["m_leafChlorophyllSynthesisFactorTemperature"]) treeGrowthParameters.m_leafChlorophyllSynthesisFactorTemperature = param["m_leafChlorophyllSynthesisFactorTemperature"].as<float>();
		if (param["m_leafFallProbability"]) treeGrowthParameters.m_leafFallProbability = param["m_leafFallProbability"].as<float>();
		if (param["m_leafDistanceToBranchEndLimit"]) treeGrowthParameters.m_leafDistanceToBranchEndLimit = param["m_leafDistanceToBranchEndLimit"].as<float>();

		//Fruit
		if (param["m_maxFruitSize"]) treeGrowthParameters.m_maxFruitSize = param["m_maxFruitSize"].as<glm::vec3>();
		if (param["m_fruitPositionVariance"]) treeGrowthParameters.m_fruitPositionVariance = param["m_fruitPositionVariance"].as<float>();
		if (param["m_fruitRandomRotation"]) treeGrowthParameters.m_fruitRandomRotation = param["m_fruitRandomRotation"].as<float>();

	}
}
void DeserializeRootGrowthParameters(const std::string& name, RootGrowthParameters& rootGrowthParameters, const YAML::Node& in) {
	if (in[name]) {
		auto& param = in[name];
		if (param["m_rootNodeLength"]) rootGrowthParameters.m_rootNodeLength = param["m_rootNodeLength"].as<float>();
		if (param["m_growthRate"]) rootGrowthParameters.m_growthRate = param["m_growthRate"].as<float>();
		if (param["m_endNodeThickness"]) rootGrowthParameters.m_endNodeThickness = param["m_endNodeThickness"].as<float>();
		if (param["m_thicknessAccumulationFactor"]) rootGrowthParameters.m_thicknessAccumulationFactor = param["m_thicknessAccumulationFactor"].as<float>();
		if (param["m_thicknessAccumulateAgeFactor"]) rootGrowthParameters.m_thicknessAccumulateAgeFactor = param["m_thicknessAccumulateAgeFactor"].as<float>();

		if (param["m_branchingAngleMeanVariance"]) rootGrowthParameters.m_branchingAngleMeanVariance = param["m_branchingAngleMeanVariance"].as<glm::vec2>();
		if (param["m_rollAngleMeanVariance"]) rootGrowthParameters.m_rollAngleMeanVariance = param["m_rollAngleMeanVariance"].as<glm::vec2>();
		if (param["m_apicalAngleMeanVariance"]) rootGrowthParameters.m_apicalAngleMeanVariance = param["m_apicalAngleMeanVariance"].as<glm::vec2>();

		if(param["m_environmentalFriction"]) rootGrowthParameters.m_environmentalFriction = param["m_environmentalFriction"].as<float>();
		if(param["m_environmentalFrictionFactor"]) rootGrowthParameters.m_environmentalFrictionFactor = param["m_environmentalFrictionFactor"].as<float>();

		if (param["m_apicalControlBaseDistFactor"]) rootGrowthParameters.m_apicalControlBaseDistFactor = param["m_apicalControlBaseDistFactor"].as<glm::vec2>();
		if (param["m_apicalDominance"]) rootGrowthParameters.m_apicalDominance = param["m_apicalDominance"].as<float>();
		if (param["m_apicalDominanceDistanceFactor"]) rootGrowthParameters.m_apicalDominanceDistanceFactor = param["m_apicalDominanceDistanceFactor"].as<float>();

		if (param["m_tropismSwitchingProbability"]) rootGrowthParameters.m_tropismSwitchingProbability = param["m_tropismSwitchingProbability"].as<float>();
		if (param["m_tropismSwitchingProbabilityDistanceFactor"]) rootGrowthParameters.m_tropismSwitchingProbabilityDistanceFactor = param["m_tropismSwitchingProbabilityDistanceFactor"].as<float>();
		if (param["m_tropismIntensity"]) rootGrowthParameters.m_tropismIntensity = param["m_tropismIntensity"].as<float>();

		if (param["m_baseBranchingProbability"]) rootGrowthParameters.m_baseBranchingProbability = param["m_baseBranchingProbability"].as<float>();
		if (param["m_branchingProbabilityChildrenDecrease"]) rootGrowthParameters.m_branchingProbabilityChildrenDecrease = param["m_branchingProbabilityChildrenDecrease"].as<float>();
		if (param["m_branchingProbabilityDistanceDecrease"]) rootGrowthParameters.m_branchingProbabilityDistanceDecrease = param["m_branchingProbabilityDistanceDecrease"].as<float>();
		if (param["m_branchingProbabilityOrderDecrease"]) rootGrowthParameters.m_branchingProbabilityOrderDecrease = param["m_branchingProbabilityOrderDecrease"].as<float>();

	}
}
void TreeDescriptor::Deserialize(const YAML::Node& in) {
	DeserializeTreeGrowthParameters("m_treeGrowthParameters", m_treeGrowthParameters, in);
	DeserializeRootGrowthParameters("m_rootGrowthParameters", m_rootGrowthParameters, in);
}