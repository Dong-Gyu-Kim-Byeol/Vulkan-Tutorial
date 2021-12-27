#pragma once

#include<vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

//#define TINYOBJLOADER_IMPLEMENTATION	// cpp
#include <tiny_obj_loader.h>


#define SHOW_FRAME
#define CUBE

#include <array>
#include <vector>
#include <chrono>
#include <thread>
#include <mutex>

#include <cassert>
#include <cmath>

#include "Error.h"


constexpr uint32_t VERTEX_BINDING = 0;
constexpr uint32_t INSTANCE_BINDING = 1;

const std::string MODEL_PATH = "./models/chalet.obj";

constexpr size_t THREAD_COUNT = 8; // or 10

constexpr uint32_t LOADING_BOXES_DIVISION = 3; //25; // 2n -1
constexpr uint32_t CUBE_COUNT_PER_AXIS_OF_LOADING_BOX = 10;
constexpr size_t LOADING_BOX_COUNT = LOADING_BOXES_DIVISION * LOADING_BOXES_DIVISION * LOADING_BOXES_DIVISION;
constexpr size_t CUBE_COUNT_OF_LOADING_BOX = CUBE_COUNT_PER_AXIS_OF_LOADING_BOX * CUBE_COUNT_PER_AXIS_OF_LOADING_BOX * CUBE_COUNT_PER_AXIS_OF_LOADING_BOX;
constexpr size_t CUBE_COUNT_ALL = LOADING_BOX_COUNT * CUBE_COUNT_OF_LOADING_BOX;

struct Int64_tPosision
{
	int64_t X;
	int64_t Y;
	int64_t Z;
};
struct size_tPosision
{
	size_t X;
	size_t Y;
	size_t Z;
};

class Vertex final
{
public:
	glm::vec3 Pos;
	glm::vec3 Color;
	glm::vec2 TexCoord;

	static VkVertexInputBindingDescription InputBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = VERTEX_BINDING;	// index
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::vector<VkVertexInputAttributeDescription> InputAttributeDescriptions()
	{
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
		attributeDescriptions.reserve(3);
		VkVertexInputAttributeDescription attributeDescription = {};
		attributeDescription.binding = VERTEX_BINDING;

		attributeDescription.location = 0;
		attributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;	// float: VK_FORMAT_R32_SFLOAT // vec2: VK_FORMAT_R32G32_SFLOAT // vec3 : VK_FORMAT_R32G32B32_SFLOAT // vec4 : VK_FORMAT_R32G32B32A32_SFLOAT
		attributeDescription.offset = offsetof(Vertex, Pos);
		attributeDescriptions.push_back(attributeDescription);

		attributeDescription.location = 1;
		attributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescription.offset = offsetof(Vertex, Color);
		attributeDescriptions.push_back(attributeDescription);

		attributeDescription.location = 2;
		attributeDescription.format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescription.offset = offsetof(Vertex, TexCoord);
		attributeDescriptions.push_back(attributeDescription);

		return attributeDescriptions;
	}

	static float ReverseTexCoordY(float origin)	// GLFW에 맞추기위해서 변환 매트릭스에서 정점 y값을 반대로 하기 때문에 텍스처 좌표역시 반대로 해야한다.
	{
		return 1.0f - origin;
	}

	bool operator==(const Vertex& other) const
	{
		return Pos == other.Pos && Color == other.Color && TexCoord == other.TexCoord;
	}
};
namespace std
{
	template<> struct hash<Vertex>
	{
		size_t operator()(Vertex const& vertex) const
		{
			return ((hash<glm::vec3>()(vertex.Pos)
				^ (hash<glm::vec3>()(vertex.Color) << 1))
				>> 1) ^ (hash<glm::vec2>()(vertex.TexCoord) << 1);
		}
	};
}

enum class eGameObjectType : uint32_t
{
	EMPTY,
	HOUSE,
	COUNT
};
class GameObject final
{
private:
	glm::mat4 mMat;
	eGameObjectType mType;

public:
	GameObject(const eGameObjectType _type, const glm::mat4& _mat)
	{
		mType = _type;
		mMat = _mat;
	}

	const glm::mat4& GetMat4() const
	{
		return mMat;
	}
	void SetMat4(const glm::mat4& _mat)
	{
		mMat = _mat;
	}

	eGameObjectType GetType() const
	{
		return mType;
	}

	static VkVertexInputBindingDescription InputBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = INSTANCE_BINDING;	// index
		bindingDescription.stride = sizeof(GameObject);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

