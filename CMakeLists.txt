INCLUDE_DIRECTORIES(include/)
FILE(GLOB_RECURSE MAIN_SOURCE src/*.cpp)
ADD_EXECUTABLE(main ${MAIN_SOURCE})