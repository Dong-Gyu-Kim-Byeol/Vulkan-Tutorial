#define TINYOBJLOADER_IMPLEMENTATION

#include "GameData.h"


// LoadingBoxes
// public
LoadingBoxes::LoadingBoxes() : mData(LOADING_BOX_COUNT)
{
	assert(LOADING_BOXES_DIVISION % 2 == 1);

	const CubeObject cube = { eCubeObjectType::EMPTY, {0.0f, 0.0f, 0.0f} };
	for (LoadingBox& box : mData)
	{
		box.Cubes.resize(CUBE_COUNT_OF_LOADING_BOX, cube);
		//box.InstanceVectors.resize(static_cast<size_t>(InstanceBufferObjectType::COUNT));
		box.ShowCubeCount = 0;
		box.bReady = true;
	}

	SetLoadingBoxes({ 0,0,0 });
}

const std::vector<LoadingBox>& LoadingBoxes::GetConstData() const
{
	return mData;
}
size_t LoadingBoxes::GetShowCubeCount(size_t _boxIndex) const
{
	return mData[_boxIndex].ShowCubeCount;
}

void LoadingBoxes::SetLoadingBoxes(glm::vec3 _playerPos)
{
	assert(static_cast<int64_t>(_playerPos.x) <= INT64_MAX);
	assert(static_cast<int64_t>(_playerPos.y) <= INT64_MAX);
	assert(static_cast<int64_t>(_playerPos.z) <= INT64_MAX);
	assert(static_cast<int64_t>(_playerPos.x) >= INT64_MIN);
	assert(static_cast<int64_t>(_playerPos.y) >= INT64_MIN);
	assert(static_cast<int64_t>(_playerPos.z) >= INT64_MIN);

	const int32_t cube_count_per_axis_of_loading_box = CUBE_COUNT_PER_AXIS_OF_LOADING_BOX;
	mCenterBoxMinPos = {};
	while (_playerPos.x > cube_count_per_axis_of_loading_box)
	{
		_playerPos.x -= cube_count_per_axis_of_loading_box;
		mCenterBoxMinPos.X -= cube_count_per_axis_of_loading_box;
	}
	while (_playerPos.y > cube_count_per_axis_of_loading_box)
	{
		_playerPos.y -= cube_count_per_axis_of_loading_box;
		mCenterBoxMinPos.Y -= cube_count_per_axis_of_loading_box;
	}
	while (_playerPos.z > cube_count_per_axis_of_loading_box)
	{
		_playerPos.z -= cube_count_per_axis_of_loading_box;
		mCenterBoxMinPos.Z -= cube_count_per_axis_of_loading_box;
	}
	mCenterBoxMaxPos =
	{
		mCenterBoxMaxPos.X + cube_count_per_axis_of_loading_box,
		mCenterBoxMaxPos.Y + cube_count_per_axis_of_loading_box,
		mCenterBoxMaxPos.Z + cube_count_per_axis_of_loading_box
	};

	mMinPos =
	{
		mCenterBoxMinPos.X - (LOADING_BOXES_DIVISION / 2) * cube_count_per_axis_of_loading_box,
		mCenterBoxMinPos.Y - (LOADING_BOXES_DIVISION / 2) * cube_count_per_axis_of_loading_box,
		mCenterBoxMinPos.Z - (LOADING_BOXES_DIVISION / 2) * cube_count_per_axis_of_loading_box
	};
	mMaxPos =
	{
		mCenterBoxMaxPos.X + (LOADING_BOXES_DIVISION / 2) * cube_count_per_axis_of_loading_box - 1,
		mCenterBoxMaxPos.Y + (LOADING_BOXES_DIVISION / 2) * cube_count_per_axis_of_loading_box - 1,
		mCenterBoxMaxPos.Z + (LOADING_BOXES_DIVISION / 2) * cube_count_per_axis_of_loading_box - 1
	};
}