		return bindingDescription;
	}

	static std::vector<VkVertexInputAttributeDescription> InputAttributeDescriptions()
	{
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
		attributeDescriptions.reserve(5);
		VkVertexInputAttributeDescription attributeDescription = {};
		attributeDescription.binding = INSTANCE_BINDING;

		attributeDescription.location = 3;
		attributeDescription.format = VK_FORMAT_R32G32B32A32_SFLOAT;	// float: VK_FORMAT_R32_SFLOAT // vec2: VK_FORMAT_R32G32_SFLOAT // vec3 : VK_FORMAT_R32G32B32_SFLOAT // vec4 : VK_FORMAT_R32G32B32A32_SFLOAT
		attributeDescription.offset = 0 * sizeof(glm::vec4);
		attributeDescriptions.push_back(attributeDescription);

		attributeDescription.location = 4;
		attributeDescription.format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescription.offset = 1 * sizeof(glm::vec4);
		attributeDescriptions.push_back(attributeDescription);

		attributeDescription.location = 5;
		attributeDescription.format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescription.offset = 2 * sizeof(glm::vec4);
		attributeDescriptions.push_back(attributeDescription);

		attributeDescription.location = 6;
		attributeDescription.format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescription.offset = 3 * sizeof(glm::vec4);
		attributeDescriptions.push_back(attributeDescription);

		attributeDescription.location = 7;
		attributeDescription.format = VK_FORMAT_R32_UINT;
		attributeDescription.offset = offsetof(GameObject, mType);
		attributeDescriptions.push_back(attributeDescription);

		return attributeDescriptions;
	}
};



enum class eCubeObjectType : uint32_t
{
	EMPTY,
	COLOR,
	COUNT,
	HIDDEN_FLAG = 1u << 30,
	SHOW_FLAG = 1u << 31
};
class CubeObject final
{
public:
	eCubeObjectType Type;
	glm::vec3 Pos;

	static VkVertexInputBindingDescription InputBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = INSTANCE_BINDING;	// index
		bindingDescription.stride = sizeof(CubeObject);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

		return bindingDescription;
	}

	static std::vector<VkVertexInputAttributeDescription> InputAttributeDescriptions()
	{
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
		attributeDescriptions.reserve(3);
		VkVertexInputAttributeDescription attributeDescription = {};
		attributeDescription.binding = INSTANCE_BINDING;

		attributeDescription.location = 3;
		attributeDescription.format = VK_FORMAT_R32_UINT;
		attributeDescription.offset = offsetof(CubeObject, Type);
		attributeDescriptions.push_back(attributeDescription);

		attributeDescription.location = 4;
		attributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescription.offset = offsetof(CubeObject, Pos);
		attributeDescriptions.push_back(attributeDescription);

		return attributeDescriptions;
	}
};

struct UniformBufferObject
{
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

struct PushConstantObject
{
	glm::mat4 ProjView;
	glm::mat4 View;
	glm::mat4 Proj;
};
struct PushConstantCmdObject
{
	glm::mat4 ProjView;
};


struct LoadingBox
{
	bool bReady;
	std::vector<CubeObject> Cubes;
	size_t ShowCubeCount;
#if 0
	std::vector<InstanceVector> InstanceVectors;
#endif
};

class LoadingBoxes final
{
private:
	//std::vector<LoadingBox> mData;

	bool mbReady;
	std::vector<LoadingBox> mData;
	Int64_tPosision mMinPos;
	Int64_tPosision mMaxPos;
	Int64_tPosision mCenterBoxMinPos;
	Int64_tPosision mCenterBoxMaxPos;

public:
	LoadingBoxes();
	~LoadingBoxes() = default;

	const std::vector<LoadingBox>& GetConstData() const;
	size_t GetShowCubeCount(size_t _boxIndex) const;

	void SetLoadingBoxes(glm::vec3 _playerPos);
	void SetCube(const Int64_tPosision _pos, const eCubeObjectType _type);

