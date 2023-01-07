#pragma once

#include "ecosyslab_export.h"
#include "SoilModel.hpp"
using namespace UniEngine;
namespace EcoSysLab
{
	/**
	 * \brief The soil descriptor contains the procedural paremeters for soil model.
	 * It helps provide the user's control menu and serialization outside the portable soil model
	 */
	class SoilDescriptor : public IAsset {
	public:
		glm::vec3 m_boundingBoxMin = glm::vec3(-6.4f, -3.2f, -6.4f);
		glm::uvec3 m_voxelResolution = glm::uvec3(64, 48, 64);
		SoilParameters m_soilParameters;

		AssetRef m_heightField;

		/**ImGui menu goes to here.Also you can take care you visualization with Gizmos here.
		 * Note that the visualization will only be activated while you are inspecting the soil private component in the entity inspector.
		 */
		void OnInspect() override;

		void Serialize(YAML::Emitter& out) override;

		void Deserialize(const YAML::Node& in) override;

		void CollectAssetRef(std::vector<AssetRef>& list) override;
	};
	enum class SoilProperty
	{
		Blank,
		SoilDensity,
		WaterDensityBlur,
		WaterDensity,
		WaterDensityGradient,
		Flux,
		Divergence,
		ScalarDivergence,
		NutrientDensity
	};
	/**
	 * \brief The soil is designed to be a private component of an entity.
	 * It holds the soil model and can be referenced by multiple trees.
	 * The soil will also take care of visualization and menu for soil model.
	 */
	class Soil : public IPrivateComponent {

	public:
		SoilModel m_soilModel;
		AssetRef m_soilDescriptor;

		/**ImGui menu goes to here.Also you can take care you visualization with Gizmos here.
		 * Note that the visualization will only be activated while you are inspecting the soil private component in the entity inspector.
		 */
		void OnInspect() override;

		void Serialize(YAML::Emitter& out) override;

		void Deserialize(const YAML::Node& in) override;

		void CollectAssetRef(std::vector<AssetRef>& list) override;

		void GenerateMesh();
	};
}