void LoadingBoxes::SetCube(const Int64_tPosision _pos, const eCubeObjectType _type)
{
	assert(_pos.X <= mMaxPos.X);
	assert(_pos.Y <= mMaxPos.Y);
	assert(_pos.Z <= mMaxPos.Z);
	assert(_pos.X >= mMinPos.X);
	assert(_pos.Y >= mMinPos.Y);
	assert(_pos.Z >= mMinPos.Z);

	size_t boxIndex = getLoadingBoxIndex(_pos);

	Int64_tPosision loadingBoxPosMod;
	size_tPosision cubeIndexVec3;

	loadingBoxPosMod.X = _pos.X % CUBE_COUNT_PER_AXIS_OF_LOADING_BOX;
	if (loadingBoxPosMod.X < 0)
	{
		cubeIndexVec3.X = loadingBoxPosMod.X + CUBE_COUNT_PER_AXIS_OF_LOADING_BOX;
	}
	else
	{
		cubeIndexVec3.X = loadingBoxPosMod.X;
	}

	loadingBoxPosMod.Y = _pos.Y % CUBE_COUNT_PER_AXIS_OF_LOADING_BOX;
	if (loadingBoxPosMod.Y < 0)
	{
		cubeIndexVec3.Y = loadingBoxPosMod.Y + CUBE_COUNT_PER_AXIS_OF_LOADING_BOX;
	}
	else
	{
		cubeIndexVec3.Y = loadingBoxPosMod.Y;
	}

	loadingBoxPosMod.Z = _pos.Z % CUBE_COUNT_PER_AXIS_OF_LOADING_BOX;
	if (loadingBoxPosMod.Z < 0)
	{
		cubeIndexVec3.Z = loadingBoxPosMod.Z + CUBE_COUNT_PER_AXIS_OF_LOADING_BOX;
	}
	else
	{
		cubeIndexVec3.Z = loadingBoxPosMod.Z;
	}

	assert(cubeIndexVec3.X < CUBE_COUNT_PER_AXIS_OF_LOADING_BOX);
	assert(cubeIndexVec3.Y < CUBE_COUNT_PER_AXIS_OF_LOADING_BOX);
	assert(cubeIndexVec3.Z < CUBE_COUNT_PER_AXIS_OF_LOADING_BOX);

	const size_t cubeIndex = CUBE_COUNT_PER_AXIS_OF_LOADING_BOX * (CUBE_COUNT_PER_AXIS_OF_LOADING_BOX * cubeIndexVec3.Y + cubeIndexVec3.Z) + cubeIndexVec3.X;
	mData[boxIndex].Cubes[cubeIndex] =
	{ 
		static_cast<eCubeObjectType>(static_cast<uint32_t>(_type) | static_cast<uint32_t>(eCubeObjectType::SHOW_FLAG)),
		{ static_cast<float>(_pos.X), static_cast<float>(_pos.Y), static_cast<float>(_pos.Z) } 
	};
	mData[boxIndex].ShowCubeCount++;
}

// private
size_t LoadingBoxes::getLoadingBoxIndex(const Int64_tPosision _pos) const
{
	assert(_pos.X <= mMaxPos.X);
	assert(_pos.Y <= mMaxPos.Y);
	assert(_pos.Z <= mMaxPos.Z);
	assert(_pos.X >= mMinPos.X);
	assert(_pos.Y >= mMinPos.Y);
	assert(_pos.Z >= mMinPos.Z);

	size_tPosision i = {};
	Int64_tPosision boxMinPos = mMinPos;
	while (boxMinPos.X + CUBE_COUNT_PER_AXIS_OF_LOADING_BOX <= _pos.X)
	{
		boxMinPos.X += CUBE_COUNT_PER_AXIS_OF_LOADING_BOX;
		i.X++;
	}
	while (boxMinPos.Y + CUBE_COUNT_PER_AXIS_OF_LOADING_BOX <= _pos.Y)
	{
		boxMinPos.Y += CUBE_COUNT_PER_AXIS_OF_LOADING_BOX;
		i.Y++;
	}
	while (boxMinPos.Z + CUBE_COUNT_PER_AXIS_OF_LOADING_BOX <= _pos.Z)
	{
		boxMinPos.Z += CUBE_COUNT_PER_AXIS_OF_LOADING_BOX;
		i.Z++;
	}

	return LOADING_BOXES_DIVISION * (LOADING_BOXES_DIVISION * i.Y + i.Z) + i.X;
}



