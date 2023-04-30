#pragma once

#include "ecosyslab_export.h"
#include "VoxelGrid.hpp"

using namespace UniEngine;
namespace EcoSysLab {
	typedef int PointHandle;
	struct ScannedPoint {
		PointHandle m_handle;
		std::vector<PointHandle> m_neighbors;
		int m_junctionIndex = -1;
		int m_prevHandle = -1;
		int m_nextHandle = -1;
		glm::vec3 m_position = glm::vec3(0.0f);
		glm::vec3 m_direction = glm::vec3(0.0f);
		float m_width = 0;
		bool m_junctionEnd = false;
	};
	struct ConnectivityGraphSettings {
		float m_finderStep = 0.05f;
		float m_edgeLength = 0.25f;
		int m_maxTimeout = 10;
	};

	class TreePointCloud : public IPrivateComponent {
		void FindPoints(PointHandle targetPoint, VoxelGrid<std::vector<PointHandle>> &pointVoxelGrid, float radius,
										const std::function<void(PointHandle handle)> &func) const;

		public:
		void ImportCsv(const std::filesystem::path &path);

		glm::vec3 m_min;
		glm::vec3 m_max;
		std::vector<ScannedPoint> m_points;

		void OnInspect() override;

		std::vector<std::pair<PointHandle, PointHandle>> m_scatterPointsConnections;
		std::vector<std::pair<PointHandle, PointHandle>> m_junctionConnections;
		void EstablishConnectivityGraph(const ConnectivityGraphSettings &settings);
	};
}