/* STL Headers */
#include <filesystem>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstdio>
#include <vector>
#include <string>
#include <string_view>
#include <format>
#include <span>
#include <unordered_map>
#include <algorithm>
#include <functional>
#include <memory>
#include <cstdint>
#include <cassert>
#include <sstream>
#include <chrono>
#include <utility>
#include <optional>
#include <type_traits>
#include <typeindex>
#include <thread>
#include <mutex>
#include <random>

/* Vendor Headers */
#include <entt/entt.hpp>
#include <entt/resource/cache.hpp>
#include <entt/resource/loader.hpp>
#include <glad/glad.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/string_cast.hpp>

/* Custom Headers */
#include <WinkEngine/Core/PrimitiveTypes.hpp>

/* Definitions and Macros */
namespace fs = std::filesystem;

#define COPY_CTOR(Class) Class(const Class&)
#define COPY_ASSIGN(Class) Class& operator=(const Class&)
#define MOVE_CTOR(Class) Class(Class&& o)
#define MOVE_ASSIGN(Class) Class& operator=(Class&& o)

#define COPY_CTOR_IMPL(Class) Class::Class(const Class&)
#define COPY_ASSIGN_IMPL(Class) Class& Class::operator=(const Class&)
#define MOVE_CTOR_IMPL(Class) Class::Class(Class&& o)
#define MOVE_ASSIGN_IMPL(Class) Class& Class::operator=(Class&& o)

#define DISABLE_COPY(Class)		\
	COPY_CTOR(Class) = delete;	\
	COPY_ASSIGN(Class) = delete

#define DISABLE_MOVE(Class)		\
	MOVE_CTOR(Class) = delete;	\
	MOVE_ASSIGN(Class) = delete