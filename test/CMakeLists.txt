# Generate a test for a board
# TEST the test application, implemented in ${TEST}.cpp
# BOARD_LIB a library for a board containing SystemInit() and a linker script for embedded platforms
function(board_test TEST BOARD_LIB)
	# check if board library exists for the current platform
	if(TARGET ${BOARD_LIB})
		string(REGEX REPLACE ".*\\:" "" BOARD ${BOARD_LIB})
		set(NAME "${TEST}-${BOARD}")
		message("*** Test ${TEST} on board ${BOARD}")

		add_executable(${NAME}
			${TEST}.cpp
		)
		target_include_directories(${NAME}
			PRIVATE
				../
				${BOARD}
		)
		target_link_libraries(${NAME}
			${BOARD_LIB}
			coco-dac::coco-dac
			${PROJECT_NAME}
		)

		# generate hex file for flashing the target
		if(${CMAKE_CROSSCOMPILING})
			#message("*** Generate Hex for ${NAME} using ${CMAKE_OBJCOPY}")
			add_custom_command(TARGET ${NAME}
				POST_BUILD
				COMMAND ${CMAKE_OBJCOPY} -O ihex ${NAME} ${NAME}.hex
			)
		endif()
	endif()
endfunction()

# progbox test board, derive from stm32g474nucleo
#if(${PLATFORM} STREQUAL "stm32g474xx")
#	add_library(progbox
#		progbox/debug.cpp
#	)
#
#	# inherit from stm32g474nucleo
#	target_link_libraries(progbox
#		PUBLIC
#			coco-devboards::stm32g474nucleo
#	)
#endif()

board_test(AdcTest coco-devboards::stm32g474nucleo)

board_test(AdcDeviceTest coco-devboards::native)
board_test(AdcDeviceTest coco-devboards::stm32f3348discovery)
board_test(AdcDeviceTest coco-devboards::stm32g431nucleo)
board_test(AdcDeviceTest coco-devboards::stm32g474nucleo)
#board_test(AdcDeviceTest progbox)
