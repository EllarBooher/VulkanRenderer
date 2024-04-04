#pragma once

#include "engine_types.h"
#include <variant>
#include <vector>
#include <map>

/**
	Contains reflected data from a ShaderModule, to aid with UI and proper piping of data.
	Work in progress, for now supports a very limited amount of reflection.
*/
struct ShaderReflectionData
{
	/**
		Type names correspond to the SPIR-V specification.
		The type names are not meant to exactly match the specification opcodes and layouts,
		just model it in a way that's useful.
		See https://registry.khronos.org/SPIR-V/specs/unified1/SPIRV.html section "2.2.2. Types"
	*/

	// Corresponds to OpTypeInt
	struct Integer
	{
		bool signedness;

		bool operator==(Integer const& other) const;
	};

	// Corresponds to OpTypeFloat
	using Float = std::monostate;
	using Scalar = std::monostate;

	struct Vector {
		uint32_t componentCount;

		bool operator==(Vector const& other) const;
	};
	struct Matrix {
		uint32_t columnCount;
		uint32_t rowCount;

		bool operator==(Matrix const& other) const;
	};

	struct NumericType
	{
		using ComponentType = std::variant<Integer, Float>;
		using Format = std::variant<Scalar, Vector, Matrix>;

		uint32_t componentBitWidth;
		ComponentType componentType;
		Format format;

		bool operator==(NumericType const& other) const;
	};

	/**
		Represents a type whose reflection data could not be generated,
		usually because the specific type is not supported yet.
	*/
	using UnsupportedType = std::monostate;

	struct SizedType
	{
		std::variant<NumericType, UnsupportedType> typeData;

		std::string name;
		uint32_t sizeBytes;
		uint32_t paddedSizeBytes;
	};

	struct StructureMember
	{
		uint32_t offsetBytes;
		std::string name;
		SizedType type;
	};
	// Corresponds to OpTypeStruct
	struct Structure
	{
		// TODO: test if structures can be anonymous.
		std::string name;
		uint32_t sizeBytes;
		// TODO: figure out what exactly determines the padding size
		uint32_t paddedSizeBytes;
		std::vector<StructureMember> members;

		/**
			Mutually checks if the members of this struct match any bitwise overlapping members in the other struct.
		*/
		bool logicallyCompatible(Structure const& other) const;
	};

	/*
		TODO: structs can have padding, as in bits in their representation that are not overlapped by members.
		I need to investigate exactly how this works, and how best to model this.
		This is important for push constants, since padding bits in one shader may be
		accessed in another.
	*/

	/**
		As per the Vulkan specification, Push constants must be structs.
		There can also only be one per entry point.
		https://registry.khronos.org/vulkan/specs/1.3-extensions/html/vkspec.html#interfaces-resources-pushconst
	*/
	struct PushConstant
	{
		Structure type{};
		std::string name{};

		// This does not impact the offset values generation in the type data.
		uint32_t layoutOffsetBytes{ 0 };
	};
	std::map<std::string, PushConstant> pushConstantsByEntryPoint{};

	std::string defaultEntryPoint{};

	bool defaultEntryPointHasPushConstant() const { return pushConstantsByEntryPoint.contains(defaultEntryPoint); }
	PushConstant const& defaultPushConstant() const { return pushConstantsByEntryPoint.at(defaultEntryPoint); }
};

struct ShaderWrapper
{
	static ShaderWrapper Invalid() { return ShaderWrapper("", {}, VK_NULL_HANDLE); }
	static ShaderWrapper FromBytecode(VkDevice device, std::string name, std::span<uint8_t const> spirv_bytecode);

	VkShaderModule shaderModule() const { return m_shaderModule; }
	ShaderReflectionData const& reflectionData() const { return m_reflectionData; }
	std::string name() const { return m_name; }
	VkPushConstantRange pushConstantRange(VkShaderStageFlags stageMask) const;

	std::span<uint8_t> mapRuntimePushConstant(std::string entryPoint);
	std::span<uint8_t const> readRuntimePushConstant(std::string entryPoint) const;

	template<typename T, int N>
	inline bool validatePushConstant(std::array<T, N> pushConstantData, std::string entryPoint) const
	{
		std::span<uint8_t const> byteSpan{ reinterpret_cast<uint8_t const*>(pushConstantData.data()), sizeof(T) * N };
		return validatePushConstant(byteSpan, entryPoint);
	}

	inline bool validatePushConstant(std::span<uint8_t const> pushConstantData, std::string entryPoint) const
	{
		ShaderReflectionData::PushConstant const& pushConstant{ m_reflectionData.pushConstantsByEntryPoint.at(entryPoint) };

		// The push constant in a shader has padding up to the layout(offset) specifier.
		// We assume the data we are pushing is the rest of the struct past that offset.
		if (pushConstant.type.sizeBytes - pushConstant.layoutOffsetBytes != pushConstantData.size())
		{
			return false;
		}

		// TODO: check types of each member
		return true;
	}

	void cleanup(VkDevice device) const;

	bool isValid() const { return m_shaderModule != VK_NULL_HANDLE; }

	void resetRuntimeData();

	ShaderWrapper() {};
private:
	ShaderWrapper(std::string name, ShaderReflectionData reflectionData, VkShaderModule shaderModule)
		: m_name(name)
		, m_reflectionData(reflectionData)
		, m_shaderModule(shaderModule)
	{};

	std::string m_name{};
	ShaderReflectionData m_reflectionData{};
	VkShaderModule m_shaderModule{ VK_NULL_HANDLE };

	std::map<std::string, std::vector<uint8_t>> m_runtimePushConstantsByEntryPoint{};
};

struct ComputeShaderWrapper
{
	ShaderWrapper computeShader{};
	VkPipeline pipeline{ VK_NULL_HANDLE };
	VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };

	void cleanup(VkDevice device) {
		computeShader.cleanup(device);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyPipeline(device, pipeline, nullptr);
	}
};

namespace vkutil
{
	ShaderReflectionData generateReflectionData(std::span<uint8_t const> spirv_bytecode);
}