#pragma once
#include "Shader.hpp"

/// <summary>
// Emissive Shader that samples colour values from a texture and emits that colour as light, without being affected by scene lighting. Used for the monitor screen in the scene.
/// </summary>
class EmissionShader : public Shader
{
private:
	const std::vector<uint8_t>* albedoTexture_;
	const int texWidth_, texHeight_;
	bool shadowTest_;
public:
	EmissionShader(const std::vector<uint8_t>* albedoTexture, int texWidth, int texHeight, bool shadowTest = false)
		:shadowTest_(shadowTest), albedoTexture_(albedoTexture),
		texWidth_(texWidth), texHeight_(texHeight)
	{
	}

	virtual Eigen::Vector3f getColor(const HitInfo& hitInfo,
		const Renderable* scene,
		const std::vector<std::unique_ptr<Light>>& lights,
		const Eigen::Vector3f& ambientLight,
		int currBounceCount,
		const int maxBounces) const
	{
		Eigen::Vector3f albedo;

		Eigen::Vector2f tex = hitInfo.texCoords;
		int pixX = static_cast<int>(tex.x() * texWidth_);
		int pixY = static_cast<int>((1.f - tex.y()) * texHeight_);
		pixX = std::max(pixX, 0);
		pixY = std::max(pixY, 0);
		pixX = std::min(pixX, texWidth_ - 1);
		pixY = std::min(pixY, texHeight_ - 1);

		albedo.x() = static_cast<float>((*albedoTexture_)[(pixX + texWidth_ * pixY) * 4 + 0]) / 255.f;
		albedo.y() = static_cast<float>((*albedoTexture_)[(pixX + texWidth_ * pixY) * 4 + 1]) / 255.f;
		albedo.z() = static_cast<float>((*albedoTexture_)[(pixX + texWidth_ * pixY) * 4 + 2]) / 255.f;

		Eigen::Vector3f color = albedo;

		return color;
	}
};