// GameData
// public
void GameData::Update()
{
	//std::scoped_lock<std::mutex> lock(mMutexGameData);

#ifdef SHOW_FRAME
	showFrameRate();
#endif

	mTime = std::chrono::duration<float, std::chrono::seconds::period>(std::chrono::high_resolution_clock::now() - mStartTime).count();
	mStartTime = std::chrono::high_resolution_clock::now();

	static int i = 0;
	if (i++ > 50)
	{
		//getInstanceVector(InstanceBufferObjectType::HOUSE).push_back({ glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, -1.0f, 1.0f)), InstanceBufferObjectType::HOUSE });
		i = 0;
	}

	for (GameObject& Object : mObjects)
	{
		Object.SetMat4(glm::rotate(Object.GetMat4(), mTime / 2.0f * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
	}

	//mPushConstantData.View = glm::rotate(mPushConstantData.View, mTime / 2.0f * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	mPushConstantData.ProjView = mPushConstantData.Proj * mPushConstantData.View;
}

void GameData::InitData(const float _width, const float _height)
{	// Z : 모니터 밖이 + 방향
	mLoadingBoxes.SetLoadingBoxes({ 2.0f, 2.0f, 2.0f });

	mLoadingBoxes.SetCube({ -1, 0, 1 }, eCubeObjectType::COLOR);
	mLoadingBoxes.SetCube({ -1, 0, 0 }, eCubeObjectType::COLOR);
	mLoadingBoxes.SetCube({ -1, 0, -1 }, eCubeObjectType::COLOR);
	mLoadingBoxes.SetCube({ 0, 0, 1 }, eCubeObjectType::COLOR);
	mLoadingBoxes.SetCube({ 0, 0, 0 }, eCubeObjectType::COLOR);
	mLoadingBoxes.SetCube({ 0, 0, -1 }, eCubeObjectType::COLOR);
	mLoadingBoxes.SetCube({ 1, 0, 1 }, eCubeObjectType::COLOR);
	mLoadingBoxes.SetCube({ 1, 0, 0 }, eCubeObjectType::COLOR);
	mLoadingBoxes.SetCube({ 1, 0, -1 }, eCubeObjectType::COLOR);

	//mLoadingBoxes.SetCube({ 0, 1, 0 }, eCubeObjectType::COLOR);
	//mLoadingBoxes.SetCube({ 0, 1, -1 }, eCubeObjectType::COLOR);

	const size_t maxCount = 60;
	std::vector<GameObject> instances;
	instances.reserve(maxCount);
	instances =
	{
		//{ glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f)), InstanceBufferObjectType::HOUSE },
		{ eGameObjectType::HOUSE, glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, -3.0f)) }
	};

	for (const auto& instance : instances)
	{
		//GameObject object(InstanceBufferObjectType::HOUSE, instance.Mat);
		mObjects.push_back(instance);
	}

	SetProjectionMat(_width, _height);
	mPushConstantData.View = glm::lookAt(glm::vec3(2.0f, 5.0f, 10.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	mPushConstantData.ProjView = mPushConstantData.Proj * mPushConstantData.View;


	loadModel();
}

void GameData::InitTime()
{
	mStartTime = std::chrono::high_resolution_clock::now();
}

void GameData::SetProjectionMat(const float _width, const float _height)
{
	mPushConstantData.Proj = glm::perspective(glm::radians(70.0f), _width / _height, 0.1f, 100.0f);
	mPushConstantData.Proj[1][1] *= -1;
}

const std::vector<Vertex>& GameData::GetVertices() const
{
	return mVertices;
}
const std::vector<uint32_t>& GameData::GetIndices() const
{
	return mIndices;
}

size_t GameData::GetInstancesDataByteCapacitySize() const
{
	return sizeof(GameObject) * mObjects.capacity();
}
size_t GameData::GetInstancesDataByteSize(const eGameObjectType _type) const
{
	size_t fullSize = 0;
	for (const auto& object : mObjects)
	{
		if (object.GetType() == _type)
		{ 
			fullSize += sizeof(GameObject);
		}
	}

	return fullSize;
}
size_t GameData::GetInstancesDataCount(const eGameObjectType _type) const
{
	size_t fullCount = 0;
	for (const auto& object : mObjects)
	{
		if (object.GetType() == _type)
		{
			fullCount++;
		}
	}

	return fullCount;
}
void GameData::CpyInstancesData(const eGameObjectType _type, void* _out_pData) const
{
	char* p = (char*)_out_pData;
	size_t copySize = 0;
	for (const auto& object : mObjects)
	{
		if (object.GetType() == _type)
		{
			memcpy(p + copySize, &object, sizeof(GameObject));
			copySize += sizeof(GameObject);
		}
	}
}

size_t GameData::GetCubeDataByteCapacitySize() const
{
	size_t fullSize = 0;
	for (const LoadingBox& box : mLoadingBoxes.GetConstData())
	{
		fullSize += sizeof(CubeObject) * box.Cubes.capacity();
	}

	return fullSize;
}
size_t GameData::GetShowCubeDataCount() const
{
	size_t fullCount = 0;
	for (const LoadingBox& box : mLoadingBoxes.GetConstData())
	{
		if (box.bReady == false) { continue; }

		fullCount += box.ShowCubeCount;
	}

	return fullCount;
}
void GameData::ThreadCpyShowCubeData(void* _out_pData) const
{
	char* p = (char*)_out_pData;

	std::vector<std::thread> threads;
	const size_t loadingBoxCount = mLoadingBoxes.GetConstData().size();

	size_t copySize = 0;
	size_t count = (loadingBoxCount / THREAD_COUNT) + 1;
	size_t startBoxesIndex = 0;
	for (size_t i = 0; i < THREAD_COUNT; i++)
	{
		startBoxesIndex = count * i;
		
		if ((loadingBoxCount - startBoxesIndex) <= count)
		{
			count = loadingBoxCount - startBoxesIndex;
			cpyShowCubeData(p, copySize, startBoxesIndex, count);
			break;
		}
		
		threads.push_back(std::thread(&GameData::cpyShowCubeData, &(*this), 
			p, copySize, startBoxesIndex, count));

		for (size_t i = startBoxesIndex; i < startBoxesIndex + count; i++)
		{
			copySize += mLoadingBoxes.GetShowCubeCount(i) * sizeof(CubeObject);
		}
	}

	for (std::thread& t : threads)
	{
		if (t.joinable() == true)
		{
			t.join();
		}
	}
}

const PushConstantObject& GameData::GetPushConstantData() const
{
	return mPushConstantData;
}


// private
void GameData::cpyShowCubeData(char* _out_pData, const size_t _startCopyOffset,
	const size_t _startBoxesIndex, size_t _count) const
{
	_count += _startBoxesIndex;
	_out_pData += _startCopyOffset;
	for (size_t i = _startBoxesIndex; i < _count; i++)
	{
		const LoadingBox& box = mLoadingBoxes.GetConstData()[i];

		if (box.bReady == false) { return; }

		for (const CubeObject& cube : box.Cubes)
		{
			if ((static_cast<uint32_t>(eCubeObjectType::SHOW_FLAG) & static_cast<uint32_t>(cube.Type))
				== static_cast<uint32_t>(eCubeObjectType::SHOW_FLAG))
			{
				memcpy(_out_pData, &cube, sizeof(CubeObject));
				_out_pData += sizeof(CubeObject);
			}
		}
	}
}

void GameData::loadModel()
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;	// ray tracing?
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str()))
	{
		Error("App", "loadModel", (warn + err).c_str());
	}


	std::unordered_map<Vertex, uint32_t> uniqueVertices = {};

	for (const tinyobj::shape_t& shape : shapes)
	{
		for (const tinyobj::index_t& index : shape.mesh.indices)
		{
			Vertex vertex = {};

			vertex.Pos =
			{
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			vertex.TexCoord =
			{
				attrib.texcoords[2 * index.texcoord_index + 0],
				Vertex::ReverseTexCoordY(attrib.texcoords[2 * index.texcoord_index + 1])
			};

			vertex.Color = { 1.0f, 1.0f, 1.0f };


			if (uniqueVertices.count(vertex) == 0)
			{
				uniqueVertices[vertex] = static_cast<uint32_t>(mVertices.size());
				mVertices.push_back(vertex);
			}

			mIndices.push_back(uniqueVertices[vertex]);
		}
	}
}

void GameData::showFrameRate()
{
	if (mTime == 0.0f) { return; }
	static size_t i = 0;
	static size_t count = 100;
	static bool bFirst = true;
	static std::vector<float> frames(count);


	float nowFrame = 1.0f / mTime;

	if (bFirst == true)
	{
		for (float& f : frames)
		{
			f = nowFrame;
		}
		bFirst = false;
	}

	frames[i] = nowFrame;

	float averageFrame = 0.0f;
	for (int j = 0; j < count; j++)
	{
		averageFrame += frames[j];
	}
	averageFrame /= count;

	std::cout << std::fixed;
	std::cout.precision(6);
	std::cout << "average frame rate : " << averageFrame << ",   now frame : " << nowFrame
		<< ",     Instances : " << mObjects.size() << std::endl;

	i++;
	if (i == count) { i = 0; }
}
