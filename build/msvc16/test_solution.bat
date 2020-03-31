@echo off

if ["%~1"]==[""] (
  @echo invalid arguments, please select configuration
  goto end
)

set "CONFIGURATION=%1"
set "SOLUTION_DIR=..\solutions\graphics_msvc16_test_%CONFIGURATION%"

@pushd ..
@mkdir %SOLUTION_DIR%
@pushd %SOLUTION_DIR%

CMake -G "Visual Studio 16 2019" -A Win32 "%CD%\..\.." -DCMAKE_CONFIGURATION_TYPES:STRING=%CONFIGURATION% -DGRAPHICS_EXAMPLES_BUILD:BOOL=FALSE -DGRAPHICS_TESTS:BOOL=TRUE
CMake --build . --config %CONFIGURATION%
CTest -C %CONFIGURATION%

@popd
@popd

:end
@echo Done

@pause