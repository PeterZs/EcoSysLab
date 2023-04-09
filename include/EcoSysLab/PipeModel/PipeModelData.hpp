#pragma once
#include "PlantStructure.hpp"
#include "PipeStructure.hpp"
#include "HexagonGrid.hpp"
using namespace UniEngine;
namespace EcoSysLab
{
	struct BaseHexagonGridCellData
	{
		PipeHandle m_shootPipeHandle = -1;
		PipeHandle m_rootPipeHandle = -1;
	};

	struct BaseHexagonGridData
	{
	};

	struct HexagonGridCellData
	{
		PipeHandle m_pipeHandle = -1;
	};

	struct HexagonGridData
	{
		NodeHandle m_nodeHandle = -1;
	};

	typedef HexagonGrid<BaseHexagonGridData, BaseHexagonGridCellData> PipeModelBaseHexagonGrid;
	typedef HexagonGridGroup<HexagonGridData, HexagonGridCellData> PipeModelHexagonGridGroup;

	struct PipeModelPipeGroupData
	{
	};

	struct PipeModelPipeData
	{
		PipeNodeInfo m_baseInfo;
	};

	struct PipeModelPipeNodeData
	{
		NodeHandle m_nodeHandle = -1;
	};

	typedef PipeGroup<PipeModelPipeGroupData, PipeModelPipeData, PipeModelPipeNodeData> PipeModelPipeGroup;

	struct PipeModelNodeData
	{
		int m_endNodeCount = 0;
		NodeHandle m_originalSkeletonHandle = -1;
	};

	struct PipeModelFlowData
	{
		
	};

	struct PipeModelSkeletonData
	{
		PipeModelPipeGroup m_pipeGroup;
	};
	typedef Skeleton<PipeModelSkeletonData, PipeModelFlowData, PipeModelNodeData> PipeModelSkeleton;
}