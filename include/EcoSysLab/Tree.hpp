#pragma once

#include "ecosyslab_export.h"
#include "TreeModel.hpp"
#include "TreeVisualizer.hpp"
#include "TreeMeshGenerator.hpp"

#include "LSystemString.hpp"
#include "TreeGraph.hpp"

using namespace UniEngine;
namespace EcoSysLab {
	
	class TreeDescriptor : public IAsset {
	public:
		ShootGrowthParameters m_shootGrowthParameters;
		RootGrowthParameters m_rootGrowthParameters;
		void OnCreate() override;

		void OnInspect() override;

		void CollectAssetRef(std::vector<AssetRef>& list) override;

		void Serialize(YAML::Emitter& out) override;

		void Deserialize(const YAML::Node& in) override;
	};

	class Tree : public IPrivateComponent {
		friend class EcoSysLabLayer;
		bool TryGrow(float deltaTime);
		template<typename PipeGroupData, typename PipeData, typename PipeNodeData>
		void BuildStrand(const PipeGroup<PipeGroupData, PipeData, PipeNodeData>& pipeGroup, const Pipe<PipeData>& pipe, std::vector<glm::uint>& strands, std::vector<StrandPoint>& points) const;


	public:
		template<typename PipeGroupData, typename PipeData, typename PipeNodeData>
		void BuildStrands(const PipeGroup<PipeGroupData, PipeData, PipeNodeData>& pipeGroup, std::vector<glm::uint>& strands, std::vector<StrandPoint>& points) const;

		void InitializeStrandRenderer() const;

		void Serialize(YAML::Emitter& out) override;
		bool m_splitRootTest = true;
		bool m_recordBiomassHistory = true;
		float m_leftSideBiomass;
		float m_rightSideBiomass;

		TreeMeshGeneratorSettings m_meshGeneratorSettings;
		int m_temporalProgressionIteration = 0;
		bool m_temporalProgression = false;
		void Update() override;

		void Deserialize(const YAML::Node& in) override;

		std::vector<float> m_rootBiomassHistory;
		std::vector<float> m_shootBiomassHistory;

		PrivateComponentRef m_soil;
		PrivateComponentRef m_climate;
		AssetRef m_treeDescriptor;
		bool m_enableHistory = false;
		int m_historyIteration = 30;
		TreeModel m_treeModel;
		void OnInspect() override;

		void OnDestroy() override;

		void OnCreate() override;

		void ClearMeshes();

		void GenerateMeshes(const TreeMeshGeneratorSettings& meshGeneratorSettings, int iteration = -1);

		void FromLSystemString(const std::shared_ptr<LSystemString>& lSystemString);
		void FromTreeGraph(const std::shared_ptr<TreeGraph>& treeGraph);
		void FromTreeGraphV2(const std::shared_ptr<TreeGraphV2>& treeGraphV2);
	};

	template <typename PipeGroupData, typename PipeData, typename PipeNodeData>
	void Tree::BuildStrand(const PipeGroup<PipeGroupData, PipeData, PipeNodeData>& pipeGroup,
		const Pipe<PipeData>& pipe, std::vector<glm::uint>& strands, std::vector<StrandPoint>& points) const
	{
		const auto& nodeHandles = pipe.PeekPipeNodeHandles();
		if (nodeHandles.empty()) return;
		strands.emplace_back(points.size());
		auto frontPointIndex = points.size();
		StrandPoint point;
		const auto& firstNode = pipeGroup.PeekPipeNode(nodeHandles.front());
		point.m_position = firstNode.m_info.m_globalStartPosition;
		point.m_thickness = 0.001f;
		point.m_color = glm::vec4(0, 1, 0, 1);

		points.emplace_back(point);
		points.emplace_back(point);
		for (const auto& pipeNodeHandle : pipe.PeekPipeNodeHandles())
		{
			const auto& pipeNode = pipeGroup.PeekPipeNode(pipeNodeHandle);
			point.m_color = glm::vec4(0, 1, 0, 1);
			point.m_thickness = 0.001f;
			point.m_position = pipeNode.m_info.m_globalEndPosition;
			points.emplace_back(point);
		}

		StrandPoint frontPoint;
		frontPoint = points.at(frontPointIndex);
		frontPoint.m_position = 2.0f * frontPoint.m_position - points.at(frontPointIndex + 1).m_position;
		points.at(frontPointIndex) = frontPoint;

		StrandPoint backPoint;
		backPoint = points.at(points.size() - 2);
		backPoint.m_position = 2.0f * points.at(points.size() - 1).m_position - backPoint.m_position;
		points.emplace_back(backPoint);
	}

	template <typename PipeGroupData, typename PipeData, typename PipeNodeData>
	void Tree::BuildStrands(const PipeGroup<PipeGroupData, PipeData, PipeNodeData>& pipeGroup, std::vector<glm::uint>& strands, std::vector<StrandPoint>& points) const
	{
		for(const auto& pipe : pipeGroup.PeekPipes())
		{
			BuildStrand(pipeGroup, pipe, strands, points);
		}
	}
}