	//void AddInstanceVector(const InstanceBufferObjectType _type, const InstanceVector& _rInstanceVector);// +LoadingBoxes pos 수정 필요
	//InstanceVector& GetInstanceVector(const InstanceBufferObjectType _type);// +LoadingBoxes pos 수정 필요


private:
	size_t getLoadingBoxIndex(const Int64_tPosision _pos) const;

};


class GameData final
{
public:
	//std::mutex MutexGameData;

#ifdef CUBE
	std::vector<Vertex> CubeVertices =
	{
		// up
		{{0.0f, 1.0f, -1.0f}, {0.5f, 0.5f, 0.5f}, {0.0f, Vertex::ReverseTexCoordY(1.0f)}},
		{{0.0f, 1.0f, 0.0f}, {0.5f, 0.5f, 0.5f}, {0.0f, Vertex::ReverseTexCoordY(0.0f)}},
		{{1.0f, 1.0f, 0.0f}, {0.5f, 0.5f, 0.5f}, {1.0f, Vertex::ReverseTexCoordY(0.0f)}},
		{{1.0f, 1.0f, -1.0f}, {0.5f, 0.5f, 0.5f}, {1.0f, Vertex::ReverseTexCoordY(1.0f)}},
		
		// down
		{{0.0f, 0.0f, 0.0f}, {0.5f, 0.5f, 0.5f}, {0.0f, Vertex::ReverseTexCoordY(1.0f)}},
		{{0.0f, 0.0f, -1.0f}, {0.5f, 0.5f, 0.5f}, {0.0f, Vertex::ReverseTexCoordY(0.0f)}},
		{{1.0f, 0.0f, -1.0f}, {0.5f, 0.5f, 0.5f}, {1.0f, Vertex::ReverseTexCoordY(0.0f)}},
		{{1.0f, 0.0f, 0.0f}, {0.5f, 0.5f, 0.5f}, {1.0f, Vertex::ReverseTexCoordY(1.0f)}},

		// front
		{{0.0f, 1.0f, 0.0f}, {0.5f, 0.5f, 0.5f}, {0.0f, Vertex::ReverseTexCoordY(1.0f)}},
		{{0.0f, 0.0f, 0.0f}, {0.5f, 0.5f, 0.5f}, {0.0f, Vertex::ReverseTexCoordY(0.0f)}},
		{{1.0f, 0.0f, 0.0f}, {0.5f, 0.5f, 0.5f}, {1.0f, Vertex::ReverseTexCoordY(0.0f)}},
		{{1.0f, 1.0f, 0.0f}, {0.5f, 0.5f, 0.5f}, {1.0f, Vertex::ReverseTexCoordY(1.0f)}},

		// back
		{{1.0f, 1.0f, -1.0f}, {0.5f, 0.5f, 0.5f}, {0.0f, Vertex::ReverseTexCoordY(1.0f)}},
		{{1.0f, 0.0f, -1.0f}, {0.5f, 0.5f, 0.5f}, {0.0f, Vertex::ReverseTexCoordY(0.0f)}},
		{{0.0f, 0.0f, -1.0f}, {0.5f, 0.5f, 0.5f}, {1.0f, Vertex::ReverseTexCoordY(0.0f)}},
		{{0.0f, 1.0f, -1.0f}, {0.5f, 0.5f, 0.5f}, {1.0f, Vertex::ReverseTexCoordY(1.0f)}},

		// left
		{{0.0f, 1.0f, -1.0f}, {0.5f, 0.5f, 0.5f}, {0.0f, Vertex::ReverseTexCoordY(1.0f)}},
		{{0.0f, 0.0f, -1.0f}, {0.5f, 0.5f, 0.5f}, {0.0f, Vertex::ReverseTexCoordY(0.0f)}},
		{{0.0f, 0.0f, 0.0f}, {0.5f, 0.5f, 0.5f}, {1.0f, Vertex::ReverseTexCoordY(0.0f)}},
		{{0.0f, 1.0f, 0.0f}, {0.5f, 0.5f, 0.5f}, {1.0f, Vertex::ReverseTexCoordY(1.0f)}},

		// right
		{{1.0f, 1.0f, 0.0f}, {0.5f, 0.5f, 0.5f}, {0.0f, Vertex::ReverseTexCoordY(1.0f)}},
		{{1.0f, 0.0f, 0.0f}, {0.5f, 0.5f, 0.5f}, {0.0f, Vertex::ReverseTexCoordY(0.0f)}},
		{{1.0f, 0.0f, -1.0f}, {0.5f, 0.5f, 0.5f}, {1.0f, Vertex::ReverseTexCoordY(0.0f)}},
		{{1.0f, 1.0f, -1.0f}, {0.5f, 0.5f, 0.5f}, {1.0f, Vertex::ReverseTexCoordY(1.0f)}},
	};

	std::vector<uint32_t> CubeIndices =
	{
		0, 1, 2, 2, 3, 0, // up
		4, 5, 6, 6, 7, 4, // down
		8, 9, 10, 10, 11, 8, // front
		12, 13, 14, 14, 15, 12, // back
		16, 17, 18, 18, 19, 16, // left
		20, 21, 22, 22, 23, 20 // right
	};
#endif //CUBE

public:
	GameData() = default;
	~GameData() = default;
	GameData(const GameData& _rOther) = delete;
	GameData& operator=(const GameData& _rOther) = delete;

	void Update();

	void InitData(float _width, float _height);
	void InitTime();

	void SetProjectionMat(float _width, float _height);

	const std::vector<Vertex>& GetVertices() const;
	const std::vector<uint32_t>& GetIndices() const;

	size_t GetInstancesDataByteCapacitySize() const;
	size_t GetInstancesDataByteSize(const eGameObjectType _type) const;
	size_t GetInstancesDataCount(const eGameObjectType _type) const;
	void CpyInstancesData(const eGameObjectType _type, void* _out_pData) const;

	size_t GetCubeDataByteCapacitySize() const;
	size_t GetShowCubeDataCount() const;
	void ThreadCpyShowCubeData(void* _out_pData) const;

	const PushConstantObject& GetPushConstantData() const;

	

private:
	float mTime;
	std::chrono::steady_clock::time_point mStartTime;
	
	LoadingBoxes mLoadingBoxes;
	std::vector<GameObject> mObjects;

	std::vector<Vertex> mVertices;
	std::vector<uint32_t> mIndices;
	PushConstantObject mPushConstantData;

private:
	void cpyShowCubeData(char* _out_pData, const size_t _startCopyOffset,
		const size_t _startBoxesIndex, size_t _count) const;

	void loadModel();
	void showFrameRate();
};