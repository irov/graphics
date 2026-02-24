@echo off

if ["%~1"]==[""] (
  @echo invalid arguments, please select configuration
  goto end
)

set "CONFIGURATION=%1"
set "SOLUTION_DIR=..\solutions\graphics_msvc17_%CONFIGURATION%"

@pushd ..
@mkdir %SOLUTION_DIR%
@pushd %SOLUTION_DIR%

CMake -G "Visual Studio 17 2022" -A Win32 "%CD%\..\.." -DCMAKE_CONFIGURATION_TYPES:STRING=%CONFIGURATION% -DGRAPHICS_EXAMPLES_BUILD:BOOL=TRUE -DGRAPHICS_TESTS:BOOL=TRUE

@popd
@popd

:end
@echo Done

@pause