#include "ParticlePhysics2DDemo.hpp"

#include <Times.hpp>
using namespace EcoSysLab;

void ParticlePhysics2DDemo::OnInspect(const std::shared_ptr<EditorLayer>& editorLayer)
{
	static bool enableRender = true;
	if (ImGui::Button("Reset"))
	{
		m_particlePhysics2D = {};
	}

	ImGui::Checkbox("Enable render", &enableRender);
	ImGui::DragFloat2("World center", &m_worldCenter.x, 0.001f);
	ImGui::DragFloat("World radius", &m_worldRadius, 0.01f);
	ImGui::DragFloat("Gravity strength", &m_gravityStrength, 0.01f);
	ImGui::DragFloat("Friction", &m_friction, 0.01f, 0.0f, 1.0f);
	static float targetDamping = 0.01f;
	ImGui::DragFloat("Target damping", &targetDamping, 0.01f);
	if (ImGui::Button("Apply damping"))
	{
		for (auto& particle : m_particlePhysics2D.RefParticles())
		{
			particle.SetDamping(targetDamping);
		}
	}
	static bool showGrid = false;
	ImGui::Checkbox("Show Grid", &showGrid);

	if (enableRender)
	{
		const std::string tag = "ParticlePhysics2D Scene [" + std::to_string(GetOwner().GetIndex()) + "]";
		if (ImGui::Begin(tag.c_str()))
		{
			static float elapsedTime = 0.0f;
			elapsedTime += Times::DeltaTime();
			m_particlePhysics2D.OnInspect([&](const glm::vec2 position)
				{
					if (elapsedTime > Times::TimeStep()) {
						elapsedTime = 0.0f;
						const auto particleHandle1 = m_particlePhysics2D.AllocateParticle();
						auto& particle1 = m_particlePhysics2D.RefParticle(particleHandle1);
						particle1.SetColor(glm::vec4(glm::abs(glm::ballRand(1.0f)), 1.0f));
						particle1.SetPosition(position);
						particle1.SetDamping(targetDamping);
						particle1.SetVelocity(glm::vec2(m_particlePhysics2D.m_particleRadius * 2.0f, 0.0f) / static_cast<float>(Times::TimeStep()), m_particlePhysics2D.GetDeltaTime());

						const auto particleHandle2 = m_particlePhysics2D.AllocateParticle();
						auto& particle2 = m_particlePhysics2D.RefParticle(particleHandle2);
						particle2.SetColor(glm::vec4(glm::abs(glm::ballRand(1.0f)), 1.0f));
						particle2.SetPosition(position);
						particle2.SetDamping(targetDamping);
						particle2.SetVelocity(glm::vec2(-m_particlePhysics2D.m_particleRadius * 2.0f, 0.0f) / static_cast<float>(Times::TimeStep()), m_particlePhysics2D.GetDeltaTime());

						const auto particleHandle3 = m_particlePhysics2D.AllocateParticle();
						auto& particle3 = m_particlePhysics2D.RefParticle(particleHandle3);
						particle3.SetColor(glm::vec4(glm::abs(glm::ballRand(1.0f)), 1.0f));
						particle3.SetPosition(position);
						particle3.SetDamping(targetDamping);
						particle3.SetVelocity(glm::vec2(0.0f, m_particlePhysics2D.m_particleRadius * 2.0f) / static_cast<float>(Times::TimeStep()), m_particlePhysics2D.GetDeltaTime());

						const auto particleHandle4 = m_particlePhysics2D.AllocateParticle();
						auto& particle4 = m_particlePhysics2D.RefParticle(particleHandle4);
						particle4.SetColor(glm::vec4(glm::abs(glm::ballRand(1.0f)), 1.0f));
						particle4.SetPosition(position);
						particle4.SetDamping(targetDamping);
						particle4.SetVelocity(glm::vec2(0.0f, -m_particlePhysics2D.m_particleRadius * 2.0f) / static_cast<float>(Times::TimeStep()), m_particlePhysics2D.GetDeltaTime());
					}
				}, [&](const ImVec2 origin, const float zoomFactor, ImDrawList* drawList)
					{
						const auto worldCenter = m_worldCenter * zoomFactor;
						drawList->AddCircle(origin + ImVec2(worldCenter.x, worldCenter.y),
							m_worldRadius * zoomFactor,
							IM_COL32(255,
								0,
								0, 255));
					},
					showGrid
					);
		}
		ImGui::End();
	}
}

void ParticlePhysics2DDemo::FixedUpdate()
{
	const auto gravity = m_gravityDirection * m_gravityStrength;
	m_particlePhysics2D.Simulate(Times::FixedDeltaTime(), [&](auto& particle)
		{
			//Apply gravity
			glm::vec2 acceleration = gravity;
			auto friction = -particle.GetVelocity(m_particlePhysics2D.GetDeltaTime()) * m_friction * m_particlePhysics2D.GetDeltaTime();
			if (!glm::any(glm::isnan(friction)))
			{
				acceleration += friction;
			}
			{
				particle.SetAcceleration(acceleration);
			}
			//Apply constraints
			{
				const auto toCenter = particle.GetPosition() - m_worldCenter;
				const auto distance = glm::length(toCenter);
				if (distance > m_worldRadius - m_particlePhysics2D.m_particleRadius)
				{
					const auto n = toCenter / distance;
					particle.Move(m_worldCenter + n * (m_worldRadius - m_particlePhysics2D.m_particleRadius));
				}
			}
		}
	);
}