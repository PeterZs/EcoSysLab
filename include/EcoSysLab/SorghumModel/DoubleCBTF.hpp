#pragma once

using namespace EvoEngine;
namespace EcoSysLab {

class DoubleCBTF : public IAsset {
public:
  AssetRef m_top;
  AssetRef m_bottom;
  void OnInspect(const std::shared_ptr<EditorLayer>& editorLayer) override;
  void CollectAssetRef(std::vector<AssetRef> &list) override;
  void Serialize(YAML::Emitter &out) override;
  void Deserialize(const YAML::Node &in) override;
};
} // namespace EcoSysLab