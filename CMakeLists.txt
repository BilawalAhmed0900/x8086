# CMakeList.txt : CMake project for x8086, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("x8086")

# Add source to this project's executable.
add_executable (x8086 "src/x8086.cpp" "src/utils/Logger.h" "src/system/System.h" "src/system/MemoryBus.h" "src/system/ROM.h" "src/system/ROM.cpp" "src/system/BIOSROM.cpp" "src/system/System.cpp" "src/system/IOBus.h" "src/utils/LoadFile.h" "src/utils/LoadFile.cpp" "src/system/MemoryBus.cpp" "src/system/Callbacks.h" "src/system/CPU386.cpp" "src/system/CPU386.h" "src/system/MemoryDevice.cpp" "src/utils/NextId.h" "src/utils/NextId.cpp"  "src/system/instructions/InstructionMicrostep.h"  "src/system/instructions/Instruction.h" "src/system/instructions/Instruction.cpp" "src/system/instructions/Opcodes.h" "src/system/instructions/x00.h" "src/system/instructions/x00.cpp")

target_compile_definitions(x8086 PRIVATE $<$<CONFIG:Debug>:IS_DEBUG_BUILD>)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET x8086 PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
