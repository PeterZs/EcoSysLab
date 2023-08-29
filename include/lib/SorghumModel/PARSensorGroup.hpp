#pragma once
#ifdef RAYTRACERFACILITY

#include <CUDAModule.hpp>
using namespace EvoEngine;
using namespace RayTracerFacility;
namespace EcoSysLab {
class PARSensorGroup : public IAsset {
public:
  std::vector<IlluminationSampler<glm::vec3>> m_samplers;
  void CalculateIllumination(const RayProperties& rayProperties, int seed, float pushNormalDistance);
  void OnInspect(const std::shared_ptr<EditorLayer>& editorLayer);
  void Serialize(YAML::Emitter &out) override;
  void Deserialize(const YAML::Node &in) override;
};
} // namespace EcoSysLab
#endif