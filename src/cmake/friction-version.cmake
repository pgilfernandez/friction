#
# Friction - https://friction.graphics
#
# Copyright (c) Ole-André Rodlie and contributors
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# See 'README.md' for more information.
#

set(PROJECT_VERSION_MAJOR 1)
set(PROJECT_VERSION_MINOR 0)
set(PROJECT_VERSION_PATCH 0)
set(PROJECT_VERSION_TWEAK 0)

if (PROJECT_VERSION_TWEAK GREATER 0)
    set(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}.${PROJECT_VERSION_TWEAK})
else()
    set(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})
endif()

set(GIT_BRANCH "" CACHE STRING "Git branch")
set(GIT_COMMIT "" CACHE STRING "Git commit")
set(CUSTOM_BUILD "" CACHE STRING "Custom build")
if (NOT CUSTOM_BUILD STREQUAL "")
    add_definitions(-DCUSTOM_BUILD="${CUSTOM_BUILD}")
endif()
option(FRICTION_OFFICIAL_RELEASE "" OFF)
if (${FRICTION_OFFICIAL_RELEASE})
    add_definitions(-DPROJECT_OFFICIAL)
endif()

if(NOT GIT_COMMIT OR NOT GIT_BRANCH)
    if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
        find_package(Git QUIET)
        if(GIT_FOUND)
            if(NOT GIT_COMMIT)
                execute_process(
                    COMMAND ${GIT_EXECUTABLE} rev-parse --short=8 HEAD
                    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                    OUTPUT_VARIABLE GIT_COMMIT_AUTO
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                    ERROR_QUIET
                )
                set(GIT_COMMIT ${GIT_COMMIT_AUTO})
            endif()
            if(NOT GIT_BRANCH)
                execute_process(
                    COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
                    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                    OUTPUT_VARIABLE GIT_BRANCH_AUTO
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                    ERROR_QUIET
                )
                set(GIT_BRANCH ${GIT_BRANCH_AUTO})
            endif()
        endif()
    endif()
endif()
